/**
 * ************************************************************************
 *
 * @file Event.hpp
 * @brief 公开自定义事件 API，不暴露 EnTT / Dispatcher 实现细节。
 *
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <string_view>

#include "common/CustomEvent.hpp"

namespace ui::event
{

class EventConnection
{
public:
    EventConnection() noexcept = default;
    explicit EventConnection(std::uint64_t token) noexcept;
    EventConnection(const EventConnection&) = delete;
    EventConnection& operator=(const EventConnection&) = delete;
    EventConnection(EventConnection&& other) noexcept;
    EventConnection& operator=(EventConnection&& other) noexcept;
    ~EventConnection();

    void Disconnect() noexcept;
    [[nodiscard]] bool Connected() const noexcept;

private:
    std::uint64_t m_token = 0;
};

[[nodiscard]] EventId RegisterEvent(std::string_view name);
[[nodiscard]] bool IsEventRegistered(EventId eventId);
[[nodiscard]] bool IsEventRegistered(std::string_view name);

[[nodiscard]] EventConnection On(EventId eventId, EventCallback callback);
[[nodiscard]] EventConnection On(std::string_view name, EventCallback callback);

void Trigger(EventId eventId, EventPayload payload = {});
void Trigger(std::string_view name, EventPayload payload = {});

void Enqueue(EventId eventId, EventPayload payload = {});
void Enqueue(std::string_view name, EventPayload payload = {});

void DispatchQueued();

} // namespace ui::event
