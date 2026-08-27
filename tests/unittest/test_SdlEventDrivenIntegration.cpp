#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include <SDL3/SDL.h>

#include "src/common/Events.hpp"
#include "src/core/EventLoop.hpp"
#include "src/core/SdlEventWakeup.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/systems/InteractionSystem.hpp"

namespace ui::tests
{
namespace
{

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

class EventObservation final
{
   public:
    explicit EventObservation(EventLoop& eventLoop) : m_eventLoop(&eventLoop)
    {
    }

    void onPointerMove(const events::RawPointerMove& event)
    {
        {
            std::scoped_lock lock(m_mutex);
            m_pointerMove = event;
            m_pointerThread = std::this_thread::get_id();
        }
        m_pointerCount.fetch_add(1U, std::memory_order_release);
        m_cv.notify_all();
    }

    void onKeyInput(const events::RawKeyInput& event)
    {
        {
            std::scoped_lock lock(m_mutex);
            m_keyInput = event;
            m_keyThread = std::this_thread::get_id();
        }
        m_keyCount.fetch_add(1U, std::memory_order_release);
        m_cv.notify_all();
    }

    void onWindowResized(const events::WindowPixelSizeChanged& event)
    {
        {
            std::scoped_lock lock(m_mutex);
            m_windowResized = event;
            m_resizeThread = std::this_thread::get_id();
        }
        m_resizeCount.fetch_add(1U, std::memory_order_release);
        m_cv.notify_all();
    }

    void onWindowExposed(const events::WindowExposed& event)
    {
        {
            std::scoped_lock lock(m_mutex);
            m_windowExposed = event;
            m_exposeThread = std::this_thread::get_id();
        }
        m_exposeCount.fetch_add(1U, std::memory_order_release);
        m_cv.notify_all();
    }

    void onQuitRequested(const events::QuitRequested&)
    {
        m_quitCount.fetch_add(1U, std::memory_order_release);
        m_cv.notify_all();
        if (m_stopOnQuit.load(std::memory_order_acquire))
        {
            m_eventLoop->quit();
        }
    }

    void recordFrame()
    {
        {
            std::scoped_lock lock(m_mutex);
            m_consumerThread = std::this_thread::get_id();
        }
        m_frameCount.fetch_add(1U, std::memory_order_release);
        m_cv.notify_all();
    }

    template <typename Predicate>
    [[nodiscard]] bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(m_mutex);
        return m_cv.wait_for(lock, timeout, std::forward<Predicate>(predicate));
    }

    [[nodiscard]] bool waitForIdleWindow(std::chrono::milliseconds quietWindow,
                                         std::chrono::milliseconds timeout)
    {
        const auto deadline = Clock::now() + timeout;
        auto observedFrames = frameCount();
        std::unique_lock lock(m_mutex);

        while (Clock::now() < deadline)
        {
            if (!m_cv.wait_for(lock, quietWindow,
                               [this, observedFrames]
                               { return m_frameCount.load(std::memory_order_acquire) != observedFrames; }))
            {
                return true;
            }
            observedFrames = m_frameCount.load(std::memory_order_acquire);
        }
        return false;
    }

