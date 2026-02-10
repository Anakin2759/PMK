/**
 * ************************************************************************
 *
 * @file Dispatcher.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.2
 * @brief ui单例事件分发器
 *
 * 支持两种事件分发模式：
 * 1. 紧急事件 (trigger) - 立即执行，同步调用所有监听器
 * 2. 缓冲区事件 (enqueue) - 加入队列，在事件循环的 update() 中批量处理
 *
 * 使用指南：
 * - trigger: 用于需要立即响应的事件，如 QuitRequested, UpdateRendering
 * - enqueue: 用于可以延迟处理的事件，如  WindowGraphicsContextSetEvent
 * - update: 在事件循环每帧调用，处理所有缓冲区事件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "SingletonBase.hpp"
#include "../traits/EventTraits.hpp"
namespace ui
{
class Dispatcher : public SingletonBase<Dispatcher>
{
    friend class SingletonBase<Dispatcher>;

public:
    template <traits::Events Event>
    static void Trigger(Event&& event = {})
    {
        getInstance().m_dispatcher.trigger(std::forward<decltype(event)>(event));
    }
    template <traits::Events Event>
    static void Enqueue(Event&& event = {})
    {
        getInstance().m_dispatcher.enqueue(std::forward<decltype(event)>(event));
    }

    static void Update() { getInstance().m_dispatcher.update(); }

    template <traits::Events Event>
    static void Update()
    {
        getInstance().m_dispatcher.update<Event>();
    }

    template <traits::Events Type>
    static auto Sink()
    {
        return getInstance().m_dispatcher.sink<Type>();
    }

private:
    Dispatcher() = default;

    entt::dispatcher m_dispatcher;
};
} // namespace ui
