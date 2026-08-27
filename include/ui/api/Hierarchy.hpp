/**
 * ************************************************************************
 *
 * @file Hierarchy.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-27
 * @version 0.1
 * @brief 层级关系API封装
 *
 * ************************************************************************
 */
#pragma once

#include <type_traits>
#include <vector>

#include "ui/api/Entity.hpp"
#include "ui/api/Chains.hpp"

namespace ui::hierarchy
{
void RemoveChild(ui::entity parent, ui::entity child);
void AddChild(ui::entity parent, ui::entity child);
[[nodiscard]] std::vector<ui::entity> ChildrenPostOrder(ui::entity parent);

template <typename Func>
void TraverseChildren(ui::entity parent, Func visitor)
{
    for (const ui::entity child : ChildrenPostOrder(parent))
    {
        visitor(child);
    }
}

template <typename ParentEntity, typename ChildEntity>
    requires((!std::same_as<std::remove_cvref_t<ParentEntity>, ui::entity> ||
              !std::same_as<std::remove_cvref_t<ChildEntity>, ui::entity>) &&
             (std::is_enum_v<std::remove_cvref_t<ParentEntity>> ||
              std::is_integral_v<std::remove_cvref_t<ParentEntity>>) &&
             (std::is_enum_v<std::remove_cvref_t<ChildEntity>> || std::is_integral_v<std::remove_cvref_t<ChildEntity>>))
void AddChild(ParentEntity parent, ChildEntity child)
{
    AddChild(static_cast<ui::entity>(parent), static_cast<ui::entity>(child));
}

template <typename ParentEntity, typename ChildEntity>
    requires((!std::same_as<std::remove_cvref_t<ParentEntity>, ui::entity> ||
              !std::same_as<std::remove_cvref_t<ChildEntity>, ui::entity>) &&
             (std::is_enum_v<std::remove_cvref_t<ParentEntity>> ||
              std::is_integral_v<std::remove_cvref_t<ParentEntity>>) &&
             (std::is_enum_v<std::remove_cvref_t<ChildEntity>> || std::is_integral_v<std::remove_cvref_t<ChildEntity>>))
void RemoveChild(ParentEntity parent, ChildEntity child)
{
    RemoveChild(static_cast<ui::entity>(parent), static_cast<ui::entity>(child));
}
}  // namespace ui::hierarchy

namespace ui::actions::hierarchy
{
inline constexpr EntityAction<static_cast<void (*)(ui::entity, ui::entity)>(&ui::hierarchy::AddChild)>
    ADD_CHILD_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, ui::entity)>(&ui::hierarchy::RemoveChild)>
    REMOVE_CHILD_ACTION{};
}  // namespace ui::actions::hierarchy

namespace ui::chains
{
inline auto AddChild(ui::entity child)
{
    return ui::actions::hierarchy::ADD_CHILD_ACTION.bind(child);
}
inline auto RemoveChild(ui::entity child)
{
    return ui::actions::hierarchy::REMOVE_CHILD_ACTION.bind(child);
}
}  // namespace ui::chains