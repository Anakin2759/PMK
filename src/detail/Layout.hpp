/**
 * ************************************************************************
 *
 * @file layout.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-27
 * @version 0.1
 * @brief 布局API封装
  - 提供设置布局方向、间距和内边距的接口
  - 支持标记布局脏标志以触发重新布局
  - 基于ECS组件实现布局状态管理
  - 简化UI元素的布局控制逻辑
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "common/Policies.hpp"
#include "entt/entity/fwd.hpp"

namespace ui::detail::layout
{
/**
 * @brief 设置布局方向
 * @param entity 目标实体
 * @param direction 布局方向（水平或垂直）
 */
void SetLayoutDirection(entt::entity entity, policies::LayoutDirection direction);
void SetLayoutSpacing(entt::entity entity, float spacing);
void SetPadding(entt::entity entity, float left, float top, float right, float bottom);
void SetPadding(entt::entity entity, float padding);
void CenterInParent(entt::entity entity);

} // namespace ui::detail::layout
