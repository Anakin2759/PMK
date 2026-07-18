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
void SetWindowFlag(ui::entity entity, policies::WindowFlag flag);

void MarkLayoutChanged(ui::entity entity);
void MarkVisualChanged(ui::entity entity);
void MarkLayoutAndVisualChanged(ui::entity entity);
void MarkLayoutDirty(ui::entity entity);
void MarkRenderDirty(ui::entity entity);
void CloseWindow(ui::entity entity);
void QuitUiEventLoop();

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkLayoutChanged(EntityLike entity)
{
    MarkLayoutChanged(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkVisualChanged(EntityLike entity)
{
    MarkVisualChanged(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkLayoutAndVisualChanged(EntityLike entity)
{
    MarkLayoutAndVisualChanged(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkLayoutDirty(EntityLike entity)
{
    MarkLayoutDirty(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void MarkRenderDirty(EntityLike entity)
{
    MarkRenderDirty(static_cast<ui::entity>(entity));
}

[[nodiscard]] Vec2 GetAbsolutePosition(ui::entity entity);
[[nodiscard]] Rect GetEntityRect(ui::entity entity);
[[nodiscard]] Rect GetScrollViewportRect(ui::entity entity);
[[nodiscard]] float GetScrollViewportLength(ui::entity entity, bool isVertical);
[[nodiscard]] float GetScrollContentLength(ui::entity entity, bool isVertical);
[[nodiscard]] float GetScrollMaxOffset(ui::entity entity, bool isVertical);
[[nodiscard]] VerticalScrollbarGeometry GetVerticalScrollbarGeometry(ui::entity entity);

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] Vec2 GetAbsolutePosition(EntityLike entity)
{
    return GetAbsolutePosition(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] Rect GetEntityRect(EntityLike entity)
{
    return GetEntityRect(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] Rect GetScrollViewportRect(EntityLike entity)
{
    return GetScrollViewportRect(static_cast<ui::entity>(entity));
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] float GetScrollViewportLength(EntityLike entity, bool isVertical)
{
    return GetScrollViewportLength(static_cast<ui::entity>(entity), isVertical);
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] float GetScrollContentLength(EntityLike entity, bool isVertical)
{
    return GetScrollContentLength(static_cast<ui::entity>(entity), isVertical);
}

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity>
             && (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
[[nodiscard]] float GetScrollMaxOffset(EntityLike entity, bool isVertical)
{
    return GetScrollMaxOffset(static_cast<ui::entity>(entity), isVertical);
}

void InvokeTask(UiRuntime& runtime, std::function<void()> func);
using TaskHandle = std::uint32_t;
TaskHandle TimerCallback(UiRuntime& runtime, std::uint32_t interval, std::function<void()> func);
void CancelQueuedTask(UiRuntime& runtime, TaskHandle handle);
bool IsEntityExist(const std::string& alias);

} // namespace ui::utils

namespace ui::actions::utils
{
inline constexpr EntityAction<&ui::utils::SetWindowFlag> SET_WINDOW_FLAG_ACTION{};
} // namespace ui::actions::utils

namespace ui::chains
{
inline auto WindowFlag(policies::WindowFlag flag)
{
    return ui::actions::utils::SET_WINDOW_FLAG_ACTION.bind(flag);
}
} // namespace ui::chains