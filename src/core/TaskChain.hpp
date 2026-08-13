/**
 * ************************************************************************
 *
 * @file TaskChain.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-25
 * @version 0.1
 * @brief UI 固定帧管线与任务组合工具
 *
 * FrameTick 是生产帧的唯一入口，固定执行输入、队列、逻辑、布局、渲染和帧尾阶段。
 * UpdateLayout / UpdateRendering 没有即时旁路白名单。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include "common/Events.hpp"
#include "common/GlobalContext.hpp"
#include "core/UiRuntime.hpp"
#include "core/UiRuntimeScope.hpp"
#include "helper/Helper.hpp"
#include "SystemManager.hpp"

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
        return [storedArgs = std::forward<Self>(self).args, nextTask = std::forward<Task>(next)]() mutable
        { return std::apply(nextTask, storedArgs); };
    }
};

// --- 4. CPO 实现 ---

namespace pipe_cpo
{
struct PipeFn
{
    // 处理任务之间的拼接 (Task | Task)
    template <IsTask F, IsTask G>
    constexpr auto operator()(F&& firstTask, G&& secondTask) const
    {
        return Combined<std::decay_t<F>, std::decay_t<G>>{std::forward<F>(firstTask), std::forward<G>(secondTask)};
    }
};
}  // namespace pipe_cpo
inline constexpr pipe_cpo::PipeFn PIPE_COMPOSER{};

// --- 5. 运算符重载 ---

// 情况 A: 任务 | 任务
template <IsTask F, IsTask G>
auto operator|(F&& firstTask, G&& secondTask)
{
    return PIPE_COMPOSER(std::forward<F>(firstTask), std::forward<G>(secondTask));
}

// 情况 B: Context | 任务 (由 BoundContext 内部实现，此处仅为语义辅助)
template <typename... T>
auto WrapArgs(T&&... args)
{
    return BoundContext<std::decay_t<T>...>{std::make_tuple(std::forward<T>(args)...)};
}

// --- 6. 唯一固定帧入口 ---

class FrameTick
{
   public:
    FrameTick(UiRuntime& runtime, SystemManager& systems) noexcept : m_runtime(&runtime), m_systems(&systems)
    {
    }

    void operator()(uint32_t delta) const
    {
        UiRuntimeScope const runtimeScope{*m_runtime};
        auto& frameContext = m_runtime->registry().template getOrEmplaceInCtx<globalcontext::FrameContext>();
        struct IdleStageGuard
        {
            globalcontext::FrameContext& frame;
            ~IdleStageGuard()
            {
                frame.stage = globalcontext::FrameStage::IDLE;
            }
        } const idleStageGuard{frameContext};

        frameContext.stage = globalcontext::FrameStage::BEGIN_FRAME;
        ++frameContext.frameNumber;
        frameContext.intervalMs = delta;
        frameContext.layoutUpdateCount = 0;
        frameContext.renderUpdateCount = 0;
        frameContext.frameSlot = (frameContext.frameSlot + 1) % 2;

        frameContext.stage = globalcontext::FrameStage::POLL_INPUT;
        m_systems->pollInput();

        auto& disp = m_runtime->dispatcher();
        frameContext.stage = globalcontext::FrameStage::DISPATCH_INTERNAL_QUEUED;
        disp.update();

        frameContext.stage = globalcontext::FrameStage::LOGIC;
        disp.trigger<ui::events::UpdateTimer>();
        disp.trigger<events::TickKeyRepeat>();
        disp.trigger<ui::events::UpdateEvent>();

        frameContext.stage = globalcontext::FrameStage::DISPATCH_PUBLIC_QUEUED;
        detail::event_bridge::DispatchQueued();

        frameContext.stage = globalcontext::FrameStage::LAYOUT;
        disp.trigger<ui::events::UpdateLayout>();

        frameContext.stage = globalcontext::FrameStage::RENDER;
        disp.trigger<ui::events::UpdateRendering>();

        frameContext.stage = globalcontext::FrameStage::END_FRAME;
        disp.trigger<ui::events::EndFrame>();
    }

   private:
    UiRuntime* m_runtime;
    SystemManager* m_systems;
};

}  // namespace ui::tasks