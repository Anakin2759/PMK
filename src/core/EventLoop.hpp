/**
 * ************************************************************************
 *
 * @file EventLoop.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-19
 * @version 0.1
 * @brief ui事件循环管理类
    基于自研无锁 MPSC 队列（utils/EventLoop.hpp）的跨平台事件循环
    维持ui线程的持续运行
    提供启动和停止事件循环的接口
    ui实体的渲染和输入处理对应的系统被提交到该事件循环中执行
    事件循环本身不管理线程
    16ms 节流投递帧回调（自研帧调度器，不使用 ASIO）

    先处理SDL的事件，然后驱动ECS系统更新UI状态，然后处理渲染，
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include "utils/EventLoop.hpp"

namespace ui
{
class EventLoop
{
   public:
    EventLoop();

    ~EventLoop() noexcept;

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    void exec();

    void quit();

    // 直接投递一个“零参数可调用对象”（例如带捕获的 lambda）。
    template <typename Func>
        requires std::invocable<std::decay_t<Func>>
    void invoke(Func&& func)
    {
        m_loop.PostOrThrow([callable = std::forward<Func>(func)]() mutable { std::invoke(std::move(callable)); });
    }

    template <typename Func, typename... Args>
        requires(sizeof...(Args) > 0) && std::invocable<std::decay_t<Func>, std::decay_t<Args>...>
    void invoke(Func&& func, Args&&... args)
    {
        m_loop.PostOrThrow([callable = std::forward<Func>(func), ... capturedArgs = std::forward<Args>(args)]() mutable
                           { std::invoke(std::move(callable), std::move(capturedArgs)...); });
    }

    // 注册默认处理器（无参数版本）
    template <typename Func>
        requires std::invocable<std::decay_t<Func>>
    void registerDefaultHandler(Func&& func)
    {
        m_defaultHandler = [callable = std::forward<Func>(func)]() mutable { std::invoke(std::move(callable)); };
    }

    // 注册默认处理器（带参数版本）
    template <typename Func, typename... Args>
        requires(sizeof...(Args) > 0) && std::invocable<std::decay_t<Func>, std::decay_t<Args>...>
    void registerDefaultHandler(Func&& func, Args&&... args)
    {
        m_defaultHandler = [callable = std::forward<Func>(func), ... capturedArgs = std::forward<Args>(args)]() mutable
        { std::invoke(std::move(callable), std::move(capturedArgs)...); };
    }

   private:
    void startFrameScheduler();
    void stopFrameScheduler() noexcept;
    void postDefaultHandler();

    utils::EventLoop m_loop;
    std::jthread m_frameScheduler;
    std::atomic<bool> m_running;
    std::move_only_function<void()> m_defaultHandler;
};
}  // namespace ui