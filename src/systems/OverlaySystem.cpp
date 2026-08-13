/**
 * ************************************************************************
 *
 * @file OverlaySystem.cpp
 * @brief 浮层系统实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include "OverlaySystem.hpp"

#include <algorithm>

#include <SDL3/SDL_mouse.h>

#include "common/GlobalContext.hpp"
#include "common/Tags.hpp"
#include "common/components/Overlay.hpp"
#include "common/components/Layout.hpp"

namespace ui::systems
{

namespace
{

globalcontext::OverlayContext& EnsureOverlayContext(Registry& reg)
{
    if (auto* existing = reg.ctx().find<globalcontext::OverlayContext>())
    {
        return *existing;
    }
    return reg.ctx().emplace<globalcontext::OverlayContext>();
}

}  // namespace

void OverlaySystem::registerHandlersImpl()
{
    m_disp->sink<events::OverlayOpenRequest>().connect<&OverlaySystem::onOpenRequest>(*this);
    m_disp->sink<events::OverlayCloseRequest>().connect<&OverlaySystem::onCloseRequest>(*this);
    m_disp->sink<events::OverlayCloseAllRequest>().connect<&OverlaySystem::onCloseAllRequest>(*this);
    m_disp->sink<events::HitPointerButton>().connect<&OverlaySystem::onHitPointerButton>(*this);
}

void OverlaySystem::unregisterHandlersImpl()
{
    m_disp->sink<events::OverlayOpenRequest>().disconnect<&OverlaySystem::onOpenRequest>(*this);
    m_disp->sink<events::OverlayCloseRequest>().disconnect<&OverlaySystem::onCloseRequest>(*this);
    m_disp->sink<events::OverlayCloseAllRequest>().disconnect<&OverlaySystem::onCloseAllRequest>(*this);
    m_disp->sink<events::HitPointerButton>().disconnect<&OverlaySystem::onHitPointerButton>(*this);
}

void OverlaySystem::onOpenRequest(const events::OverlayOpenRequest& event)
{
    if (event.entity == entt::null || !m_reg->valid(event.entity))
    {
        return;
    }

    auto& reg = *m_reg;
    auto& ctx = EnsureOverlayContext(reg);

    // 已在栈中则忽略重复打开
    if (std::ranges::find(ctx.stack, event.entity) != ctx.stack.end())
    {
        return;
    }

    const int zLevel = static_cast<int>(ctx.stack.size());
    const int zOrder = ctx.nextZBase + zLevel;

    // 统一分配 z-order 并写入浮层标记
    reg.emplace_or_replace<components::ZOrderIndex>(event.entity).value = zOrder;
    auto& overlay = reg.emplace_or_replace<components::OverlayLayer>(event.entity);
    overlay.owner = event.owner;
    overlay.zLevel = zLevel;

    ctx.stack.push_back(event.entity);
}

void OverlaySystem::onCloseRequest(const events::OverlayCloseRequest& event)
{
    if (event.entity == entt::null)
    {
        return;
    }

    auto& reg = *m_reg;
    auto& ctx = EnsureOverlayContext(reg);

    const auto iter = std::ranges::find(ctx.stack, event.entity);
    if (iter == ctx.stack.end())
    {
        return;
    }

    entt::entity owner = entt::null;
    if (const auto* overlay = reg.try_get<components::OverlayLayer>(event.entity); overlay != nullptr)
    {
        owner = overlay->owner;
    }

    ctx.stack.erase(iter);

    // 焦点恢复到 owner（若有效且可聚焦）
    if (owner != entt::null && reg.valid(owner) && reg.any_of<components::FocusableTag>(owner))
    {
        m_disp->trigger<events::FocusChangeRequest>(events::FocusChangeRequest{owner});
    }
}

void OverlaySystem::onCloseAllRequest(const events::OverlayCloseAllRequest& /*event*/)
{
    auto& reg = *m_reg;
    auto& ctx = EnsureOverlayContext(reg);

    // 从栈顶到栈底依次出栈（后进先出）
    while (!ctx.stack.empty())
    {
        const entt::entity top = ctx.stack.back();
        ctx.stack.pop_back();
        if (const auto* overlay = reg.try_get<components::OverlayLayer>(top); overlay != nullptr &&
            overlay->owner != entt::null && reg.valid(overlay->owner) &&
            reg.any_of<components::FocusableTag>(overlay->owner))
        {
            // 仅恢复最顶层浮层的 owner（最后一次覆盖），避免多次触发
            m_disp->trigger<events::FocusChangeRequest>(events::FocusChangeRequest{overlay->owner});
            return;
        }
    }
}

bool OverlaySystem::isDescendantOf(entt::entity entity, entt::entity anchor) const
{
    if (anchor == entt::null)
    {
        return false;
    }

    entt::entity current = entity;
    while (current != entt::null && m_reg->valid(current))
    {
        if (current == anchor)
        {
            return true;
        }
        const auto* hierarchy = m_reg->try_get<components::Hierarchy>(current);
        current = (hierarchy != nullptr) ? hierarchy->parent : entt::null;
    }
    return false;
}

void OverlaySystem::onHitPointerButton(const events::HitPointerButton& event)
{
    // 仅处理左键按下
    if (event.raw.button != SDL_BUTTON_LEFT || !event.raw.pressed)
    {
        return;
    }

    auto& reg = *m_reg;
    auto& ctx = EnsureOverlayContext(reg);
    if (ctx.stack.empty())
    {
        return;
    }

    // 只检查最顶层浮层：命中点在其子树内则保留，否则关闭
    const entt::entity top = ctx.stack.back();
    if (event.hitEntity != entt::null && isDescendantOf(event.hitEntity, top))
    {
        return;
    }

    m_disp->trigger<events::OverlayCloseRequest>(events::OverlayCloseRequest{top});
}

}  // namespace ui::systems
