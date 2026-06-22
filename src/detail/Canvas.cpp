#include "Canvas.hpp"

#include "common/Scale.hpp"

#include "core/RuntimeFacade.hpp"
#include "common/components/Data.hpp"
#include <vector>
#include <utility>

namespace ui::detail::canvas
{

namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}
} // namespace

void Clear(entt::entity entity)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    list.commands.clear();
}

void DrawLine(entt::entity entity, Vec2 from, Vec2 endPos, Color color, float lineWidth)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    list.commands.push_back({.type = components::CanvasDrawType::LINE,
                             .p1 = scale::Metric(from),
                             .p2 = scale::Metric(endPos),
                             .p3 = {},
                             .p4 = {},
                             .color = color,
                             .lineWidth = scale::Metric(lineWidth),
                             .points = {}});
}

void DrawRect(entt::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color, float lineWidth)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    list.commands.push_back({.type = components::CanvasDrawType::RECT,
                             .p1 = scale::Metric(topLeft),
                             .p2 = scale::Metric(bottomRight),
                             .p3 = {},
                             .p4 = {},
                             .color = color,
                             .lineWidth = scale::Metric(lineWidth),
                             .points = {}});
}

void DrawFilledRect(entt::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    list.commands.push_back({.type = components::CanvasDrawType::FILLED_RECT,
                             .p1 = scale::Metric(topLeft),
                             .p2 = scale::Metric(bottomRight),
                             .p3 = {},
                             .p4 = {},
                             .color = color,
                             .lineWidth = scale::Metric(1.0F),
                             .points = {}});
}

void DrawCircle(entt::entity entity, Vec2 center, float radius, Color color, float lineWidth)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    list.commands.push_back({.type = components::CanvasDrawType::CIRCLE,
                             .p1 = scale::Metric(center),
                             .p2 = {scale::Metric(radius), 0.0F},
                             .p3 = {},
                             .p4 = {},
                             .color = color,
                             .lineWidth = scale::Metric(lineWidth),
                             .points = {}});
}

void DrawFilledCircle(entt::entity entity, Vec2 center, float radius, Color color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    list.commands.push_back({.type = components::CanvasDrawType::FILLED_CIRCLE,
                             .p1 = scale::Metric(center),
                             .p2 = {scale::Metric(radius), 0.0F},
                             .p3 = {},
                             .p4 = {},
                             .color = color,
                             .lineWidth = scale::Metric(1.0F),
                             .points = {}});
}

void DrawPolyline(entt::entity entity, std::vector<Vec2> points, Color color, float lineWidth)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity) || points.size() < 2) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    components::CanvasDrawCommand cmd;
    cmd.type = components::CanvasDrawType::POLYLINE;
    cmd.color = color;
    cmd.lineWidth = scale::Metric(lineWidth);
    for (auto& point : points)
    {
        point = scale::Metric(point);
    }
    cmd.points = std::move(points);
    list.commands.push_back(std::move(cmd));
}

void DrawCubicBezier(entt::entity entity, Vec2 startPos, Vec2 cp1, Vec2 cp2, Vec2 endPos, Color color, float lineWidth)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& list = reg.get_or_emplace<components::CanvasDrawList>(entity);
    components::CanvasDrawCommand cmd;
    cmd.type = components::CanvasDrawType::CUBIC_BEZIER;
    cmd.p1 = scale::Metric(startPos);
    cmd.p2 = scale::Metric(cp1);
    cmd.p3 = scale::Metric(cp2);
    cmd.p4 = scale::Metric(endPos);
    cmd.color = color;
    cmd.lineWidth = scale::Metric(lineWidth);
    list.commands.push_back(std::move(cmd));
}
} // namespace ui::detail::canvas
