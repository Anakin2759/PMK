/**
 * ************************************************************************
 *
 * @file Canvas.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-18
 * @version 0.1
 * @brief Canvas 绘图 API — 提供命令式绘制和 Chain DSL
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "common/Types.hpp"
#include "entt/entity/fwd.hpp"

#include <vector>

namespace ui::detail::canvas
{

void Clear(entt::entity entity);
void DrawLine(entt::entity entity, Vec2 from, Vec2 endPos, Color color, float lineWidth = 1.0F);
void DrawRect(entt::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color, float lineWidth = 1.0F);
void DrawFilledRect(entt::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color);
void DrawCircle(entt::entity entity, Vec2 center, float radius, Color color, float lineWidth = 1.0F);
void DrawFilledCircle(entt::entity entity, Vec2 center, float radius, Color color);

// ---- 新增：折线 & 三次贝塞尔 ----
void DrawPolyline(entt::entity entity, std::vector<Vec2> points, Color color, float lineWidth = 1.0F);

void DrawCubicBezier(
    entt::entity entity, Vec2 startPos, Vec2 cp1, Vec2 cp2, Vec2 endPos, Color color, float lineWidth = 1.0F);
} // namespace ui::detail::canvas
