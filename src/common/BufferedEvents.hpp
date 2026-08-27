/**
 * @file BufferedEvents.hpp
 * @brief 内部缓冲事件的唯一有序目录。
 *
 * 类型顺序就是 FrameTick 的派发顺序。任何通过 Dispatcher::enqueue()
 * 入队的内部事件都必须先登记在此；公开 EventBridge 使用独立的动态事件队列。
 */

#pragma once

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>

#include "Events.hpp"

namespace ui::events
{
namespace detail
{
template <typename... Events>
struct BufferedEventList
{
    static constexpr std::size_t SIZE = sizeof...(Events);
};

template <typename Event, typename List>
struct IsInBufferedEventList;

template <typename Event, typename... Events>
struct IsInBufferedEventList<Event, BufferedEventList<Events...>>
    : std::bool_constant<(std::same_as<std::remove_cvref_t<Event>, Events> || ...)>
{
};
}  // namespace detail

/**
 * @brief 内部缓冲事件目录；顺序表达平台 → Raw → Hit → Hover 的依赖链。
 */
using InternalBufferedEvents = detail::BufferedEventList<
    QuitRequested, CloseWindow, WindowPixelSizeChanged, WindowMoved, WindowExposed,
    RawPointerMove, RawPointerButton, RawPointerWheel,
    HitPointerMove, HitPointerButton, HitPointerWheel, UnhoverEvent, HoverEvent>;

template <typename Event>
inline constexpr bool IS_INTERNAL_BUFFERED_EVENT =
    detail::IsInBufferedEventList<Event, InternalBufferedEvents>::value;

inline constexpr std::size_t INTERNAL_BUFFERED_EVENT_COUNT = InternalBufferedEvents::SIZE;

/**
 * @brief 为了避免重复代码，提供一个模板函数来遍历所有内部缓冲事件类型。
 * @tparam Function {template <typename Event> void Function()}
 * @param function {comment}
 */

template <typename Function, typename... Events>
constexpr void ForEachInternalBufferedEventImpl(Function&& function, detail::BufferedEventList<Events...>)
{
    (std::forward<Function>(function).template operator()<Events>(), ...); //
}

template <typename Function>
constexpr void ForEachInternalBufferedEvent(Function&& function)
{
    ForEachInternalBufferedEventImpl(std::forward<Function>(function), InternalBufferedEvents{});
}
}  // namespace ui::events