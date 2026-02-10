/**
 * ************************************************************************
 *
 * @file TaskChain.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-25
 * @version 0.1
 * @brief  ui每帧执行的任务链封装

  - 定义渲染任务和输入处理任务

   -基本上是固定流程，所以暂时就直接写仿函数

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include "../common/Events.hpp"
#include "../singleton/Dispatcher.hpp"
#include "../singleton/Logger.hpp"
#include "../systems/InteractionSystem.hpp"
namespace ui::tasks
{

// --- 1. 基础 Concept 与 辅助工具 ---

template <typename T>
concept IsTask = requires { typename std::remove_cvref_t<T>::is_task_tag; };

// --- 2. 核心组合器：广播模式 ---

template <typename F, typename G>
struct Combined
{
    using is_task_tag = void;
    F first;
    G second;

    // C++23 deducing this: 处理任务对象的生命周期（左值拷贝/右值移动）
    template <typename Self, typename... Args>
    decltype(auto) operator()(this Self&& self, Args&&... args)
    {
        // 广播模式：每个任务都接收相同的原始参数
        std::invoke(std::forward<Self>(self).first, args...);
        return std::invoke(std::forward<Self>(self).second, std::forward<Args>(args)...);
    }
};

// --- 3. 参数种子节点 (The Wrapper) ---

template <typename... StoredArgs>
struct BoundContext
{
    using is_task_tag = void;
    std::tuple<StoredArgs...> args;

    // 当 Context 遇到第一个任务时，将参数绑定
    template <typename Self, typename Task>
    auto operator|(this Self&& self, Task&& next)
    {
        // 返回一个闭包，它捕获了参数，并且它的 operator() 不需要再传参
        return [s = std::forward<Self>(self).args, t = std::forward<Task>(next)](auto&&... extra_args) mutable
        {
            // 如果执行时还传了新参数，合并它们，否则只传 Bound 的参数
            return std::apply(t, s);
        };
    }
};

// --- 4. CPO 实现 ---

namespace _pipe_cpo
{
struct _fn
{
    // 处理任务之间的拼接 (Task | Task)
    template <IsTask F, IsTask G>
    constexpr auto operator()(F&& f, G&& g) const
    {
        return Combined<std::decay_t<F>, std::decay_t<G>>{std::forward<F>(f), std::forward<G>(g)};
    }
};
} // namespace _pipe_cpo
inline constexpr _pipe_cpo::_fn pipe{};

// --- 5. 运算符重载 ---

// 情况 A: 任务 | 任务
template <IsTask F, IsTask G>
auto operator|(F&& f, G&& g)
{
    return pipe(std::forward<F>(f), std::forward<G>(g));
}

// 情况 B: Context | 任务 (由 BoundContext 内部实现，此处仅为语义辅助)
template <typename... T>
auto WrapArgs(T&&... args)
{
    return BoundContext<std::decay_t<T>...>{std::make_tuple(std::forward<T>(args)...)};
}

// --- 6. 具体任务类实现 ---

struct RenderTask
{
    using is_task_tag = void;
    uint32_t m_remainingTime = 0;
    uint32_t m_delayTime = 16;

    void operator()(uint32_t delta)
    {
        if (m_remainingTime > delta)
        {
            m_remainingTime -= delta;
            return;
        }
        m_remainingTime = m_delayTime;
        Dispatcher::Trigger<ui::events::UpdateLayout>();
        Dispatcher::Trigger<ui::events::UpdateRendering>();
        Dispatcher::Trigger<ui::events::EndFrame>(); // 帧结束时批量应用状态更新
    }
};

struct InputTask
{
    using is_task_tag = void;
    uint32_t m_remainingTime = 0;
    uint32_t m_delayTime = 32;

    void operator()(uint32_t delta)
    {
        if (m_remainingTime > delta)
        {
            m_remainingTime -= delta;
            return;
        }
        m_remainingTime = m_delayTime;
        ui::systems::InteractionSystem::SDLEvent();
    }
};

struct QueuedTask
{
    using is_task_tag = void;

    void operator()(uint32_t delta)
    {
        auto& frameContext = Registry::ctx().get<globalcontext::FrameContext>();
        frameContext.intervalMs = delta;
        frameContext.frameSlot = (frameContext.frameSlot + 1) % 2;
        Dispatcher::Trigger<ui::events::UpdateTimer>();
        Dispatcher::Update();
    }
};

} // namespace ui::tasks