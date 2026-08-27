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

#include "core/UiRuntime.hpp"
#include "helper/Helper.hpp"

namespace ui::event
{

EventConnection::EventConnection(std::weak_ptr<ui::detail::event_bridge::EventDomain> domain,
                                 std::uint64_t token) noexcept
    : m_domain(std::move(domain)), m_token(token)
{
}

EventConnection::EventConnection(EventConnection&& other) noexcept
    : m_domain(std::move(other.m_domain)), m_token(std::exchange(other.m_token, 0))
{
}

EventConnection& EventConnection::operator=(EventConnection&& other) noexcept
{
    if (this != &other)
    {
        Disconnect();
        m_domain = std::move(other.m_domain);
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

    if (auto domain = m_domain.lock())
    {
        ui::detail::event_bridge::Disconnect(*domain, m_token);
    }
    m_token = 0;
    m_domain.reset();
}

bool EventConnection::Connected() const noexcept
{
    if (m_token == 0)
    {
        return false;
    }
    auto domain = m_domain.lock();
    return domain != nullptr && ui::detail::event_bridge::Connected(*domain, m_token);
}

EventId RegisterEvent(std::string_view name)
{
    return ui::detail::event_bridge::RegisterEvent(UiRuntime::current(), name);
}

bool IsEventRegistered(EventId eventId)
{
    return ui::detail::event_bridge::IsEventRegistered(UiRuntime::current(), eventId);
}

bool IsEventRegistered(std::string_view name)
{
    return ui::detail::event_bridge::IsEventRegistered(UiRuntime::current(), name);
}

EventConnection On(EventId eventId, EventCallback callback)
{
    auto connection = ui::detail::event_bridge::Connect(UiRuntime::current(), eventId, std::move(callback));
    return EventConnection{std::move(connection.domain), connection.token};
}

EventConnection On(std::string_view name, EventCallback callback)
{
    auto connection = ui::detail::event_bridge::Connect(UiRuntime::current(), name, std::move(callback));
    return EventConnection{std::move(connection.domain), connection.token};
}

void Trigger(EventId eventId, EventPayload payload)
{
    ui::detail::event_bridge::Trigger(UiRuntime::current(), eventId, std::move(payload));
}

void Trigger(std::string_view name, EventPayload payload)
{
    ui::detail::event_bridge::Trigger(UiRuntime::current(), name, std::move(payload));
}

void Enqueue(EventId eventId, EventPayload payload)
{
    ui::detail::event_bridge::Enqueue(UiRuntime::current(), eventId, std::move(payload));
}

void Enqueue(std::string_view name, EventPayload payload)
{
    ui::detail::event_bridge::Enqueue(UiRuntime::current(), name, std::move(payload));
}

void DispatchQueued()
{
    ui::detail::event_bridge::DispatchQueued(UiRuntime::current());
}

}  // namespace ui::event
