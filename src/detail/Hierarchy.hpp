/**
 * ************************************************************************
 *
 * @file Hierarchy.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-27
 * @version 0.1
 * @brief 层级关系API封装
  - 提供添加/移除子元素的接口
  - 支持遍历子元素的功能
  - 基于ECS组件实现层级关系管理
  - 简化UI元素的层级操作逻辑
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <vector>

#include "entt/entity/fwd.hpp"

namespace ui::detail::hierarchy
{
/**
 * @brief 从父节点移除子节点
 * @param parent 父节点实体
 * @param child 子节点实体
 */
void RemoveChild(entt::entity parent, entt::entity child);
/**
 * @brief 向父节点添加子节点
 * @param parent 父节点实体
 * @param child 子节点实体
 */
void AddChild(entt::entity parent, entt::entity child);

[[nodiscard]] std::vector<entt::entity> ChildrenPostOrder(entt::entity parent);

} // namespace ui::detail::hierarchy
