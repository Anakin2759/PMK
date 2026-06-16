#include "RuntimeFacade.hpp"

#include "UiRuntime.hpp"

#include "WindowEntityLookup.hpp"
#include "singleton/Registry.hpp"
#include "singleton/Dispatcher.hpp"
#include "entt/entity/fwd.hpp"
#include <cstdint>

namespace ui
{

RuntimeFacade::ActiveRuntimeState RuntimeFacade::activateRuntime(UiRuntime& runtime) const
{
    WorkerMailbox*& mailboxSlot = activeMailbox();

    // 设置 RuntimeFacade 内部的 thread_local 跟踪指针（替代 deprecated Registry::current()）
    Registry*& regSlot = activeRegistryPtr();
    Dispatcher*& dispSlot = activeDispatcherPtr();

    auto state = ActiveRuntimeState{
        .registry = std::exchange(regSlot, &runtime.m_registry),
        .dispatcher = std::exchange(dispSlot, &runtime.m_dispatcher),
        .mailbox = std::exchange(mailboxSlot, &runtime.m_mailbox),
    };

    // 同时同步 EnTT 单例层（供旧代码过渡期使用，后续移除）
    Registry::swapActiveInstance(&runtime.m_registry);
    Dispatcher::swapActiveInstance(&runtime.m_dispatcher);

    return state;
}

void RuntimeFacade::restoreRuntime(ActiveRuntimeState previousRuntime) const
{
    // 恢复 RuntimeFacade 内部指针
    activeRegistryPtr() = previousRuntime.registry;
    activeDispatcherPtr() = previousRuntime.dispatcher;
    activeMailbox() = previousRuntime.mailbox;

    // 同步恢复 EnTT 单例层（过渡期）
    Dispatcher::swapActiveInstance(previousRuntime.dispatcher);
    Registry::swapActiveInstance(previousRuntime.registry);
}

entt::entity RuntimeFacade::WindowLookupService::findById(uint32_t windowId) const
{
    return window_lookup::FindWindowEntityById(windowId);
}

void RuntimeFacade::WindowLookupService::remember(entt::entity entity) const
{
    window_lookup::RememberWindowEntity(entity);
}

void RuntimeFacade::WindowLookupService::invalidateId(uint32_t windowId) const
{
    window_lookup::InvalidateWindowId(windowId);
}

void RuntimeFacade::WindowLookupService::invalidateEntity(entt::entity entity) const
{
    window_lookup::InvalidateWindowEntity(entity);
}

} // namespace ui