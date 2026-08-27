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
    const auto normalizedFps = fps == 0U ? kDefaultTargetFrameRate : fps;
    m_targetFrameRate.store(normalizedFps, std::memory_order_release);
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
    notifyExternalEvent();
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
    //   默认 60fps；调用 setTargetFrameRate 可动态改变帧率，0 恢复默认值。
    // - EVENT_DRIVEN：空闲时挂起，仅在退出或外部投递（invoke 唤醒调度器）时
    //   投递默认处理器，实现帧空闲 CPU 归零；由上层负责保证动画/定时器仍被驱动。
    auto nextFrameTime = std::chrono::steady_clock::now();
    auto observedEpoch = m_scheduleEpoch.load(std::memory_order_acquire);

    while (!stopToken.stop_requested() && m_running.load())
    {
        const auto mode = m_scheduleMode.load(std::memory_order_acquire);

        if (mode == FrameScheduleMode::EVENT_DRIVEN)
        {
            // 事件驱动：空闲挂起，等待纪元、模式变化或停止。
            // 谓词检查纪元而非仅停止条件，保证 notify 先于 wait 注册时也能立即返回。
            std::unique_lock lock(m_scheduleMutex);
            m_scheduleCv.wait(lock, [this, &stopToken, observedEpoch] {
                return stopToken.stop_requested() || !m_running.load() ||
                       m_scheduleMode.load(std::memory_order_acquire) != FrameScheduleMode::EVENT_DRIVEN ||
                       m_scheduleEpoch.load(std::memory_order_acquire) != observedEpoch;
            });
            observedEpoch = m_scheduleEpoch.load(std::memory_order_acquire);
        }
        else
        {
            // 固定帧率：计算下一帧时间并精确等待
            const auto fps = m_targetFrameRate.load(std::memory_order_acquire);
            const auto interval = std::chrono::microseconds(1000000 / fps);
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

void EventLoop::notifyExternalEvent() noexcept
{
    m_scheduleEpoch.fetch_add(1, std::memory_order_release);
    m_scheduleCv.notify_all();
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
