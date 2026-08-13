/**
 * ************************************************************************
 *
 * @file FocusNavigationSystem.cpp
 * @brief 焦点导航系统实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include "FocusNavigationSystem.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>

#include "common/GlobalContext.hpp"
#include "common/Tags.hpp"
#include "helper/Helper.hpp"

namespace ui::systems
{

namespace
{

/// 空间导航中次轴距离在评分中的权重（远小于主轴，仅用于 tie-break）。
constexpr float kSecondaryAxisWeight = 0.0001F;

/// 矩形中心偏移系数（用于取宽/高的一半求中心）。
constexpr float kHalf = 0.5F;

/// 矩形中心横向坐标。
float CenterX(const Rect& rect)
{
    return rect.x() + (rect.width() * kHalf);
}

/// 矩形中心纵向坐标。
float CenterY(const Rect& rect)
{
    return rect.y() + (rect.height() * kHalf);
}

globalcontext::StateContext& EnsureStateContext(Registry& reg)
{
    if (auto* existing = reg.ctx().find<globalcontext::StateContext>())
    {
        return *existing;
    }
    return reg.ctx().emplace<globalcontext::StateContext>();
}

/// 可聚焦判定：有 FocusableTag 且未禁用且可见。
bool IsFocusable(Registry& reg, entt::entity entity)
{
    return reg.all_of<components::FocusableTag>(entity) && !reg.any_of<components::DisabledTag>(entity) &&
           reg.all_of<components::VisibleTag>(entity);
}

}  // namespace

void FocusNavigationSystem::registerHandlersImpl()
{
    m_disp->sink<events::RawKeyInput>().connect<&FocusNavigationSystem::onRawKeyInput>(*this);
}

void FocusNavigationSystem::unregisterHandlersImpl()
{
    m_disp->sink<events::RawKeyInput>().disconnect<&FocusNavigationSystem::onRawKeyInput>(*this);
}

void FocusNavigationSystem::onRawKeyInput(const events::RawKeyInput& event)
{
    if (!event.pressed || event.repeat)
    {
        return;
    }

    if (event.key == SDLK_TAB)
    {
        navigateSequential((event.modifiers & SDL_KMOD_SHIFT) != 0);
        return;
    }

    switch (event.key)
    {
        case SDLK_UP:
        case SDLK_DOWN:
        case SDLK_LEFT:
        case SDLK_RIGHT:
            navigateSpatial(event.key);
            break;
        default:
            break;
    }
}

void FocusNavigationSystem::navigateSequential(bool backward)
{
    auto& reg = *m_reg;
    auto& state = EnsureStateContext(reg);

    // 焦点陷阱：若有当前焦点，限制在同一作用域（窗口/对话框）内循环，避免 Tab 逃出弹窗。
    entt::entity scope = entt::null;
    if (state.focusedEntity != entt::null && reg.valid(state.focusedEntity))
    {
        scope = findFocusScope(state.focusedEntity);
    }

    const std::vector<entt::entity> focusables = collectFocusables(scope);
    if (focusables.empty())
    {
        return;
    }

    // 定位当前焦点在序列中的位置；无焦点时 currentIndex == focusables.size()
    std::size_t currentIndex = focusables.size();
    if (state.focusedEntity != entt::null)
    {
        for (std::size_t i = 0; i < focusables.size(); ++i)
        {
            if (focusables[i] == state.focusedEntity)
            {
                currentIndex = i;
                break;
            }
        }
    }

    entt::entity target = entt::null;
    if (backward)
    {
        if (currentIndex == focusables.size() || currentIndex == 0)
        {
            target = focusables.back();  // 无焦点，或已在最前 → 环绕到最后
        }
        else
        {
            target = focusables[currentIndex - 1];
        }
    }
    else
    {
        if (currentIndex == focusables.size() || currentIndex == focusables.size() - 1)
        {
            target = focusables.front();  // 无焦点，或已在最后 → 环绕到最前
        }
        else
        {
            target = focusables[currentIndex + 1];
        }
    }

    requestFocus(target);
}

void FocusNavigationSystem::navigateSpatial(int32_t key)
{
    auto& reg = *m_reg;
    auto& state = EnsureStateContext(reg);

    if (state.focusedEntity == entt::null || !reg.valid(state.focusedEntity))
    {
        return;  // 无当前焦点时方向键不启动导航（仅 Tab 可启动）
    }

    const entt::entity scope = findFocusScope(state.focusedEntity);
    const std::vector<entt::entity> focusables = collectFocusables(scope);

    const Rect currentRect = ui::utils::GetEntityRect(state.focusedEntity);
    const float currentCenterX = CenterX(currentRect);
    const float currentCenterY = CenterY(currentRect);

    entt::entity best = entt::null;
    float bestScore = std::numeric_limits<float>::infinity();

    for (const entt::entity candidate : focusables)
    {
        if (candidate == state.focusedEntity)
        {
            continue;
        }

        const Rect candidateRect = ui::utils::GetEntityRect(candidate);
        const float deltaX = CenterX(candidateRect) - currentCenterX;
        const float deltaY = CenterY(candidateRect) - currentCenterY;

        float primaryDelta = 0.0F;    // 主轴方向距离（> 0 才符合方向）
        float secondaryDelta = 0.0F;  // 次轴方向距离（用于 tie-break）

        switch (key)
        {
            case SDLK_UP:
                primaryDelta = -deltaY;
                secondaryDelta = std::abs(deltaX);
                break;
            case SDLK_DOWN:
                primaryDelta = deltaY;
                secondaryDelta = std::abs(deltaX);
                break;
            case SDLK_LEFT:
                primaryDelta = -deltaX;
                secondaryDelta = std::abs(deltaY);
                break;
            case SDLK_RIGHT:
                primaryDelta = deltaX;
                secondaryDelta = std::abs(deltaY);
                break;
            default:
                return;
        }

        if (primaryDelta <= 0.0F)
        {
            continue;  // 不在目标方向一侧
        }

        const float score = primaryDelta + (secondaryDelta * kSecondaryAxisWeight);
        if (score < bestScore)
        {
            bestScore = score;
            best = candidate;
        }
    }

    if (best != entt::null)
    {
        requestFocus(best);
    }
}

std::vector<entt::entity> FocusNavigationSystem::collectFocusables(entt::entity scope) const
{
    std::vector<entt::entity> result;
    for (const entt::entity entity : m_reg->view<components::FocusableTag>())
    {
        if (!IsFocusable(*m_reg, entity))
        {
            continue;
        }
        if (scope != entt::null && entity != scope && !isDescendantOf(entity, scope))
        {
            continue;
        }
        result.push_back(entity);
    }

    // entt view 的遍历顺序是未指定的；此处显式按实体 id 升序（即创建顺序）排序，
    // 保证 Tab 顺序导航的确定性。
    std::ranges::sort(result);
    return result;
}

entt::entity FocusNavigationSystem::findFocusScope(entt::entity entity) const
{
    entt::entity current = entity;
    while (current != entt::null && m_reg->valid(current))
    {
        if (m_reg->any_of<components::WindowTag, components::DialogTag>(current))
        {
            return current;
        }
        const auto* hierarchy = m_reg->try_get<components::Hierarchy>(current);
        current = (hierarchy != nullptr) ? hierarchy->parent : entt::null;
    }
    return entt::null;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- entity/anchor 语义清晰
bool FocusNavigationSystem::isDescendantOf(entt::entity entity, entt::entity anchor) const
{
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

void FocusNavigationSystem::requestFocus(entt::entity target)
{
    m_disp->trigger<events::FocusChangeRequest>(events::FocusChangeRequest{target});
}

}  // namespace ui::systems
