/**
 * EventLoop implementation
 */

#include "EventLoop.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
namespace ui
{
namespace
{
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

void EventLoop::startFrameScheduler()
{
    m_frameScheduler = std::jthread(
        [this](std::stop_token stopToken)
        {
            while (!stopToken.stop_requested() && m_running.load())
            {
                std::this_thread::sleep_for(kFrameInterval);

                if (stopToken.stop_requested() || !m_running.load())
                {
                    break;
                }

                postDefaultHandler();
            }
        });
}

void EventLoop::stopFrameScheduler() noexcept
{
    if (m_frameScheduler.joinable())
    {
        m_frameScheduler.request_stop();
        m_frameScheduler.join();
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

    m_loop.Quit(0, true);
}

}  // namespace ui
