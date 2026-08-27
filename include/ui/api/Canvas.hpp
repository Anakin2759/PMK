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

#include <vector>

#include "ui/Color.hpp"
#include "ui/MathTypes.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui::canvas
{

void Clear(UiRuntime& runtime, ui::entity entity);
void DrawLine(UiRuntime& runtime, ui::entity entity, Vec2 from, Vec2 endPos, Color color, float lineWidth = 1.0F);
void DrawRect(UiRuntime& runtime, ui::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color, float lineWidth = 1.0F);
void DrawFilledRect(UiRuntime& runtime, ui::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color);
void DrawCircle(UiRuntime& runtime, ui::entity entity, Vec2 center, float radius, Color color, float lineWidth = 1.0F);
void DrawFilledCircle(UiRuntime& runtime, ui::entity entity, Vec2 center, float radius, Color color);
void DrawPolyline(UiRuntime& runtime, ui::entity entity, std::vector<Vec2> points, Color color, float lineWidth = 1.0F);
void DrawCubicBezier(UiRuntime& runtime, ui::entity entity, Vec2 startPos, Vec2 cp1, Vec2 cp2, Vec2 endPos,
                     Color color, float lineWidth = 1.0F);

/** @brief 路径构建器，调用 commit() 将路径写入 Canvas。 */
class Painter
{
   public:
    Painter(UiRuntime& runtime, ui::entity canvas) : m_runtime(&runtime), m_canvas(canvas)
    {
    }

    Painter& moveTo(Vec2 pos);
    Painter& lineTo(Vec2 pos);
    Painter& cubicTo(Vec2 cp1, Vec2 cp2, Vec2 endPos);
    Painter& polyline(std::vector<Vec2> points);
    Painter& commit(Color color, float lineWidth = 1.0F);

   private:
    UiRuntime* m_runtime;
    ui::entity m_canvas;
    std::vector<Vec2> m_path;
    Vec2 m_cursor{0.0F, 0.0F};
};

}  // namespace ui::canvas

namespace ui::actions::canvas
{
inline constexpr EntityAction<&ui::canvas::Clear> CANVAS_CLEAR_ACTION{};
inline constexpr EntityAction<&ui::canvas::DrawLine> CANVAS_DRAW_LINE_ACTION{};
inline constexpr EntityAction<&ui::canvas::DrawRect> CANVAS_DRAW_RECT_ACTION{};
inline constexpr EntityAction<&ui::canvas::DrawFilledRect> CANVAS_DRAW_FILLED_RECT_ACTION{};
}  // namespace ui::actions::canvas

namespace ui::chains
{

inline auto CanvasClear()
{
    return ui::actions::canvas::CANVAS_CLEAR_ACTION.bind();
}

inline auto CanvasDrawLine(Vec2 from, Vec2 endPos, Color color, float lineWidth = 1.0F)
{
    return ui::actions::canvas::CANVAS_DRAW_LINE_ACTION.bind(from, endPos, color, lineWidth);
}

inline auto CanvasDrawRect(Vec2 topLeft, Vec2 bottomRight, Color color, float lineWidth = 1.0F)
{
    return ui::actions::canvas::CANVAS_DRAW_RECT_ACTION.bind(topLeft, bottomRight, color, lineWidth);
}

inline auto CanvasDrawFilledRect(Vec2 topLeft, Vec2 bottomRight, Color color)
{
    return ui::actions::canvas::CANVAS_DRAW_FILLED_RECT_ACTION.bind(topLeft, bottomRight, color);
}

}  // namespace ui::chains