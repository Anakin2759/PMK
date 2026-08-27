#include "ui/api/Utils.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <ranges>
#include <utility>
#include <string>
#include "core/UiRuntime.hpp"
#include "helper/Helper.hpp"
#include "systems/TimerSystem.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/entity/entity.hpp"
#include "common/Tags.hpp"
#include "common/components/Layout.hpp"
#include "ui/Policies.hpp"
#include "common/components/Window.hpp"
#include "common/Events.hpp"
#include "common/Types.hpp"
#include "common/components/Interaction.hpp"
#include "common/components/Data.hpp"
namespace ui::utils
{
namespace
{
Registry& CurrentRegistry()
{
    return UiRuntime::current().registry();
}
}  // namespace

void MarkLayoutChanged(UiRuntime& runtime, ui::entity entity)
{
    auto& reg = runtime.registry();
    if (!reg.valid(entity))
    {
        return;
    }

    entt::entity current = detail::ToInternal(entity);
    while (current != entt::null && reg.valid(current))
    {
        reg.emplace_or_replace<components::LayoutDirtyTag>(current);
        const auto* hierarchy = reg.try_get<components::Hierarchy>(current);
        current = hierarchy != nullptr ? hierarchy->parent : entt::null;
    }
}

void MarkLayoutChanged(ui::entity entity)
{
    MarkLayoutChanged(UiRuntime::current(), entity);
}

void MarkVisualChanged(UiRuntime& runtime, ui::entity entity)
{
    auto& reg = runtime.registry();
    if (!reg.valid(entity))
        return;

    reg.emplace_or_replace<components::RenderDirtyTag>(entity);

    entt::entity current = detail::ToInternal(entity);
    entt::entity rootWindow = entt::null;
    while (current != entt::null && reg.valid(current))
    {
        if (reg.any_of<components::WindowTag, components::DialogTag>(current))
            rootWindow = current;
        const auto* hierarchy = reg.try_get<components::Hierarchy>(current);
        current = hierarchy != nullptr ? hierarchy->parent : entt::null;
    }

    if (rootWindow != entt::null && rootWindow != detail::ToInternal(entity))
        reg.emplace_or_replace<components::RenderDirtyTag>(rootWindow);
}

void MarkVisualChanged(ui::entity entity)
{
    MarkVisualChanged(UiRuntime::current(), entity);
}

void MarkLayoutAndVisualChanged(UiRuntime& runtime, ui::entity entity)
{
    MarkLayoutChanged(runtime, entity);
    MarkVisualChanged(runtime, entity);
}

void MarkLayoutAndVisualChanged(ui::entity entity)
{
    MarkLayoutAndVisualChanged(UiRuntime::current(), entity);
}

void MarkLayoutDirty(UiRuntime& runtime, ui::entity entity) { MarkLayoutChanged(runtime, entity); }
void MarkRenderDirty(UiRuntime& runtime, ui::entity entity) { MarkVisualChanged(runtime, entity); }

void MarkLayoutDirty(ui::entity entity)
{
    MarkLayoutChanged(entity);
}

void MarkRenderDirty(ui::entity entity)
{
    MarkVisualChanged(entity);
}

bool HasAlignment(policies::Alignment value, policies::Alignment flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

void SetWindowFlag(ui::entity entity, policies::WindowFlag flag)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity))
        return;
    auto& windowComp = reg.get_or_emplace<components::Window>(entity);

    windowComp.flags |= flag;
}

void CloseWindow(ui::entity entity)
{
    auto& runtime = UiRuntime::current();
    auto& reg = runtime.registry();
    if (!reg.valid(entity))
        return;
    runtime.dispatcher().enqueue<events::CloseWindow>(events::CloseWindow{detail::ToInternal(entity)});
}

void QuitUiEventLoop()
{
    UiRuntime::current().dispatcher().trigger<ui::events::QuitRequested>(ui::events::QuitRequested{});
};

Vec2 GetAbsolutePosition(UiRuntime& /*runtime*/, ui::entity entity)
{
    return GetAbsolutePosition(entity);
}

Rect GetEntityRect(UiRuntime& /*runtime*/, ui::entity entity)
{
    return GetEntityRect(entity);
}

