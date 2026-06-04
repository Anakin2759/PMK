/**
 * ************************************************************************
 *
 * @file EventBridge.hpp
 * @brief 公开自定义事件 API 的内部桥接实现声明。
 *
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <string_view>

#include "api/Event.hpp"

namespace ui::detail::event_bridge
{

[[nodiscard]] ui::event::EventId RegisterEvent(std::string_view name);
[[nodiscard]] bool IsEventRegistered(ui::event::EventId eventId);
[[nodiscard]] bool IsEventRegistered(std::string_view name);

[[nodiscard]] std::uint64_t Connect(ui::event::EventId eventId, ui::event::EventCallback callback);
[[nodiscard]] std::uint64_t Connect(std::string_view name, ui::event::EventCallback callback);
void Disconnect(std::uint64_t token) noexcept;
[[nodiscard]] bool Connected(std::uint64_t token) noexcept;

void Trigger(ui::event::EventId eventId, ui::event::EventPayload payload);
void Trigger(std::string_view name, ui::event::EventPayload payload);

void Enqueue(ui::event::EventId eventId, ui::event::EventPayload payload);
void Enqueue(std::string_view name, ui::event::EventPayload payload);

void DispatchQueued();

} // namespace ui::detail::event_bridge
