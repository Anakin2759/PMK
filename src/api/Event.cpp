/**
 * ************************************************************************
 *
 * @file Event.cpp
 * @brief 公开自定义事件 API 实现。
 *
 * ************************************************************************
 */
#include "ui/api/Event.hpp"

#include <utility>

#include "helper/Helper.hpp"

namespace ui::event
{

EventConnection::EventConnection(std::uint64_t token) noexcept : m_token(token) {}

EventConnection::EventConnection(EventConnection&& other) noexcept : m_token(std::exchange(other.m_token, 0)) {}

EventConnection& EventConnection::operator=(EventConnection&& other) noexcept
{
    if (this != &other)
    {
        Disconnect();
        m_token = std::exchange(other.m_token, 0);
    }
    return *this;
}

EventConnection::~EventConnection()
{
    Disconnect();
}

void EventConnection::Disconnect() noexcept
{
    if (m_token == 0)
    {
        return;
    }

    detail::event_bridge::Disconnect(m_token);
    m_token = 0;
}

bool EventConnection::Connected() const noexcept
{
    return m_token != 0 && detail::event_bridge::Connected(m_token);
}

EventId RegisterEvent(std::string_view name)
{
    return detail::event_bridge::RegisterEvent(name);
}

bool IsEventRegistered(EventId eventId)
{
    return detail::event_bridge::IsEventRegistered(eventId);
}

bool IsEventRegistered(std::string_view name)
{
    return detail::event_bridge::IsEventRegistered(name);
}

EventConnection On(EventId eventId, EventCallback callback)
{
    return EventConnection{detail::event_bridge::Connect(eventId, std::move(callback))};
}

EventConnection On(std::string_view name, EventCallback callback)
{
    return EventConnection{detail::event_bridge::Connect(name, std::move(callback))};
}

void Trigger(EventId eventId, EventPayload payload)
{
    detail::event_bridge::Trigger(eventId, std::move(payload));
}

void Trigger(std::string_view name, EventPayload payload)
{
    detail::event_bridge::Trigger(name, std::move(payload));
}

void Enqueue(EventId eventId, EventPayload payload)
{
    detail::event_bridge::Enqueue(eventId, std::move(payload));
}

void Enqueue(std::string_view name, EventPayload payload)
{
    detail::event_bridge::Enqueue(name, std::move(payload));
}

void DispatchQueued()
{
    detail::event_bridge::DispatchQueued();
}

} // namespace ui::event
