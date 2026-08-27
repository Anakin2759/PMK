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

EventId RegisterEvent(UiRuntime& runtime, std::string_view name)
{
    return ui::detail::event_bridge::RegisterEvent(runtime, name);
}

bool IsEventRegistered(UiRuntime& runtime, EventId eventId)
{
    return ui::detail::event_bridge::IsEventRegistered(runtime, eventId);
}

bool IsEventRegistered(UiRuntime& runtime, std::string_view name)
{
    return ui::detail::event_bridge::IsEventRegistered(runtime, name);
}

EventConnection On(UiRuntime& runtime, EventId eventId, EventCallback callback)
{
    auto connection = ui::detail::event_bridge::Connect(runtime, eventId, std::move(callback));
    return EventConnection{std::move(connection.domain), connection.token};
}

EventConnection On(UiRuntime& runtime, std::string_view name, EventCallback callback)
{
    auto connection = ui::detail::event_bridge::Connect(runtime, name, std::move(callback));
    return EventConnection{std::move(connection.domain), connection.token};
}

void Trigger(UiRuntime& runtime, EventId eventId, EventPayload payload)
{
    ui::detail::event_bridge::Trigger(runtime, eventId, std::move(payload));
}

void Trigger(UiRuntime& runtime, std::string_view name, EventPayload payload)
{
    ui::detail::event_bridge::Trigger(runtime, name, std::move(payload));
}

void Enqueue(UiRuntime& runtime, EventId eventId, EventPayload payload)
{
    ui::detail::event_bridge::Enqueue(runtime, eventId, std::move(payload));
}

void Enqueue(UiRuntime& runtime, std::string_view name, EventPayload payload)
{
    ui::detail::event_bridge::Enqueue(runtime, name, std::move(payload));
}

void DispatchQueued(UiRuntime& runtime)
{
    ui::detail::event_bridge::DispatchQueued(runtime);
}

}  // namespace ui::event
