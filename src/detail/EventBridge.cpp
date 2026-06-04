/**
 * ************************************************************************
 *
 * @file EventBridge.cpp
 * @brief 公开自定义事件 API 的运行时隔离桥接实现。
 *
 * ************************************************************************
 */
#include "detail/EventBridge.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/RuntimeFacade.hpp"

namespace ui::detail::event_bridge
{
namespace
{

struct CallbackSlot
{
    std::uint64_t token = 0;
    std::shared_ptr<ui::event::EventCallback> callback;
    bool connected = true;
};

struct QueuedCustomEvent
{
    ui::event::EventId id = ui::event::INVALID_EVENT_ID;
    ui::event::EventPayload payload;
};

struct EventRegistryContext
{
    std::unordered_map<std::string, ui::event::EventId> idsByName;
    std::unordered_map<ui::event::EventId, std::string> namesById;
    std::unordered_map<ui::event::EventId, std::vector<CallbackSlot>> callbacks;
    std::unordered_map<std::uint64_t, ui::event::EventId> idsByToken;
    std::vector<QueuedCustomEvent> queue;
    std::uint64_t nextToken = 1;
    ui::event::EventId nextEventId = 1;
};

[[nodiscard]] EventRegistryContext& CurrentContext()
{
    return RuntimeFacade::current().ensureContext<EventRegistryContext>();
}

[[nodiscard]] CallbackSlot* FindSlot(EventRegistryContext& ctx, std::uint64_t token) noexcept
{
    const auto idIt = ctx.idsByToken.find(token);
    if (idIt == ctx.idsByToken.end())
    {
        return nullptr;
    }

    const auto callbacksIt = ctx.callbacks.find(idIt->second);
    if (callbacksIt == ctx.callbacks.end())
    {
        return nullptr;
    }

    auto& slots = callbacksIt->second;
    const auto slotIt = std::ranges::find_if(slots, [token](const CallbackSlot& slot) { return slot.token == token; });
    return slotIt == slots.end() ? nullptr : &*slotIt;
}

void Dispatch(EventRegistryContext& ctx, ui::event::EventId eventId, const ui::event::EventPayload& payload)
{
    if (eventId == ui::event::INVALID_EVENT_ID)
    {
        return;
    }

    const auto callbacksIt = ctx.callbacks.find(eventId);
    if (callbacksIt == ctx.callbacks.end())
    {
        return;
    }

    for (auto& slot : callbacksIt->second)
    {
        if (slot.connected && slot.callback != nullptr && static_cast<bool>(*slot.callback))
        {
            (*slot.callback)(payload);
        }
    }
}

} // namespace

ui::event::EventId RegisterEvent(std::string_view name)
{
    if (name.empty())
    {
        return ui::event::INVALID_EVENT_ID;
    }

    auto& ctx = CurrentContext();
    const std::string key{name};
    if (const auto eventIt = ctx.idsByName.find(key); eventIt != ctx.idsByName.end())
    {
        return eventIt->second;
    }

    const auto eventId = ctx.nextEventId++;
    ctx.idsByName.emplace(key, eventId);
    ctx.namesById.emplace(eventId, key);
    return eventId;
}

bool IsEventRegistered(ui::event::EventId eventId)
{
    if (eventId == ui::event::INVALID_EVENT_ID)
    {
        return false;
    }

    return CurrentContext().namesById.contains(eventId);
}

bool IsEventRegistered(std::string_view name)
{
    if (name.empty())
    {
        return false;
    }

    return CurrentContext().idsByName.contains(std::string{name});
}

std::uint64_t Connect(ui::event::EventId eventId, ui::event::EventCallback callback)
{
    if (eventId == ui::event::INVALID_EVENT_ID || !static_cast<bool>(callback))
    {
        return 0;
    }

    auto& ctx = CurrentContext();
    if (!ctx.namesById.contains(eventId))
    {
        return 0;
    }

    const auto token = ctx.nextToken++;
    ctx.callbacks[eventId].push_back(
        CallbackSlot{.token = token,
                     .callback = std::make_shared<ui::event::EventCallback>(std::move(callback)),
                     .connected = true});
    ctx.idsByToken.emplace(token, eventId);
    return token;
}

std::uint64_t Connect(std::string_view name, ui::event::EventCallback callback)
{
    return Connect(RegisterEvent(name), std::move(callback));
}

void Disconnect(std::uint64_t token) noexcept
{
    if (token == 0)
    {
        return;
    }

    auto& ctx = CurrentContext();
    if (auto* slot = FindSlot(ctx, token); slot != nullptr)
    {
        slot->connected = false;
    }
    ctx.idsByToken.erase(token);
}

bool Connected(std::uint64_t token) noexcept
{
    if (token == 0)
    {
        return false;
    }

    auto& ctx = CurrentContext();
    const auto* slot = FindSlot(ctx, token);
    return slot != nullptr && slot->connected;
}

void Trigger(ui::event::EventId eventId, ui::event::EventPayload payload)
{
    Dispatch(CurrentContext(), eventId, payload);
}

void Trigger(std::string_view name, ui::event::EventPayload payload)
{
    Dispatch(CurrentContext(), RegisterEvent(name), payload);
}

void Enqueue(ui::event::EventId eventId, ui::event::EventPayload payload)
{
    if (eventId == ui::event::INVALID_EVENT_ID || !IsEventRegistered(eventId))
    {
        return;
    }

    CurrentContext().queue.push_back(QueuedCustomEvent{.id = eventId, .payload = std::move(payload)});
}

void Enqueue(std::string_view name, ui::event::EventPayload payload)
{
    const auto eventId = RegisterEvent(name);
    if (eventId == ui::event::INVALID_EVENT_ID)
    {
        return;
    }

    CurrentContext().queue.push_back(QueuedCustomEvent{.id = eventId, .payload = std::move(payload)});
}

void DispatchQueued()
{
    auto& ctx = CurrentContext();
    auto pending = std::exchange(ctx.queue, {});
    for (const auto& event : pending)
    {
        Dispatch(ctx, event.id, event.payload);
    }
}

} // namespace ui::detail::event_bridge
