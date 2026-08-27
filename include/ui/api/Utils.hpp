/**
 * ************************************************************************
 *
 * @file Utils.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-06
 * @version 0.1
 * @brief 对外部提供的通用工具函数
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

#include "ui/Geometry.hpp"
#include "ui/MathTypes.hpp"
#include "ui/Policies.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui
{
class UiRuntime;
}

namespace ui::utils
{

bool HasAlignment(policies::Alignment value, policies::Alignment flag);
void SetWindowFlag(UiRuntime& runtime, ui::entity entity, policies::WindowFlag flag);

void MarkLayoutChanged(UiRuntime& runtime, ui::entity entity);
void MarkVisualChanged(UiRuntime& runtime, ui::entity entity);
void MarkLayoutAndVisualChanged(UiRuntime& runtime, ui::entity entity);
void MarkLayoutDirty(UiRuntime& runtime, ui::entity entity);
void MarkRenderDirty(UiRuntime& runtime, ui::entity entity);
void CloseWindow(UiRuntime& runtime, ui::entity entity);
void QuitUiEventLoop(UiRuntime& runtime);

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkLayoutChanged(UiRuntime& runtime, EntityLike entity)
{
    MarkLayoutChanged(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkVisualChanged(UiRuntime& runtime, EntityLike entity)
{
    MarkVisualChanged(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkLayoutAndVisualChanged(UiRuntime& runtime, EntityLike entity)
{
    MarkLayoutAndVisualChanged(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkLayoutDirty(UiRuntime& runtime, EntityLike entity)
{
    MarkLayoutDirty(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkRenderDirty(UiRuntime& runtime, EntityLike entity)
{
    MarkRenderDirty(runtime, static_cast<ui::entity>(entity));
}

[[nodiscard]] Vec2 GetAbsolutePosition(UiRuntime& runtime, ui::entity entity);
[[nodiscard]] Rect GetEntityRect(UiRuntime& runtime, ui::entity entity);
[[nodiscard]] Rect GetScrollViewportRect(UiRuntime& runtime, ui::entity entity);
[[nodiscard]] float GetScrollViewportLength(UiRuntime& runtime, ui::entity entity, bool isVertical);
[[nodiscard]] float GetScrollContentLength(UiRuntime& runtime, ui::entity entity, bool isVertical);
[[nodiscard]] float GetScrollMaxOffset(UiRuntime& runtime, ui::entity entity, bool isVertical);
[[nodiscard]] VerticalScrollbarGeometry GetVerticalScrollbarGeometry(UiRuntime& runtime, ui::entity entity);

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] Vec2 GetAbsolutePosition(UiRuntime& runtime, EntityLike entity)
{
    return GetAbsolutePosition(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] Rect GetEntityRect(UiRuntime& runtime, EntityLike entity)
{
    return GetEntityRect(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] Rect GetScrollViewportRect(UiRuntime& runtime, EntityLike entity)
{
    return GetScrollViewportRect(runtime, static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] float GetScrollViewportLength(UiRuntime& runtime, EntityLike entity, bool isVertical)
{
    return GetScrollViewportLength(runtime, static_cast<ui::entity>(entity), isVertical);
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] float GetScrollContentLength(UiRuntime& runtime, EntityLike entity, bool isVertical)
{
    return GetScrollContentLength(runtime, static_cast<ui::entity>(entity), isVertical);
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] float GetScrollMaxOffset(UiRuntime& runtime, EntityLike entity, bool isVertical)
{
    return GetScrollMaxOffset(runtime, static_cast<ui::entity>(entity), isVertical);
}

void InvokeTask(UiRuntime& runtime, std::function<void()> func);
using TaskHandle = std::uint32_t;
TaskHandle TimerCallback(UiRuntime& runtime, std::uint32_t interval, std::function<void()> func);
void CancelQueuedTask(UiRuntime& runtime, TaskHandle handle);
bool IsEntityExist(UiRuntime& runtime, const std::string& alias);

}  // namespace ui::utils

namespace ui::actions::utils
{
inline constexpr EntityAction<&ui::utils::SetWindowFlag> SET_WINDOW_FLAG_ACTION{};
}  // namespace ui::actions::utils

namespace ui::chains
{
inline auto WindowFlag(policies::WindowFlag flag)
{
    return ui::actions::utils::SET_WINDOW_FLAG_ACTION.bind(flag);
}
}  // namespace ui::chains