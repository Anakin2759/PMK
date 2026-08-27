/**
 * ************************************************************************
 *
 * @file EventLoop.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-19
 * @version 0.1
 * @brief ui事件循环管理类
    基于自研无锁 MPSC 队列（utils/EventLoop.hpp）的跨平台事件循环
    维持ui线程的持续运行
    提供启动和停止事件循环的接口
    ui实体的渲染和输入处理对应的系统被提交到该事件循环中执行
    事件循环本身不管理线程
    默认 60fps 节流投递帧回调（自研帧调度器，不使用 ASIO）

    先处理SDL的事件，然后驱动ECS系统更新UI状态，然后处理渲染，
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <concepts>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include "utils/EventLoop.hpp"

namespace ui
{

/// 帧调度模式（P1-4 双模式设计）
/// - FIXED_RATE：固定帧率节流（默认，向后兼容，默认 60fps）
/// - EVENT_DRIVEN：事件驱动，空闲时 CPU 归零，任务/事件到达即唤醒
enum class FrameScheduleMode : std::uint8_t
{
    FIXED_RATE,
    EVENT_DRIVEN
};

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

    /// 设置 FIXED_RATE 模式的目标帧率；0 会规范化为默认 60fps。
    /// 若未来需要不锁帧，应增加显式调度模式，不复用数值哨兵。
    void setTargetFrameRate(std::uint32_t fps);

    [[nodiscard]] std::uint32_t targetFrameRate() const noexcept;

    // 帧调度模式切换（运行中可用）
    void setFrameScheduleMode(FrameScheduleMode mode);

    [[nodiscard]] FrameScheduleMode frameScheduleMode() const noexcept;

    /// 通知调度器有外部事件到达。该入口仅执行原子递增和条件变量通知，
    /// 可由 SDL event watch 等非 UI 线程回调安全调用。
    void notifyExternalEvent() noexcept;

    // 直接投递一个“零参数可调用对象”（例如带捕获的 lambda）。
    template <typename Func>
        requires std::invocable<std::decay_t<Func>>
    void invoke(Func&& func)
    {
        m_loop.PostOrThrow([callable = std::forward<Func>(func)]() mutable { std::invoke(std::move(callable)); });
        notifyExternalEvent();
    }

    template <typename Func, typename... Args>
        requires(sizeof...(Args) > 0) && std::invocable<std::decay_t<Func>, std::decay_t<Args>...>
    void invoke(Func&& func, Args&&... args)
    {
        m_loop.PostOrThrow([callable = std::forward<Func>(func), ... capturedArgs = std::forward<Args>(args)]() mutable
                           { std::invoke(std::move(callable), std::move(capturedArgs)...); });
        notifyExternalEvent();
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
    void frameSchedulerLoop(std::stop_token stopToken);

    utils::EventLoop m_loop;
    std::jthread m_frameScheduler;
    std::atomic<bool> m_running;
    std::move_only_function<void()> m_defaultHandler;

    // P1-4 双模式调度状态
    // 默认固定帧率；setter 保证存储值始终非零。
    static constexpr std::uint32_t kDefaultTargetFrameRate = 60;
    std::atomic<std::uint32_t> m_targetFrameRate{kDefaultTargetFrameRate};
    std::atomic<FrameScheduleMode> m_scheduleMode{FrameScheduleMode::FIXED_RATE};
    // 唤醒纪元：由 invoke、外部事件、模式切换和 quit 递增，等待谓词检测纪元变化，
    // 消除「notify 先于 wait 注册」的丢失窗口（与 utils::EventLoop 方案 B 同原理）。
    std::atomic<std::uint64_t> m_scheduleEpoch{0};
    std::mutex m_scheduleMutex;
    std::condition_variable m_scheduleCv;
};
}  // namespace ui