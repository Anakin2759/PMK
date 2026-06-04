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
#include <functional>
#include <string>
#include <string_view>

#include "api/Entity.hpp"

namespace ui::event
{

using EventId = std::uint32_t;
inline constexpr EventId INVALID_EVENT_ID = 0;

struct EventPayload
{
    ui::entity source = ui::null_entity;
    ui::entity target = ui::null_entity;
    std::string name;
    std::string text;
    std::int64_t intValue = 0;
    double floatValue = 0.0;
};

using EventCallback = std::move_only_function<void(const EventPayload&)>;

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
