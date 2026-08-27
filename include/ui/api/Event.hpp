/**
 * @file Event.hpp
 * @brief 公开自定义事件 API，不暴露 EnTT / Dispatcher 实现细节。
 */
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "ui/api/Entity.hpp"

namespace ui
{
class UiRuntime;
}

namespace ui::detail::event_bridge
{
struct EventDomain;
}

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
    EventConnection(std::weak_ptr<ui::detail::event_bridge::EventDomain> domain, std::uint64_t token) noexcept;
    EventConnection(const EventConnection&) = delete;
    EventConnection& operator=(const EventConnection&) = delete;
    EventConnection(EventConnection&& other) noexcept;
    EventConnection& operator=(EventConnection&& other) noexcept;
    ~EventConnection();

    void Disconnect() noexcept;
    [[nodiscard]] bool Connected() const noexcept;

   private:
    std::weak_ptr<ui::detail::event_bridge::EventDomain> m_domain;
    std::uint64_t m_token = 0;
};

[[nodiscard]] EventId RegisterEvent(UiRuntime& runtime, std::string_view name);
[[nodiscard]] bool IsEventRegistered(UiRuntime& runtime, EventId eventId);
[[nodiscard]] bool IsEventRegistered(UiRuntime& runtime, std::string_view name);
[[nodiscard]] EventConnection On(UiRuntime& runtime, EventId eventId, EventCallback callback);
[[nodiscard]] EventConnection On(UiRuntime& runtime, std::string_view name, EventCallback callback);
void Trigger(UiRuntime& runtime, EventId eventId, EventPayload payload = {});
void Trigger(UiRuntime& runtime, std::string_view name, EventPayload payload = {});
void Enqueue(UiRuntime& runtime, EventId eventId, EventPayload payload = {});
void Enqueue(UiRuntime& runtime, std::string_view name, EventPayload payload = {});
void DispatchQueued(UiRuntime& runtime);
/// 入队并在所属 Runtime 的下一次 QueuedTask 阶段自动派发。
/// 入队并在所属 Runtime 的下一次 QueuedTask 阶段自动派发。
/// 立即清空当前 Runtime 的公开事件队列；保留供测试和显式调度使用。
}  // namespace ui::event
