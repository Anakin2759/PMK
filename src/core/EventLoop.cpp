/**
 * EventLoop implementation
 */

#include "EventLoop.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <thread>
namespace ui
{
namespace
{
// 默认固定帧间隔（16ms ≈ 60fps），兼容历史行为。
// 通过 setTargetFrameRate 可在运行期覆盖，帧率优先级高于固定间隔。
constexpr auto kFrameInterval = std::chrono::milliseconds(16);

void WriteStderr(const char* text) noexcept
{
    if (text == nullptr)
    {
        return;
    }

    const auto textSize = std::strlen(text);
    if (std::fwrite(text, 1U, textSize, stderr) != textSize)
    {
        std::clearerr(stderr);
    }
}
}  // namespace

EventLoop::EventLoop() : m_running(false)
{
}

EventLoop::~EventLoop() noexcept
{
    try
    {
        quit();
    }
    catch (const std::exception& exception)
    {
        WriteStderr("[EventLoop] destructor cleanup failed: ");
        WriteStderr(exception.what());
        WriteStderr("\n");
    }
    catch (...)
    {
        WriteStderr("[EventLoop] destructor cleanup failed with unknown exception\n");
    }
}

void EventLoop::setTargetFrameRate(std::uint32_t fps)
{
    m_targetFrameRate.store(fps, std::memory_order_release);
    // 帧率变更后立即唤醒调度器重新计算间隔
    m_scheduleCv.notify_all();
}

std::uint32_t EventLoop::targetFrameRate() const noexcept
{
    return m_targetFrameRate.load(std::memory_order_acquire);
}

void EventLoop::setFrameScheduleMode(FrameScheduleMode mode)
{
    m_scheduleMode.store(mode, std::memory_order_release);
    m_scheduleCv.notify_all();
}

FrameScheduleMode EventLoop::frameScheduleMode() const noexcept
{
    return m_scheduleMode.load(std::memory_order_acquire);
}

void EventLoop::startFrameScheduler()
{
    m_frameScheduler = std::jthread([this](std::stop_token stopToken) { frameSchedulerLoop(std::move(stopToken)); });
}

void EventLoop::frameSchedulerLoop(std::stop_token stopToken)
{
    // 双模式帧调度（P1-4）：
    // - FIXED_RATE（默认）：以目标帧率精确节流（sleep_until 避免累积漂移），
    //   兼容历史 16ms 行为；调用 setTargetFrameRate 可动态改变帧率。
    // - EVENT_DRIVEN：空闲时挂起，仅在退出或外部投递（invoke 唤醒调度器）时
    //   投递默认处理器，实现帧空闲 CPU 归零；由上层负责保证动画/定时器仍被驱动。
    auto nextFrameTime = std::chrono::steady_clock::now();

    while (!stopToken.stop_requested() && m_running.load())
    {
        const auto mode = m_scheduleMode.load(std::memory_order_acquire);

        if (mode == FrameScheduleMode::EVENT_DRIVEN)
        {
            // 事件驱动：空闲挂起，等待纪元变化（invoke/quit 递增）或停止。
            // 谓词检查纪元而非仅停止条件，保证 notify 先于 wait 注册时也能立即返回。
            const auto epoch = m_scheduleEpoch.load(std::memory_order_acquire);
            std::unique_lock lock(m_scheduleMutex);
            m_scheduleCv.wait(lock, [this, &stopToken, epoch] {
                return stopToken.stop_requested() || !m_running.load() ||
                       m_scheduleEpoch.load(std::memory_order_acquire) != epoch;
            });
        }
        else
        {
            // 固定帧率：计算下一帧时间并精确等待
            const auto fps = m_targetFrameRate.load(std::memory_order_acquire);
            const auto interval = fps > 0 ? std::chrono::microseconds(1000000 / fps) : kFrameInterval;
            nextFrameTime = std::max(nextFrameTime + interval, std::chrono::steady_clock::now());

            std::unique_lock lock(m_scheduleMutex);
            m_scheduleCv.wait_until(lock, nextFrameTime, [this, fps, &stopToken] {
                return stopToken.stop_requested() || !m_running.load() ||
                       m_scheduleMode.load(std::memory_order_acquire) != FrameScheduleMode::FIXED_RATE ||
                       m_targetFrameRate.load(std::memory_order_acquire) != fps;
            });

            if (m_scheduleMode.load(std::memory_order_acquire) != FrameScheduleMode::FIXED_RATE)
            {
                // 模式被切换到事件驱动，立即进入下一轮
                continue;
            }
        }

        if (stopToken.stop_requested() || !m_running.load())
        {
            break;
        }

        postDefaultHandler();
    }
}

void EventLoop::stopFrameScheduler() noexcept
{
    if (m_frameScheduler.joinable())
    {
        m_frameScheduler.request_stop();
        m_scheduleCv.notify_all();
        m_frameScheduler.join();
    }
}

void EventLoop::WakeSchedulerIfEventDriven() noexcept
{
    if (m_scheduleMode.load(std::memory_order_acquire) == FrameScheduleMode::EVENT_DRIVEN)
    {
        m_scheduleEpoch.fetch_add(1, std::memory_order_release);
        m_scheduleCv.notify_all();
    }
}

void EventLoop::postDefaultHandler()
{
    if (!m_defaultHandler)
    {
        return;
    }

    (void)m_loop.Post(
        [this]
        {
            if (m_running.load() && m_defaultHandler)
            {
                m_defaultHandler();
            }
        });
}

void EventLoop::exec()
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
    {
        return;
    }

    m_loop.Reset();
    postDefaultHandler();
    startFrameScheduler();
    (void)m_loop.Exec();
    stopFrameScheduler();
    m_running.store(false);
}

void EventLoop::quit()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    m_scheduleEpoch.fetch_add(1, std::memory_order_release);
    m_scheduleCv.notify_all();
    m_loop.Quit(0, true);
}

}  // namespace ui