Rect GetScrollViewportRect(UiRuntime& /*runtime*/, ui::entity entity)
{
    return GetScrollViewportRect(entity);
}

float GetScrollViewportLength(UiRuntime& /*runtime*/, ui::entity entity, bool isVertical)
{
    return GetScrollViewportLength(entity, isVertical);
}

float GetScrollContentLength(UiRuntime& /*runtime*/, ui::entity entity, bool isVertical)
{
    return GetScrollContentLength(entity, isVertical);
}

float GetScrollMaxOffset(UiRuntime& /*runtime*/, ui::entity entity, bool isVertical)
{
    return GetScrollMaxOffset(entity, isVertical);
}

VerticalScrollbarGeometry GetVerticalScrollbarGeometry(UiRuntime& /*runtime*/, ui::entity entity)
{
    return GetVerticalScrollbarGeometry(entity);
}

Vec2 GetAbsolutePosition(ui::entity entity)
{
    auto& reg = CurrentRegistry();
    std::vector<entt::entity> path;
    entt::entity current = detail::ToInternal(entity);
    while (current != entt::null && reg.valid(current))
    {
        path.push_back(current);
        const auto* hierarchy = reg.try_get<components::Hierarchy>(current);
        current = hierarchy == nullptr ? entt::null : hierarchy->parent;
    }

    Vec2 position(0.0F, 0.0F);
    for (auto currentEntity : std::views::reverse(path))
    {
        if (reg.any_of<components::WindowTag, components::DialogTag>(currentEntity))
        {
            continue;
        }

        const auto* positionComp = reg.try_get<components::Position>(currentEntity);
        if (positionComp != nullptr)
        {
            position += positionComp->value;
        }

        // 若当前祖先节点（非目标实体本身）是滚动容器，
        // 其子内容被 RenderSystem 整体偏移 -scrollOffset，命中测试需同步扣除。
        if (currentEntity != detail::ToInternal(entity))
        {
            const auto* scrollArea = reg.try_get<components::ScrollArea>(currentEntity);
            if (scrollArea != nullptr)
            {
                position.x() -= scrollArea->scrollOffset.x();
                position.y() -= scrollArea->scrollOffset.y();
            }
        }
    }

    return position;
}

Rect GetEntityRect(ui::entity entity)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity))
    {
        return {};
    }

    const auto* sizeComp = reg.try_get<components::Size>(entity);
    if (sizeComp == nullptr)
    {
        return {GetAbsolutePosition(entity), Vec2(0.0F, 0.0F)};
    }

    return {GetAbsolutePosition(entity), sizeComp->size};
}

Rect GetScrollViewportRect(ui::entity entity)
{
    const Rect entityRect = GetEntityRect(entity);
    const auto* padding = CurrentRegistry().try_get<components::Padding>(entity);
    if (padding == nullptr)
    {
        return entityRect;
    }

    const float left = padding->values.w();
    const float top = padding->values.x();
    const float right = padding->values.y();
    const float bottom = padding->values.z();

    return {entityRect.x() + left, entityRect.y() + top, std::max(0.0F, entityRect.width() - left - right),
            std::max(0.0F, entityRect.height() - top - bottom)};
}

float GetScrollViewportLength(ui::entity entity, bool isVertical)
{
    const Rect viewportRect = GetScrollViewportRect(entity);
    return isVertical ? viewportRect.height() : viewportRect.width();
}

float GetScrollContentLength(ui::entity entity, bool isVertical)
{
    const auto* scrollArea = CurrentRegistry().try_get<components::ScrollArea>(entity);
    if (scrollArea == nullptr)
    {
        return 0.0F;
    }

    return isVertical ? scrollArea->contentSize.y() : scrollArea->contentSize.x();
}

float GetScrollMaxOffset(ui::entity entity, bool isVertical)
{
    const float contentLength = GetScrollContentLength(entity, isVertical);
    const float viewportLength = GetScrollViewportLength(entity, isVertical);
    return std::max(0.0F, contentLength - viewportLength);
}

