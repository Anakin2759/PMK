/**
 * ************************************************************************
 *
 * @file Layout.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-27
 * @version 0.1
 * @brief 布局 API 封装
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "ui/Policies.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui
{
class UiRuntime;
}

namespace ui::layout
{
void SetLayoutDirection(UiRuntime& runtime, ui::entity entity, policies::LayoutDirection direction);
void SetLayoutSpacing(UiRuntime& runtime, ui::entity entity, float spacing);
void SetPadding(UiRuntime& runtime, ui::entity entity, float left, float top, float right, float bottom);
void SetPadding(UiRuntime& runtime, ui::entity entity, float padding);
void CenterInParent(UiRuntime& runtime, ui::entity entity);
}  // namespace ui::layout

namespace ui::actions::layout
{
inline constexpr EntityAction<static_cast<void (*)(UiRuntime&, ui::entity, policies::LayoutDirection)>(
    &ui::layout::SetLayoutDirection)>
    SET_LAYOUT_DIRECTION_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(UiRuntime&, ui::entity, float)>(&ui::layout::SetLayoutSpacing)>
    SET_LAYOUT_SPACING_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(UiRuntime&, ui::entity, float, float, float, float)>(
    ui::layout::SetPadding)>
    SET_PADDING_EDGES_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(UiRuntime&, ui::entity, float)>(ui::layout::SetPadding)>
    SET_PADDING_ALL_ACTION{};
inline constexpr EntityAction<&ui::layout::CenterInParent> CENTER_IN_PARENT_ACTION{};
}  // namespace ui::actions::layout

namespace ui::chains
{
inline auto LayoutDirection(ui::policies::LayoutDirection direction)
{
    return ui::actions::layout::SET_LAYOUT_DIRECTION_ACTION.bind(direction);
}

inline auto Spacing(float spacing)
{
    return ui::actions::layout::SET_LAYOUT_SPACING_ACTION.bind(spacing);
}

inline auto Padding(float left, float top, float right, float bottom)
{
    return ui::actions::layout::SET_PADDING_EDGES_ACTION.bind(left, top, right, bottom);
}

inline auto Padding(float padding)
{
    return ui::actions::layout::SET_PADDING_ALL_ACTION.bind(padding);
}

inline auto Center()
{
    return ui::actions::layout::CENTER_IN_PARENT_ACTION.bind();
}
}  // namespace ui::chains