    [[nodiscard]] std::uint32_t frameCount() const noexcept
    {
        return m_frameCount.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint32_t pointerCount() const noexcept
    {
        return m_pointerCount.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint32_t keyCount() const noexcept
    {
        return m_keyCount.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint32_t resizeCount() const noexcept
    {
        return m_resizeCount.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint32_t exposeCount() const noexcept
    {
        return m_exposeCount.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint32_t quitCount() const noexcept
    {
        return m_quitCount.load(std::memory_order_acquire);
    }

    void stopOnQuit() noexcept
    {
        m_stopOnQuit.store(true, std::memory_order_release);
    }

    [[nodiscard]] events::RawPointerMove pointerMove() const
    {
        std::scoped_lock lock(m_mutex);
        return m_pointerMove;
    }

    [[nodiscard]] events::RawKeyInput keyInput() const
    {
        std::scoped_lock lock(m_mutex);
        return m_keyInput;
    }

    [[nodiscard]] events::WindowPixelSizeChanged windowResized() const
    {
        std::scoped_lock lock(m_mutex);
        return m_windowResized;
    }

    [[nodiscard]] events::WindowExposed windowExposed() const
    {
        std::scoped_lock lock(m_mutex);
        return m_windowExposed;
    }

    [[nodiscard]] std::thread::id consumerThread() const
    {
        std::scoped_lock lock(m_mutex);
        return m_consumerThread;
    }

    [[nodiscard]] std::thread::id pointerThread() const
    {
        std::scoped_lock lock(m_mutex);
        return m_pointerThread;
    }

    [[nodiscard]] std::thread::id keyThread() const
    {
        std::scoped_lock lock(m_mutex);
        return m_keyThread;
    }

    [[nodiscard]] std::thread::id resizeThread() const
    {
        std::scoped_lock lock(m_mutex);
        return m_resizeThread;
    }

    [[nodiscard]] std::thread::id exposeThread() const
    {
        std::scoped_lock lock(m_mutex);
        return m_exposeThread;
    }

   private:
    EventLoop* m_eventLoop;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<std::uint32_t> m_frameCount{0};
    std::atomic<std::uint32_t> m_pointerCount{0};
    std::atomic<std::uint32_t> m_keyCount{0};
    std::atomic<std::uint32_t> m_resizeCount{0};
    std::atomic<std::uint32_t> m_exposeCount{0};
    std::atomic<std::uint32_t> m_quitCount{0};
    std::atomic<bool> m_stopOnQuit{false};
    events::RawPointerMove m_pointerMove{};
    events::RawKeyInput m_keyInput{};
    events::WindowPixelSizeChanged m_windowResized{};
    events::WindowExposed m_windowExposed{};
    std::thread::id m_consumerThread{};
    std::thread::id m_pointerThread{};
    std::thread::id m_keyThread{};
    std::thread::id m_resizeThread{};
    std::thread::id m_exposeThread{};
};

class SdlEventDrivenIntegrationTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_EQ(SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_EVENTS), 0U);
        if (SDL_getenv("SDL_VIDEODRIVER") == nullptr)
        {
            ASSERT_TRUE(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")) << SDL_GetError();
        }
        ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) << SDL_GetError();

        m_window = SDL_CreateWindow("SDL event-driven integration", 64, 64, SDL_WINDOW_HIDDEN);
        ASSERT_NE(m_window, nullptr) << SDL_GetError();
        m_windowId = SDL_GetWindowID(m_window);
        ASSERT_NE(m_windowId, 0U) << SDL_GetError();

        SDL_Event pending{};
        while (SDL_PollEvent(&pending))
        {
        }

        m_runtime = std::make_unique<UiRuntime>();
        m_interaction = std::make_unique<systems::InteractionSystem>(*m_runtime);
        m_observation = std::make_unique<EventObservation>(m_eventLoop);
        connectListeners();

        m_eventLoop.registerDefaultHandler(
            [this]
            {
                m_interaction->pollInput();
                m_runtime->dispatcher().update();
                m_observation->recordFrame();
            });
        m_eventLoop.setFrameScheduleMode(FrameScheduleMode::EVENT_DRIVEN);
        m_wakeup = std::make_unique<detail::SdlEventWakeup>(m_eventLoop);
        m_loopThread = std::thread([this] { m_eventLoop.exec(); });

        ASSERT_TRUE(m_observation->waitUntil([this] { return m_observation->frameCount() >= 1U; }, 500ms))
            << "EventLoop 未执行启动帧";
        ASSERT_TRUE(m_observation->waitForIdleWindow(80ms, 1s)) << "SDL 队列未进入空闲状态";
    }

    void TearDown() override
    {
        m_eventLoop.quit();
        if (m_loopThread.joinable())
        {
            m_loopThread.join();
        }
        m_wakeup.reset();
        disconnectListeners();
        m_observation.reset();
        m_interaction.reset();
        m_runtime.reset();
        if (m_window != nullptr)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
        SDL_Quit();
        EXPECT_EQ(SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_EVENTS), 0U);
    }

    void connectListeners()
    {
        auto& dispatcher = m_runtime->dispatcher();
        dispatcher.sink<events::RawPointerMove>().connect<&EventObservation::onPointerMove>(*m_observation);
        dispatcher.sink<events::RawKeyInput>().connect<&EventObservation::onKeyInput>(*m_observation);
        dispatcher.sink<events::WindowPixelSizeChanged>().connect<&EventObservation::onWindowResized>(*m_observation);
        dispatcher.sink<events::WindowExposed>().connect<&EventObservation::onWindowExposed>(*m_observation);
        dispatcher.sink<events::QuitRequested>().connect<&EventObservation::onQuitRequested>(*m_observation);
    }

    void disconnectListeners()
    {
        if (m_runtime == nullptr || m_observation == nullptr)
        {
            return;
        }
        auto& dispatcher = m_runtime->dispatcher();
        dispatcher.sink<events::RawPointerMove>().disconnect<&EventObservation::onPointerMove>(*m_observation);
        dispatcher.sink<events::RawKeyInput>().disconnect<&EventObservation::onKeyInput>(*m_observation);
        dispatcher.sink<events::WindowPixelSizeChanged>().disconnect<&EventObservation::onWindowResized>(*m_observation);
        dispatcher.sink<events::WindowExposed>().disconnect<&EventObservation::onWindowExposed>(*m_observation);
        dispatcher.sink<events::QuitRequested>().disconnect<&EventObservation::onQuitRequested>(*m_observation);
    }

    void pushAndAwait(SDL_Event& event, std::uint32_t beforeFrames, auto count, const char* eventName)
    {
        ASSERT_TRUE(SDL_PushEvent(&event)) << eventName << ": " << SDL_GetError();
        ASSERT_TRUE(m_observation->waitUntil(
            [this, beforeFrames, count]
            { return m_observation->frameCount() > beforeFrames && count() > 0U; },
            500ms))
            << eventName << " 未在有界时间内唤醒默认帧并转换内部事件";
    }

    EventLoop m_eventLoop;
    std::unique_ptr<UiRuntime> m_runtime;
    std::unique_ptr<systems::InteractionSystem> m_interaction;
    std::unique_ptr<EventObservation> m_observation;
    std::unique_ptr<detail::SdlEventWakeup> m_wakeup;
    SDL_Window* m_window = nullptr;
    SDL_WindowID m_windowId = 0;
    std::thread m_loopThread;
};

TEST_F(SdlEventDrivenIntegrationTest, PushEventMatrixAdvancesFramesAndConvertsOnConsumerThread)
{
    const auto pointerBeforeFrames = m_observation->frameCount();
    SDL_Event pointerEvent{};
    pointerEvent.type = SDL_EVENT_MOUSE_MOTION;
    pointerEvent.motion.windowID = m_windowId;
    pointerEvent.motion.x = 12.5F;
    pointerEvent.motion.y = 23.5F;
    pointerEvent.motion.xrel = 2.0F;
    pointerEvent.motion.yrel = -3.0F;
    pushAndAwait(pointerEvent, pointerBeforeFrames, [this] { return m_observation->pointerCount(); }, "mouse motion");

    const auto pointer = m_observation->pointerMove();
    EXPECT_EQ(pointer.position, Vec2(12.5F, 23.5F));
    EXPECT_EQ(pointer.delta, Vec2(2.0F, -3.0F));
    EXPECT_EQ(pointer.windowID, m_windowId);
    EXPECT_EQ(m_observation->pointerThread(), m_observation->consumerThread());

    const auto keyBeforeFrames = m_observation->frameCount();
    SDL_Event keyEvent{};
    keyEvent.type = SDL_EVENT_KEY_DOWN;
    keyEvent.key.windowID = m_windowId;
    keyEvent.key.key = SDLK_A;
    keyEvent.key.mod = SDL_KMOD_SHIFT;
    keyEvent.key.down = true;
    keyEvent.key.repeat = false;
    pushAndAwait(keyEvent, keyBeforeFrames, [this] { return m_observation->keyCount(); }, "key down");

    const auto key = m_observation->keyInput();
    EXPECT_EQ(key.key, static_cast<std::int32_t>(SDLK_A));
    EXPECT_TRUE(key.pressed);
    EXPECT_FALSE(key.repeat);
    EXPECT_EQ(key.modifiers, static_cast<std::uint16_t>(SDL_KMOD_SHIFT));
    EXPECT_EQ(m_observation->keyThread(), m_observation->consumerThread());

    const auto resizeBeforeFrames = m_observation->frameCount();
    SDL_Event resizeEvent{};
    resizeEvent.type = SDL_EVENT_WINDOW_RESIZED;
    resizeEvent.window.windowID = m_windowId;
    resizeEvent.window.data1 = 320;
    resizeEvent.window.data2 = 180;
    pushAndAwait(resizeEvent, resizeBeforeFrames, [this] { return m_observation->resizeCount(); }, "window resized");

    const auto resized = m_observation->windowResized();
    EXPECT_EQ(resized.windowID, m_windowId);
    EXPECT_EQ(resized.width, 320);
    EXPECT_EQ(resized.height, 180);
    EXPECT_EQ(resized.source, events::WindowMetricChangeSource::RESIZED);
    EXPECT_EQ(m_observation->resizeThread(), m_observation->consumerThread());

    const auto exposeBeforeFrames = m_observation->frameCount();
    SDL_Event exposeEvent{};
    exposeEvent.type = SDL_EVENT_WINDOW_EXPOSED;
    exposeEvent.window.windowID = m_windowId;
    pushAndAwait(exposeEvent, exposeBeforeFrames, [this] { return m_observation->exposeCount(); }, "window exposed");

    EXPECT_EQ(m_observation->windowExposed().windowID, m_windowId);
    EXPECT_EQ(m_observation->exposeThread(), m_observation->consumerThread());
}

TEST_F(SdlEventDrivenIntegrationTest, IdleWindowDoesNotContinuouslyProduceFrames)
{
    const auto idleFrames = m_observation->frameCount();
    const bool producedFrame = m_observation->waitUntil(
        [this, idleFrames] { return m_observation->frameCount() != idleFrames; }, 120ms);
    EXPECT_FALSE(producedFrame);
    EXPECT_EQ(m_observation->frameCount(), idleFrames);
}

TEST_F(SdlEventDrivenIntegrationTest, ScheduleModeSwitchesInBothDirectionsWithoutLosingSdlWakeup)
{
    const auto beforeFixedRate = m_observation->frameCount();
    m_eventLoop.setTargetFrameRate(20U);
    m_eventLoop.setFrameScheduleMode(FrameScheduleMode::FIXED_RATE);
    ASSERT_TRUE(m_observation->waitUntil(
        [this, beforeFixedRate] { return m_observation->frameCount() > beforeFixedRate; }, 500ms));

    m_eventLoop.setFrameScheduleMode(FrameScheduleMode::EVENT_DRIVEN);
    ASSERT_TRUE(m_observation->waitForIdleWindow(100ms, 1s));
    const auto eventDrivenFrames = m_observation->frameCount();
    EXPECT_FALSE(m_observation->waitUntil(
        [this, eventDrivenFrames] { return m_observation->frameCount() != eventDrivenFrames; }, 120ms));

    SDL_Event exposed{};
    exposed.type = SDL_EVENT_WINDOW_EXPOSED;
    exposed.window.windowID = m_windowId;
    pushAndAwait(exposed, eventDrivenFrames, [this] { return m_observation->exposeCount(); },
                 "window exposed after mode switches");
    EXPECT_EQ(m_observation->exposeThread(), m_observation->consumerThread());
}

TEST_F(SdlEventDrivenIntegrationTest, PushedQuitWakesConsumerAndStopsLoop)
{
    m_observation->stopOnQuit();
    SDL_Event quitEvent{};
    quitEvent.type = SDL_EVENT_QUIT;
    ASSERT_TRUE(SDL_PushEvent(&quitEvent)) << SDL_GetError();

    ASSERT_TRUE(m_observation->waitUntil([this] { return m_observation->quitCount() > 0U; }, 500ms))
        << "SDL quit 未唤醒消费线程";
    if (m_loopThread.joinable())
    {
        m_loopThread.join();
    }
}

}  // namespace
}  // namespace ui::tests