VerticalScrollbarGeometry GetVerticalScrollbarGeometry(ui::entity entity)
{
    VerticalScrollbarGeometry geometry;
    const auto* scrollArea = CurrentRegistry().try_get<components::ScrollArea>(entity);
    if (scrollArea == nullptr)
    {
        return geometry;
    }

    const bool hasVerticalScroll =
        scrollArea->scroll == policies::Scroll::VERTICAL || scrollArea->scroll == policies::Scroll::BOTH;
    if (!hasVerticalScroll)
    {
        return geometry;
    }

    const Rect containerRect = GetEntityRect(entity);
    const Rect viewportRect = GetScrollViewportRect(entity);
    geometry.containerRect = {containerRect.x(), containerRect.y(), containerRect.width(), containerRect.height()};
    geometry.viewportRect = {viewportRect.x(), viewportRect.y(), viewportRect.width(), viewportRect.height()};
    geometry.viewportHeight = geometry.viewportRect.height;
    geometry.contentHeight = scrollArea->contentSize.y();
    geometry.trackHeight = geometry.containerRect.height;
    geometry.maxScroll = std::max(0.0F, geometry.contentHeight - geometry.viewportHeight);

    if (geometry.contentHeight <= geometry.viewportHeight || geometry.trackHeight <= 0.0F)
    {
        return geometry;
    }

    geometry.trackRect = {
        geometry.containerRect.x + geometry.containerRect.width - components::ScrollArea::SCROLLBAR_TRACK_WIDTH -
            components::ScrollArea::SCROLLBAR_TRACK_PADDING,
        geometry.containerRect.y, components::ScrollArea::SCROLLBAR_TRACK_WIDTH, geometry.trackHeight};

    const float visibleRatio = geometry.viewportHeight / geometry.contentHeight;
    geometry.thumbHeight = std::min(geometry.trackHeight, std::max(components::ScrollArea::SCROLLBAR_THUMB_MIN_SIZE,
                                                                   geometry.trackHeight * visibleRatio));

    const float scrollRatio =
        geometry.maxScroll > 0.0F ? std::clamp(scrollArea->scrollOffset.y() / geometry.maxScroll, 0.0F, 1.0F) : 0.0F;
    const float thumbTravel = std::max(0.0F, geometry.trackHeight - geometry.thumbHeight);
    const float thumbTop =
        geometry.trackRect.y + (thumbTravel * scrollRatio) + components::ScrollArea::SCROLLBAR_THUMB_INSET;

    geometry.thumbRect = {
        geometry.containerRect.x + geometry.containerRect.width - components::ScrollArea::SCROLLBAR_THUMB_WIDTH -
            components::ScrollArea::SCROLLBAR_TRACK_PADDING - 1.0F,
        thumbTop, components::ScrollArea::SCROLLBAR_THUMB_WIDTH,
        std::max(0.0F, geometry.thumbHeight - (components::ScrollArea::SCROLLBAR_THUMB_INSET * 2.0F))};
    geometry.visible = true;
    return geometry;
}

void InvokeTask(VoidCallback func)
{
    auto timerSystem = systems::TimerSystem{UiRuntime::current()};
    timerSystem.addTask(0, std::move(func), true);
}
/**
 * @brief 注册一个定时任务，返回任务句柄
 * @param interval 间隔时间（毫秒）
 * @param func 任务函数
 * @return 任务句柄
 */
TaskHandle TimerCallback(uint32_t interval, VoidCallback func)
{
    auto timerSystem = systems::TimerSystem{UiRuntime::current()};
    return timerSystem.addTask(interval, std::move(func));
}

/**
 * @brief 取消注册一个定时任务
 * @param handle 任务句柄
 */
void CancelQueuedTask(TaskHandle handle)
{
    auto timerSystem = systems::TimerSystem{UiRuntime::current()};
    timerSystem.cancelTask(handle);
}
/**
 * @brief 判断实体别名是否存在
 * @param alias 实体别名
 * @return true 实体存在
 * @return false 实体不存在
 */
bool IsEntityExist(const std::string& alias)
{
    auto& reg = CurrentRegistry();
    auto view = reg.view<components::BaseInfo>();

    return std::ranges::any_of(view, [&view, &alias](entt::entity entity) -> bool
                               { return view.get<components::BaseInfo>(entity).alias == alias; });
}

}  // namespace ui::utils