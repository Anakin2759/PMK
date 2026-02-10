/**
 * ************************************************************************
 *
 * @file animation.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-27
 * @version 0.1
 * @brief 动画API封装
  - 提供启动和停止位置与透明度动画的接口
  - 基于ECS组件实现动画状态管理
  - 简化动画控制逻辑，便于UI元素动画效果实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <entt/entt.hpp>
#include "Utils.hpp"

namespace ui::animation
{
void StartPositionAnimation(::entt::entity entity, const Vec2& startPos, const Vec2& endPos, float duration);
void StartAlphaAnimation(::entt::entity entity, float startAlpha, float endAlpha, float duration);
void StopAnimation(::entt::entity entity);

} // namespace ui::animation
