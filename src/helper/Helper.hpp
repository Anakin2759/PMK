#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/CustomEvent.hpp"
#include "common/EntityTypes.hpp"
#include "ui/ErrorCodes.hpp"
#include "ui/Geometry.hpp"
#include "ui/Policies.hpp"
#include "ui/Result.hpp"
#include "ui/TweenOptions.hpp"
#include "ui/api/Scale.hpp"
#include "common/Tags.hpp"
#include "ui/api/Theme.hpp"
#include "common/Types.hpp"
#include "common/components/Data.hpp"
#include "common/components/Animation.hpp"
#include "common/components/Interaction.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Visual.hpp"
#include "common/components/Window.hpp"
#include "core/UiRuntime.hpp"
#include "core/WindowSync.hpp"
#include "SDL3/SDL_video.h"
#include "entt/entity/entity.hpp"

namespace ui
{
class UiRuntime;
}

namespace ui::utils
{
void MarkVisualChanged(ui::entity entity);
void MarkLayoutAndVisualChanged(ui::entity entity);
void MarkLayoutDirty(ui::entity entity);
void MarkRenderDirty(ui::entity entity);
}  // namespace ui::utils

namespace ui::detail
{

static_assert(sizeof(ui::entity) == sizeof(entt::entity), "ui::entity must match entt::entity storage size");
static_assert(ui::null_entity == static_cast<ui::entity>(entt::null), "ui::null_entity must match entt::null");

[[nodiscard]] constexpr entt::entity ToInternal(ui::entity entity) noexcept
{
    return static_cast<entt::entity>(entity);
}

[[nodiscard]] constexpr ui::entity ToPublic(entt::entity entity) noexcept
{
    return static_cast<ui::entity>(entity);
}

[[nodiscard]] inline ui::Result<ui::entity> ToPublic(ui::Result<entt::entity>&& result)
{
    if (!result)
    {
        return std::unexpected(std::move(result).error());
    }
    return ToPublic(*result);
}

[[nodiscard]] inline ui::Result<ui::entity> ToPublic(const ui::Result<entt::entity>& result)
{
    if (!result)
    {
        return std::unexpected(result.error());
    }
    return ToPublic(*result);
}

}  // namespace ui::detail

namespace ui::utils
{
void MarkVisualChanged(entt::entity entity);
void MarkLayoutAndVisualChanged(entt::entity entity);
void MarkLayoutDirty(entt::entity entity);
}  // namespace ui::utils

namespace ui::detail::animation
{
// 实现位于 src/helper/HelperAnimation.cpp（非 inline，摊薄 entt 模板实例化，
// 降低每个包含 Helper.hpp 的 TU 的编译内存峰值）。
void MarkRenderDirtyInternal(entt::entity entity);
void ConfigureTiming(entt::entity entity, const ui::animation::TweenOptions& options);
void StartPositionAnimation(entt::entity entity, const Vec2& from, const Vec2& to,
                            const ui::animation::TweenOptions& options = {});
void StartAlphaAnimation(entt::entity entity, float from, float to,
                         const ui::animation::TweenOptions& options = {});
void StartScaleAnimation(entt::entity entity, const Vec2& from, const Vec2& to,
                         const ui::animation::TweenOptions& options = {});
void StartRenderOffsetAnimation(entt::entity entity, const Vec2& from, const Vec2& to,
                                const ui::animation::TweenOptions& options = {});
void StartColorAnimation(entt::entity entity, const Color& from, const Color& to,
                         const ui::animation::TweenOptions& options = {});
void StartTransformAnimation(entt::entity entity, const std::optional<Vec2>& targetScale,
                             const std::optional<Vec2>& targetOffset,
                             const ui::animation::TweenOptions& options = {},
                             const Vec2& defaultScale = {1.0F, 1.0F}, const Vec2& defaultOffset = {0.0F, 0.0F});
void StopAnimation(entt::entity entity);

// ==================== P2-4：暂停/恢复/完成/取消 + 回调 ====================

void PauseAnimation(entt::entity entity);
void ResumeAnimation(entt::entity entity);
void FinishAnimation(entt::entity entity, bool settleToEnd = false);
void CancelAnimation(entt::entity entity, bool settleToEnd = false);
void SetAnimationCallbacks(entt::entity entity, ui::Callback<> onComplete, ui::Callback<> onCancel = {},
                           ui::Callback<> onStart = {});
}  // namespace ui::detail::animation

namespace ui::controls::bridge
{
inline void SetSliderRange(entt::entity e, float min, float max)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    auto& v = r.get_or_emplace<components::SliderInfo>(e);
    v.minValue = std::min(min, max);
    v.maxValue = std::max(min, max);
    v.currentValue = std::clamp(v.currentValue, v.minValue, v.maxValue);
    utils::MarkLayoutAndVisualChanged(e);
}
inline void SetSliderValue(entt::entity e, float value)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    auto& v = r.get_or_emplace<components::SliderInfo>(e);
    const float x = std::clamp(value, v.minValue, v.maxValue);
    if (std::abs(v.currentValue - x) < 0.0001F)
        return;
    v.currentValue = x;
    if (v.onValueChanged)
        v.onValueChanged(x);
    utils::MarkVisualChanged(e);
}
inline void SetSliderStep(entt::entity e, float value)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::SliderInfo>(e).step = std::max(0.0F, value);
    utils::MarkVisualChanged(e);
}
inline void SetSliderOrientation(entt::entity e, policies::Orientation value)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::SliderInfo>(e).vertical = value;
    auto& size = r.get_or_emplace<components::Size>(e);
    size.size = value == policies::Orientation::VERTICAL ? Vec2{scale::Metric(28.0F), scale::Metric(200.0F)}
                                                         : Vec2{scale::Metric(200.0F), scale::Metric(28.0F)};
    size.sizePolicy = policies::Size::FIXED;
    utils::MarkLayoutAndVisualChanged(e);
}
inline void SetSliderOnValueChanged(entt::entity e, components::on_event<float> cb)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        r.get_or_emplace<components::SliderInfo>(e).onValueChanged = std::move(cb);
}
inline void SetSliderTrackColor(entt::entity e, const Color& v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::SliderInfo>(e).trackColor = v;
    utils::MarkVisualChanged(e);
}
inline void SetSliderFillColor(entt::entity e, const Color& v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::SliderInfo>(e).fillColor = v;
    utils::MarkVisualChanged(e);
}
inline void SetSliderThumbColor(entt::entity e, const Color& v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::SliderInfo>(e).thumbColor = v;
    utils::MarkVisualChanged(e);
}
inline void SetSliderThumbSize(entt::entity e, float v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    auto& x = r.get_or_emplace<components::SliderInfo>(e);
    x.thumbSize = std::max(scale::Metric(4.0F), scale::Metric(v));
    x.thumbRadius = x.thumbSize * 0.5F;
    utils::MarkVisualChanged(e);
}
inline void SetSliderTrackThickness(entt::entity e, float v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::SliderInfo>(e).trackThickness = std::max(scale::Metric(2.0F), scale::Metric(v));
    utils::MarkVisualChanged(e);
}
inline void SetProgressValue(entt::entity e, float v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ProgressBar>(e).progress = std::clamp(v, 0.0F, 1.0F);
    utils::MarkVisualChanged(e);
}
inline void SetProgressFillColor(entt::entity e, const Color& v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ProgressBar>(e).fillColor = v;
    utils::MarkVisualChanged(e);
}
inline void SetProgressBackgroundColor(entt::entity e, const Color& v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ProgressBar>(e).backgroundColor = v;
    utils::MarkVisualChanged(e);
}
inline void SetProgressLabelVisibility(entt::entity e, policies::LabelVisibility v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ProgressBar>(e).showLabel = v;
    utils::MarkVisualChanged(e);
}
inline void SetProgressAnimated(entt::entity e, policies::AnimationState v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ProgressBar>(e).animated = v;
    utils::MarkVisualChanged(e);
}
inline void SetScrollMode(entt::entity e, policies::Scroll v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ScrollArea>(e).scroll = v;
    utils::MarkLayoutAndVisualChanged(e);
}
inline void SetScrollBarPolicy(entt::entity e, policies::ScrollBar v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ScrollArea>(e).scrollBar = v;
    utils::MarkLayoutAndVisualChanged(e);
}
inline void SetScrollAnchor(entt::entity e, policies::ScrollAnchor v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ScrollArea>(e).anchor = v;
    utils::MarkLayoutAndVisualChanged(e);
}
inline void SetScrollSpeed(entt::entity e, float v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    r.get_or_emplace<components::ScrollArea>(e).scrollSpeed = std::max(1.0F, v);
    utils::MarkVisualChanged(e);
}
inline void SetCheckBoxChecked(entt::entity e, bool v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    if (auto* x = r.try_get<components::CheckBox>(e))
    {
        x->checked = v;
        utils::MarkVisualChanged(e);
    }
}
inline void SetCheckBoxOnChanged(entt::entity e, components::on_event<bool> cb)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        if (auto* x = r.try_get<components::CheckBox>(e))
            x->onChanged = std::move(cb);
}
inline void SetDropDownOptions(entt::entity e, std::vector<std::string> v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    if (auto* x = r.try_get<components::DropDown>(e))
    {
        x->options = std::move(v);
        x->selectedIndex = 0;
        if (auto* text = r.try_get<components::Text>(e))
            text->content = x->selectedText();
        utils::MarkVisualChanged(e);
    }
}
inline void SetDropDownSelected(entt::entity e, int v)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    if (auto* x = r.try_get<components::DropDown>(e))
    {
        x->selectedIndex = x->options.empty() ? 0 : std::clamp(v, 0, static_cast<int>(x->options.size()) - 1);
        if (auto* text = r.try_get<components::Text>(e))
            text->content = x->selectedText();
        utils::MarkVisualChanged(e);
    }
}
inline void SetDropDownOnChanged(entt::entity e, components::on_event<int> cb)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        if (auto* x = r.try_get<components::DropDown>(e))
            x->onChanged = std::move(cb);
}
inline void SetDraggable(entt::entity e, bool v)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        r.get_or_emplace<components::Draggable>(e).enabled =
            v ? policies::Feature::ENABLED : policies::Feature::DISABLED;
}
inline void SetDragLockAxis(entt::entity e, bool x, bool y)
{
    auto& r = UiRuntime::current().registry();
    if (!r.valid(e))
        return;
    auto& v = r.get_or_emplace<components::Draggable>(e);
    v.lockX = x;
    v.lockY = y;
}
inline void SetOnDragStart(entt::entity e, components::on_event<> cb)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        r.get_or_emplace<components::Draggable>(e).onDragStart = std::move(cb);
}
inline void SetOnDragEnd(entt::entity e, components::on_event<> cb)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        r.get_or_emplace<components::Draggable>(e).onDragEnd = std::move(cb);
}
inline void SetOnDragMove(entt::entity e, components::on_event<Vec2> cb)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        r.get_or_emplace<components::Draggable>(e).onDragMove = std::move(cb);
}
inline void SetDroppable(entt::entity e, bool v)
{
    auto& r = UiRuntime::current().registry();
    if (r.valid(e))
        r.get_or_emplace<components::Droppable>(e).enabled =
            v ? policies::Feature::ENABLED : policies::Feature::DISABLED;
}
}  // namespace ui::controls::bridge

namespace ui::detail::text
{
inline void SetText(entt::entity entity, const std::string& content)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity) || !reg.any_of<components::Text>(entity))
        return;
    reg.get<components::Text>(entity).content = content;
    utils::MarkLayoutDirty(entity);
}

inline void SetButtonEnabled(entt::entity entity, bool enabled)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (enabled)
        reg.remove<components::DisabledTag>(entity);
    else
        reg.emplace_or_replace<components::DisabledTag>(entity);
}

inline void SetTextContent(entt::entity entity, const std::string& content)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).content = content;
    utils::MarkLayoutDirty(entity);
}

inline void SetTextWordWrap(entt::entity entity, policies::TextWrap mode)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).wordWrap = mode;
    utils::MarkLayoutDirty(entity);
}

inline void SetTextAlignment(entt::entity entity, policies::Alignment alignment)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).alignment = alignment;
    utils::MarkLayoutDirty(entity);
}

inline void SetTextColor(entt::entity entity, const Color& color)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (auto* text = reg.try_get<components::Text>(entity))
        text->color = color;
    if (auto* edit = reg.try_get<components::TextEdit>(entity))
        edit->textColor = color;
}

[[nodiscard]] inline std::string GetTextEditContent(entt::entity entity)
{
    auto& reg = UiRuntime::current().registry();
    if (reg.valid(entity))
    {
        if (const auto* edit = reg.try_get<components::TextEdit>(entity))
            return edit->buffer;
    }
    return {};
}

inline void SetTextEditContent(entt::entity entity, const std::string& content)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (auto* edit = reg.try_get<components::TextEdit>(entity))
    {
        edit->buffer = content;
        edit->cursorPosition = std::min(edit->cursorPosition, content.size());
        edit->hasSelection = false;
        edit->selectionStart = 0;
        edit->selectionEnd = 0;
    }
}

inline void SetPasswordMode(entt::entity entity, policies::TextFlag mode)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (auto* edit = reg.try_get<components::TextEdit>(entity))
        edit->inputMode |= mode;
}

inline void SetClickCallback(entt::entity entity, components::on_event<> callback)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    auto& clickable = reg.get_or_emplace<components::Clickable>(entity);
    clickable.onClick = std::move(callback);
    clickable.enabled = policies::Feature::ENABLED;
}

inline void SetOnSubmit(entt::entity entity, components::on_event<> callback)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (auto* edit = reg.try_get<components::TextEdit>(entity))
        edit->onSubmit = std::move(callback);
}

inline void SetOnTextChanged(entt::entity entity, components::on_event<const std::string&> callback)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (auto* edit = reg.try_get<components::TextEdit>(entity))
        edit->onTextChanged = std::move(callback);
}

inline void SetLineHeight(entt::entity entity, float height)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).lineHeight = scale::Metric(height);
    utils::MarkLayoutDirty(entity);
}

inline void SetCharacterSpacing(entt::entity entity, float spacing)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).letterSpacing = scale::Metric(spacing);
    utils::MarkLayoutDirty(entity);
}

inline void SetTextWrapWidth(entt::entity entity, float width)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).wrapWidth = scale::Metric(width);
    utils::MarkLayoutDirty(entity);
}

inline void SetFontSize(entt::entity entity, float size)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Text>(entity).fontSize = scale::Metric(size);
    utils::MarkLayoutDirty(entity);
}
}  // namespace ui::detail::text

namespace ui::detail::table
{
inline void SetColumns(entt::entity e, int count, std::vector<std::string> headers = {})
{
    auto& v = UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e);
    v.columnCount = count;
    v.headers = std::move(headers);
    for (auto& row : v.cells)
        row.resize(static_cast<size_t>(count));
}
inline void SetColumnWidths(entt::entity e, std::vector<float> values)
{
    auto& v = UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e);
    if (v.columnSizing == policies::TableColumnSizing::FIXED || v.columnSizing == policies::TableColumnSizing::ADAPTIVE)
        for (auto& x : values)
            x = scale::Metric(x);
    v.columnWidths = std::move(values);
}
inline void AddRow(entt::entity e, std::vector<std::string> values)
{
    auto& v = UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e);
    std::vector<components::TableCell> row(static_cast<size_t>(v.columnCount));
    for (int i = 0; i < v.columnCount && std::cmp_less(i, values.size()); ++i)
        row[static_cast<size_t>(i)].text = std::move(values[static_cast<size_t>(i)]);
    v.cells.push_back(std::move(row));
}
inline void SetCell(entt::entity e, int row, int col, std::string value)
{
    auto* v = UiRuntime::current().registry().try_get<components::TableInfo>(e);
    if (v && row >= 0 && std::cmp_less(row, v->cells.size()) && col >= 0 && col < v->columnCount)
        v->cells[static_cast<size_t>(row)][static_cast<size_t>(col)].text = std::move(value);
}
inline void SetCellColor(entt::entity e, int row, int col, Color text, Color bg)
{
    auto* v = UiRuntime::current().registry().try_get<components::TableInfo>(e);
    if (v && row >= 0 && std::cmp_less(row, v->cells.size()) && col >= 0 && col < v->columnCount)
    {
        auto& c = v->cells[static_cast<size_t>(row)][static_cast<size_t>(col)];
        c.textColor = text;
        c.bgColor = bg;
    }
}
inline void ClearRows(entt::entity e)
{
    if (auto* v = UiRuntime::current().registry().try_get<components::TableInfo>(e))
    {
        v->cells.clear();
        v->selectedRow = -1;
    }
}
inline void SetSelectedRow(entt::entity e, int v)
{
    UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e).selectedRow = v;
}
inline void SetHeaderTextColor(entt::entity e, Color v)
{
    UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e).headerTextColor = v;
}
inline void SetColumnSizing(entt::entity e, policies::TableColumnSizing v)
{
    UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e).columnSizing = v;
}
inline void SetMinColumnWidths(entt::entity e, std::vector<float> v)
{
    for (auto& x : v)
        x = scale::Metric(x);
    UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e).minColumnWidths = std::move(v);
}
inline void SetMinRowHeight(entt::entity e, float v)
{
    UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e).minRowHeight =
        std::max(0.0F, scale::Metric(v));
}
inline void SetRowHeight(entt::entity e, float v)
{
    UiRuntime::current().registry().get_or_emplace<components::TableInfo>(e).rowHeight =
        std::max(0.0F, scale::Metric(v));
}
[[nodiscard]] inline float MinColumnWidthAt(const components::TableInfo& info, int columnIndex)
{
    if (std::cmp_less(columnIndex, info.minColumnWidths.size()))
    {
        return std::max(0.0F, info.minColumnWidths.at(static_cast<size_t>(columnIndex)));
    }
    return 0.0F;
}

[[nodiscard]] inline std::vector<float> ComputeEqualColumnWidths(const components::TableInfo& info, float visibleWidth)
{
    std::vector<float> widths(static_cast<size_t>(info.columnCount), 0.0F);
    const float equalWidth = visibleWidth / static_cast<float>(info.columnCount);
    for (int columnIndex = 0; columnIndex < info.columnCount; ++columnIndex)
    {
        widths.at(static_cast<size_t>(columnIndex)) = std::max(equalWidth, MinColumnWidthAt(info, columnIndex));
    }
    return widths;
}

[[nodiscard]] inline std::vector<float> ComputeFixedColumnWidths(const components::TableInfo& info, float visibleWidth)
{
    if (!std::cmp_equal(info.columnWidths.size(), info.columnCount))
    {
        return ComputeEqualColumnWidths(info, visibleWidth);
    }
    std::vector<float> widths(static_cast<size_t>(info.columnCount), 0.0F);
    for (int columnIndex = 0; columnIndex < info.columnCount; ++columnIndex)
    {
        widths.at(static_cast<size_t>(columnIndex)) =
            std::max(info.columnWidths.at(static_cast<size_t>(columnIndex)), MinColumnWidthAt(info, columnIndex));
    }
    return widths;
}

[[nodiscard]] inline float TotalColumnWeight(const components::TableInfo& info)
{
    float totalWeight = 0.0F;
    if (std::cmp_equal(info.columnWidths.size(), info.columnCount))
    {
        for (const float weight : info.columnWidths)
            totalWeight += std::max(0.0F, weight);
    }
    return totalWeight > 0.0F ? totalWeight : static_cast<float>(info.columnCount);
}

[[nodiscard]] inline std::vector<float> ComputeProportionalColumnWidths(const components::TableInfo& info,
                                                                        float visibleWidth)
{
    const float totalWeight = TotalColumnWeight(info);
    std::vector<float> widths(static_cast<size_t>(info.columnCount), 0.0F);
    for (int columnIndex = 0; columnIndex < info.columnCount; ++columnIndex)
    {
        float weight = 1.0F;
        if (std::cmp_less(columnIndex, info.columnWidths.size()))
        {
            weight = std::max(0.0F, info.columnWidths.at(static_cast<size_t>(columnIndex)));
        }
        widths.at(static_cast<size_t>(columnIndex)) =
            std::max((weight / totalWeight) * visibleWidth, MinColumnWidthAt(info, columnIndex));
    }
    return widths;
}

[[nodiscard]] inline std::vector<float> ComputeAdaptiveColumnWidths(const components::TableInfo& info,
                                                                    float visibleWidth)
{
    std::vector<float> widths(static_cast<size_t>(info.columnCount), 0.0F);
    float fixedTotal = 0.0F;
    int flexCount = 0;
    for (int columnIndex = 0; columnIndex < info.columnCount; ++columnIndex)
    {
        const float fixedWidth = std::cmp_less(columnIndex, info.columnWidths.size())
                                     ? info.columnWidths.at(static_cast<size_t>(columnIndex))
                                     : 0.0F;
        if (fixedWidth > 0.0F)
        {
            widths.at(static_cast<size_t>(columnIndex)) = std::max(fixedWidth, MinColumnWidthAt(info, columnIndex));
            fixedTotal += widths.at(static_cast<size_t>(columnIndex));
        }
        else
        {
            ++flexCount;
        }
    }
    const float remainingWidth = std::max(0.0F, visibleWidth - fixedTotal);
    const float flexWidth = flexCount > 0 ? remainingWidth / static_cast<float>(flexCount) : 0.0F;
    for (int columnIndex = 0; columnIndex < info.columnCount; ++columnIndex)
    {
        const bool fixed = std::cmp_less(columnIndex, info.columnWidths.size()) &&
                           info.columnWidths.at(static_cast<size_t>(columnIndex)) > 0.0F;
        if (!fixed)
        {
            widths.at(static_cast<size_t>(columnIndex)) = std::max(flexWidth, MinColumnWidthAt(info, columnIndex));
        }
    }
    return widths;
}

[[nodiscard]] inline std::vector<float> ComputeColumnWidths(const components::TableInfo& info, float tableWidth)
{
    if (info.columnCount <= 0)
        return {};
    const float visibleWidth = std::max(0.0F, tableWidth);
    switch (info.columnSizing)
    {
        case policies::TableColumnSizing::FIXED:
            return ComputeFixedColumnWidths(info, visibleWidth);
        case policies::TableColumnSizing::PROPORTIONAL:
            return ComputeProportionalColumnWidths(info, visibleWidth);
        case policies::TableColumnSizing::ADAPTIVE:
            return ComputeAdaptiveColumnWidths(info, visibleWidth);
        case policies::TableColumnSizing::EQUAL:
        default:
            return ComputeEqualColumnWidths(info, visibleWidth);
    }
}

inline void SetCellWidget(entt::entity tableEntity, int row, int col, entt::entity widgetEntity)
{
    auto& reg = UiRuntime::current().registry();
    auto* info = reg.try_get<components::TableInfo>(tableEntity);
    if (info == nullptr || row < 0 || !std::cmp_less(row, info->cells.size()) || col < 0 || col >= info->columnCount ||
        !reg.valid(widgetEntity))
        return;

    auto& cell = info->cells.at(static_cast<size_t>(row)).at(static_cast<size_t>(col));
    if (cell.cellEntity != entt::null && cell.cellEntity != widgetEntity && reg.valid(cell.cellEntity))
    {
        if (auto* hierarchy = reg.try_get<components::Hierarchy>(tableEntity))
        {
            std::erase(hierarchy->children, cell.cellEntity);
        }
        if (auto* hierarchy = reg.try_get<components::Hierarchy>(cell.cellEntity))
            hierarchy->parent = entt::null;
    }

    cell.cellEntity = widgetEntity;
    reg.emplace_or_replace<components::TableCellWidgetTag>(widgetEntity);
    auto& children = reg.get_or_emplace<components::Hierarchy>(tableEntity).children;
    if (std::ranges::find(children, widgetEntity) == children.end())
    {
        reg.get_or_emplace<components::Hierarchy>(widgetEntity).parent = tableEntity;
        reg.remove<components::RootTag>(widgetEntity);
        children.push_back(widgetEntity);
    }
}
}  // namespace ui::detail::table

namespace ui::utils
{

bool HasAlignment(policies::Alignment value, policies::Alignment flag);
void SetWindowFlag(ui::entity entity, policies::WindowFlag flag);
void MarkLayoutChanged(ui::entity entity);
void MarkVisualChanged(ui::entity entity);
void MarkLayoutAndVisualChanged(ui::entity entity);
void MarkLayoutDirty(ui::entity entity);
void CloseWindow(ui::entity entity);
void QuitUiEventLoop();
[[nodiscard]] Vec2 GetAbsolutePosition(ui::entity entity);
[[nodiscard]] Rect GetEntityRect(ui::entity entity);
[[nodiscard]] Rect GetScrollViewportRect(ui::entity entity);
[[nodiscard]] float GetScrollViewportLength(ui::entity entity, bool isVertical);
[[nodiscard]] float GetScrollContentLength(ui::entity entity, bool isVertical);
[[nodiscard]] float GetScrollMaxOffset(ui::entity entity, bool isVertical);
[[nodiscard]] VerticalScrollbarGeometry GetVerticalScrollbarGeometry(ui::entity entity);
void InvokeTask(UiRuntime& runtime, VoidCallback func);
using TaskHandle = uint32_t;
TaskHandle TimerCallback(UiRuntime& runtime, uint32_t interval, VoidCallback func);
void CancelQueuedTask(UiRuntime& runtime, TaskHandle handle);
bool IsEntityExist(const std::string& alias);

inline void SetWindowFlag(entt::entity entity, policies::WindowFlag flag)
{
    SetWindowFlag(detail::ToPublic(entity), flag);
}

inline void MarkLayoutChanged(entt::entity entity)
{
    MarkLayoutChanged(detail::ToPublic(entity));
}

inline void MarkVisualChanged(entt::entity entity)
{
    MarkVisualChanged(static_cast<ui::entity>(entity));
}

inline void MarkLayoutAndVisualChanged(entt::entity entity)
{
    MarkLayoutAndVisualChanged(static_cast<ui::entity>(entity));
}

inline void MarkLayoutDirty(entt::entity entity)
{
    MarkLayoutDirty(detail::ToPublic(entity));
}

inline void MarkRenderDirty(entt::entity entity)
{
    MarkRenderDirty(detail::ToPublic(entity));
}

[[nodiscard]] inline Vec2 GetAbsolutePosition(entt::entity entity)
{
    return GetAbsolutePosition(detail::ToPublic(entity));
}

[[nodiscard]] inline Rect GetEntityRect(entt::entity entity)
{
    return GetEntityRect(detail::ToPublic(entity));
}

[[nodiscard]] inline Rect GetScrollViewportRect(entt::entity entity)
{
    return GetScrollViewportRect(detail::ToPublic(entity));
}

[[nodiscard]] inline float GetScrollViewportLength(entt::entity entity, bool isVertical)
{
    return GetScrollViewportLength(detail::ToPublic(entity), isVertical);
}

[[nodiscard]] inline float GetScrollContentLength(entt::entity entity, bool isVertical)
{
    return GetScrollContentLength(detail::ToPublic(entity), isVertical);
}

[[nodiscard]] inline float GetScrollMaxOffset(entt::entity entity, bool isVertical)
{
    return GetScrollMaxOffset(detail::ToPublic(entity), isVertical);
}

[[nodiscard]] inline VerticalScrollbarGeometry GetVerticalScrollbarGeometry(entt::entity entity)
{
    return GetVerticalScrollbarGeometry(detail::ToPublic(entity));
}

}  // namespace ui::utils

namespace ui::detail::size
{

inline void SetFixedSize(entt::entity entity, float width, float height)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;

    auto& size = reg.get_or_emplace<components::Size>(entity);
    size.sizePolicy = policies::Size::FIXED;
    size.size = {scale::Metric(width), scale::Metric(height)};
    utils::MarkLayoutDirty(entity);
}

inline void SetSizePolicy(entt::entity entity, policies::Size policy)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Size>(entity).sizePolicy = policy;
    utils::MarkLayoutDirty(entity);
}

inline void SetSize(entt::entity entity, float width, float height)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Size>(entity).size = {scale::Metric(width), scale::Metric(height)};
    utils::MarkLayoutDirty(entity);
}

inline void SetPosition(entt::entity entity, float positionX, float positionY)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Position>(entity).value = {scale::Metric(positionX), scale::Metric(positionY)};
    utils::MarkLayoutDirty(entity);
}

}  // namespace ui::detail::size

namespace ui::detail::visibility
{

inline void SetVisible(entt::entity entity, bool visible)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (visible)
    {
        reg.emplace_or_replace<components::VisibleTag>(entity);
    }
    else
    {
        reg.remove<components::VisibleTag>(entity);
    }
    utils::MarkLayoutAndVisualChanged(entity);
}

inline void Show(entt::entity entity)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.emplace_or_replace<components::VisibleTag>(entity);
    auto* windowComp = reg.try_get<components::Window>(entity);
    if (windowComp != nullptr && windowComp->windowID != 0)
    {
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp->windowID);
        if (sdlWindow != nullptr)
        {
            window_sync::SyncWindowProperties(entity, *windowComp, sdlWindow);
            SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            SDL_ShowWindow(sdlWindow);
            int posX = 0;
            int posY = 0;
            SDL_GetWindowPosition(sdlWindow, &posX, &posY);
            if (auto* position = reg.try_get<components::Position>(entity))
            {
                position->value = Vec2{static_cast<float>(posX), static_cast<float>(posY)};
            }
        }
    }
    utils::MarkLayoutAndVisualChanged(entity);
}

inline void Hide(entt::entity entity)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.remove<components::VisibleTag>(entity);
    auto* windowComp = reg.try_get<components::Window>(entity);
    if (windowComp != nullptr && windowComp->windowID != 0)
    {
        if (SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp->windowID))
            SDL_HideWindow(sdlWindow);
    }
    utils::MarkLayoutAndVisualChanged(entity);
}

inline void SetAlpha(entt::entity entity, float alpha)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Alpha>(entity).value = std::clamp(alpha, 0.0F, 1.0F);
    utils::MarkVisualChanged(entity);
}

inline void SetBackgroundColor(entt::entity entity, const Color& color)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    auto& background = reg.get_or_emplace<components::Background>(entity);
    background.color = color;
    background.enabled = policies::Feature::ENABLED;
    utils::MarkVisualChanged(entity);
}

inline void SetBorderRadius(entt::entity entity, float radius)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    auto& background = reg.get_or_emplace<components::Background>(entity);
    const auto CLAMPED = std::max(0.0F, scale::Metric(radius));
    background.borderRadius = {CLAMPED, CLAMPED, CLAMPED, CLAMPED};
    background.enabled = policies::Feature::ENABLED;
    if (auto* border = reg.try_get<components::Border>(entity))
    {
        border->borderRadius = {CLAMPED, CLAMPED, CLAMPED, CLAMPED};
    }
    utils::MarkVisualChanged(entity);
}

inline void SetBorderColor(entt::entity entity, const Color& color)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    auto& border = reg.get_or_emplace<components::Border>(entity);
    border.color = color;
    border.enabled = policies::Feature::ENABLED;
    utils::MarkVisualChanged(entity);
}

inline void SetBorderThickness(entt::entity entity, float thickness)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    auto& border = reg.get_or_emplace<components::Border>(entity);
    border.thickness = scale::Metric(thickness);
    border.enabled = policies::Feature::ENABLED;
    utils::MarkVisualChanged(entity);
}

}  // namespace ui::detail::visibility

namespace ui::detail::layout
{

inline void SetLayoutDirection(entt::entity entity, policies::LayoutDirection direction)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::LayoutInfo>(entity).direction = direction;
    utils::MarkLayoutDirty(entity);
}

inline void SetLayoutSpacing(entt::entity entity, float spacing)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (auto* layout = reg.try_get<components::LayoutInfo>(entity))
    {
        layout->spacing = std::max(0.0F, scale::Metric(spacing));
        utils::MarkLayoutDirty(entity);
    }
}

inline void SetPadding(entt::entity entity, float left, float top, float right, float bottom)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::Padding>(entity).values =
        Vec4(scale::Metric(top), scale::Metric(right), scale::Metric(bottom), scale::Metric(left));
    utils::MarkLayoutDirty(entity);
}

inline void SetPadding(entt::entity entity, float padding)
{
    SetPadding(entity, padding, padding, padding, padding);
}

inline void CenterInParent(entt::entity entity)
{
    utils::MarkLayoutDirty(entity);
}

}  // namespace ui::detail::layout

namespace ui::detail::hierarchy
{

// NOLINTNEXTLINE(misc-no-recursion)
inline void AppendChildrenPostOrder(Registry& reg, entt::entity parent, std::vector<entt::entity>& output)
{
    if (!reg.valid(parent))
        return;
    const auto* hierarchy = reg.try_get<components::Hierarchy>(parent);
    if (hierarchy == nullptr || hierarchy->children.empty())
        return;
    auto childrenCopy = hierarchy->children;
    for (const entt::entity CHILD : childrenCopy)
    {
        if (!reg.valid(CHILD))
            continue;
        AppendChildrenPostOrder(reg, CHILD, output);
        output.push_back(CHILD);
    }
}

inline void RemoveChild(entt::entity parent, entt::entity child)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(parent) || !reg.valid(child))
        return;
    auto* parentHierarchy = reg.try_get<components::Hierarchy>(parent);
    auto* childHierarchy = reg.try_get<components::Hierarchy>(child);
    if (parentHierarchy != nullptr && childHierarchy != nullptr && childHierarchy->parent == parent)
    {
        std::erase(parentHierarchy->children, child);
        childHierarchy->parent = entt::null;
        reg.emplace_or_replace<components::RootTag>(child);
        utils::MarkLayoutAndVisualChanged(parent);
        utils::MarkLayoutAndVisualChanged(child);
    }
}

inline void AddChild(entt::entity parent, entt::entity child)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(parent) || !reg.valid(child))
        return;
    auto& childHierarchy = reg.get_or_emplace<components::Hierarchy>(child);
    if (childHierarchy.parent != entt::null && childHierarchy.parent != parent)
        RemoveChild(childHierarchy.parent, child);
    childHierarchy.parent = parent;
    reg.remove<components::RootTag>(child);
    auto& children = reg.get_or_emplace<components::Hierarchy>(parent).children;
    if (std::ranges::find(children, child) == children.end())
        children.push_back(child);
    utils::MarkLayoutAndVisualChanged(child);
}

[[nodiscard]] inline std::vector<entt::entity> ChildrenPostOrder(entt::entity parent)
{
    std::vector<entt::entity> children;
    AppendChildrenPostOrder(UiRuntime::current().registry(), parent, children);
    return children;
}

}  // namespace ui::detail::hierarchy

namespace ui::detail::icon
{

inline void SetIcon(entt::entity entity, const std::string& textureId,
                    policies::IconFlag iconflag = policies::IconFlag::DEFAULT, float iconSize = 16.0F,
                    float spacing = 4.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    (void)iconflag;
    auto& icon = reg.get_or_emplace<components::Icon>(entity);
    icon.type |= policies::IconFlag::TEXTURE;
    icon.textureId = textureId;
    icon.fontHandle = nullptr;
    icon.codepoint = 0;
    icon.size = {scale::Metric(iconSize), scale::Metric(iconSize)};
    icon.spacing = scale::Metric(spacing);
    utils::MarkLayoutAndVisualChanged(entity);
}

inline void SetIcon(entt::entity entity, const std::string& fontName, uint32_t codepoint,
                    policies::IconFlag iconflag = policies::IconFlag::DEFAULT, float iconSize = 16.0F,
                    float spacing = 4.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    (void)iconflag;
    auto& icon = reg.get_or_emplace<components::Icon>(entity);
    icon.type |= ~policies::IconFlag::TEXTURE;
    static std::unordered_set<std::string> fontNamePool;
    const auto [fontIterator, wasInserted] = fontNamePool.insert(fontName);
    (void)wasInserted;
    icon.fontHandle = fontIterator->c_str();
    icon.codepoint = codepoint;
    icon.textureId.clear();
    icon.size = {scale::Metric(iconSize), scale::Metric(iconSize)};
    icon.spacing = scale::Metric(spacing);
    utils::MarkLayoutAndVisualChanged(entity);
}

inline void RemoveIcon(entt::entity entity)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    if (reg.any_of<components::Icon>(entity))
    {
        reg.remove<components::Icon>(entity);
        utils::MarkLayoutAndVisualChanged(entity);
    }
}

}  // namespace ui::detail::icon

namespace ui::query::bridge
{

[[nodiscard]] inline bool IsValid(entt::entity entity) noexcept
{
    return UiRuntime::current().registry().valid(entity);
}

[[nodiscard]] inline Result<entt::entity> FindByAlias(std::string_view alias)
{
    if (alias.empty())
        return Err(UiErrc::INVALID_ARGUMENT, "empty alias");
    auto& reg = UiRuntime::current().registry();
    for (const auto ENTITY : reg.view<components::BaseInfo>())
    {
        if (reg.get<components::BaseInfo>(ENTITY).alias == alias)
            return ENTITY;
    }
    return Err(UiErrc::INVALID_ENTITY, std::string(alias));
}

[[nodiscard]] inline std::string GetAlias(entt::entity entity)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return {};
    const auto* info = reg.try_get<components::BaseInfo>(entity);
    return info != nullptr ? info->alias : std::string{};
}

}  // namespace ui::query::bridge

namespace ui::theme::bridge
{

[[nodiscard]] inline ThemePalette DefaultDarkTheme()
{
    return ThemePalette{};
}

inline void SetTheme(const ThemePalette& palette)
{
    auto& context = UiRuntime::current().ensureContext<ThemeContext>();
    context.previousPalette = context.palette;
    context.palette = palette;
    ++context.version;
    context.reapplyRequested = true;
}

inline void UseDefaultDarkTheme()
{
    ui::theme::bridge::SetTheme(DefaultDarkTheme());
}

inline void RequestThemeReapply()
{
    auto& context = UiRuntime::current().ensureContext<ThemeContext>();
    context.previousPalette = context.palette;
    context.reapplyRequested = true;
}

[[nodiscard]] inline const ThemePalette& CurrentTheme()
{
    return UiRuntime::current().ensureContext<ThemeContext>().palette;
}

}  // namespace ui::theme::bridge

namespace ui::detail::event_bridge
{
namespace impl
{
struct CallbackSlot
{
    std::uint64_t token = 0;
    std::shared_ptr<ui::event::EventCallback> callback;
    bool connected = true;
};

struct QueuedCustomEvent
{
    ui::event::EventId id = ui::event::INVALID_EVENT_ID;
    ui::event::EventPayload payload;
};

struct EventRegistryContext
{
    std::unordered_map<std::string, ui::event::EventId> idsByName;
    std::unordered_map<ui::event::EventId, std::string> namesById;
    std::unordered_map<ui::event::EventId, std::vector<CallbackSlot>> callbacks;
    std::unordered_map<std::uint64_t, ui::event::EventId> idsByToken;
    std::vector<QueuedCustomEvent> queue;
    std::uint64_t nextToken = 1;
    ui::event::EventId nextEventId = 1;
};

[[nodiscard]] inline EventRegistryContext& CurrentContext()
{
    return UiRuntime::current().ensureContext<EventRegistryContext>();
}

[[nodiscard]] inline CallbackSlot* FindSlot(EventRegistryContext& context, std::uint64_t token) noexcept
{
    auto idIt = context.idsByToken.find(token);
    if (idIt == context.idsByToken.end())
        return nullptr;
    auto callbacksIt = context.callbacks.find(idIt->second);
    if (callbacksIt == context.callbacks.end())
        return nullptr;
    auto& slots = callbacksIt->second;
    auto slotIt = std::ranges::find_if(slots, [token](const CallbackSlot& slot) { return slot.token == token; });
    return slotIt == slots.end() ? nullptr : &*slotIt;
}

inline void Dispatch(EventRegistryContext& context, ui::event::EventId eventId, const ui::event::EventPayload& payload)
{
    if (eventId == ui::event::INVALID_EVENT_ID)
        return;
    auto callbacksIt = context.callbacks.find(eventId);
    if (callbacksIt == context.callbacks.end())
        return;
    for (auto& slot : callbacksIt->second)
    {
        if (slot.connected && slot.callback != nullptr && static_cast<bool>(*slot.callback))
            (*slot.callback)(payload);
    }
}
}  // namespace impl

[[nodiscard]] inline ui::event::EventId RegisterEvent(std::string_view name)
{
    if (name.empty())
        return ui::event::INVALID_EVENT_ID;
    auto& context = impl::CurrentContext();
    const std::string KEY{name};
    if (auto eventIt = context.idsByName.find(KEY); eventIt != context.idsByName.end())
        return eventIt->second;
    auto eventId = context.nextEventId++;
    context.idsByName.emplace(KEY, eventId);
    context.namesById.emplace(eventId, KEY);
    return eventId;
}

[[nodiscard]] inline bool IsEventRegistered(ui::event::EventId eventId)
{
    return eventId != ui::event::INVALID_EVENT_ID && impl::CurrentContext().namesById.contains(eventId);
}

[[nodiscard]] inline bool IsEventRegistered(std::string_view name)
{
    return !name.empty() && impl::CurrentContext().idsByName.contains(std::string{name});
}

[[nodiscard]] inline std::uint64_t Connect(ui::event::EventId eventId, ui::event::EventCallback callback)
{
    if (eventId == ui::event::INVALID_EVENT_ID || !static_cast<bool>(callback))
        return 0;
    auto& context = impl::CurrentContext();
    if (!context.namesById.contains(eventId))
        return 0;
    const auto TOKEN = context.nextToken++;
    context.callbacks[eventId].push_back({.token = TOKEN,
                                          .callback = std::make_shared<ui::event::EventCallback>(std::move(callback)),
                                          .connected = true});
    context.idsByToken.emplace(TOKEN, eventId);
    return TOKEN;
}

[[nodiscard]] inline std::uint64_t Connect(std::string_view name, ui::event::EventCallback callback)
{
    return Connect(RegisterEvent(name), std::move(callback));
}

inline void Disconnect(std::uint64_t token) noexcept
{
    if (token == 0)
        return;
    auto& context = impl::CurrentContext();
    if (auto* slot = impl::FindSlot(context, token); slot != nullptr)
        slot->connected = false;
    context.idsByToken.erase(token);
}

[[nodiscard]] inline bool Connected(std::uint64_t token) noexcept
{
    if (token == 0)
        return false;
    auto& context = impl::CurrentContext();
    const auto* slot = impl::FindSlot(context, token);
    return slot != nullptr && slot->connected;
}

// NOLINTNEXTLINE(performance-unnecessary-value-param) -- 与公开 API 的按值载荷契约一致。
inline void Trigger(ui::event::EventId eventId, ui::event::EventPayload payload)
{
    impl::Dispatch(impl::CurrentContext(), eventId, payload);
}

// NOLINTNEXTLINE(performance-unnecessary-value-param) -- 与公开 API 的按值载荷契约一致。
inline void Trigger(std::string_view name, ui::event::EventPayload payload)
{
    impl::Dispatch(impl::CurrentContext(), RegisterEvent(name), payload);
}

inline void Enqueue(ui::event::EventId eventId, ui::event::EventPayload payload)
{
    if (!IsEventRegistered(eventId))
        return;
    impl::CurrentContext().queue.push_back({.id = eventId, .payload = std::move(payload)});
}

inline void Enqueue(std::string_view name, ui::event::EventPayload payload)
{
    auto eventId = RegisterEvent(name);
    if (eventId == ui::event::INVALID_EVENT_ID)
        return;
    impl::CurrentContext().queue.push_back({.id = eventId, .payload = std::move(payload)});
}

inline void DispatchQueued()
{
    auto& context = impl::CurrentContext();
    auto pending = std::exchange(context.queue, {});
    for (const auto& event : pending)
        impl::Dispatch(context, event.id, event.payload);
}

}  // namespace ui::detail::event_bridge

namespace ui::detail::canvas
{

inline void Clear(entt::entity entity)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.clear();
}

inline void DrawLine(entt::entity entity, Vec2 from, Vec2 endPos, Color color, float lineWidth = 1.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back({.type = components::CanvasDrawType::LINE,
                                                                               .p1 = scale::Metric(from),
                                                                               .p2 = scale::Metric(endPos),
                                                                               .p3 = {},
                                                                               .p4 = {},
                                                                               .color = color,
                                                                               .lineWidth = scale::Metric(lineWidth),
                                                                               .points = {}});
}

inline void DrawRect(entt::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color, float lineWidth = 1.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back({.type = components::CanvasDrawType::RECT,
                                                                               .p1 = scale::Metric(topLeft),
                                                                               .p2 = scale::Metric(bottomRight),
                                                                               .p3 = {},
                                                                               .p4 = {},
                                                                               .color = color,
                                                                               .lineWidth = scale::Metric(lineWidth),
                                                                               .points = {}});
}

inline void DrawFilledRect(entt::entity entity, Vec2 topLeft, Vec2 bottomRight, Color color)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back(
        {.type = components::CanvasDrawType::FILLED_RECT,
         .p1 = scale::Metric(topLeft),
         .p2 = scale::Metric(bottomRight),
         .p3 = {},
         .p4 = {},
         .color = color,
         .lineWidth = scale::Metric(1.0F),
         .points = {}});
}

inline void DrawCircle(entt::entity entity, Vec2 center, float radius, Color color, float lineWidth = 1.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back(
        {.type = components::CanvasDrawType::CIRCLE,
         .p1 = scale::Metric(center),
         .p2 = {scale::Metric(radius), 0.0F},
         .p3 = {},
         .p4 = {},
         .color = color,
         .lineWidth = scale::Metric(lineWidth),
         .points = {}});
}

inline void DrawFilledCircle(entt::entity entity, Vec2 center, float radius, Color color)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back(
        {.type = components::CanvasDrawType::FILLED_CIRCLE,
         .p1 = scale::Metric(center),
         .p2 = {scale::Metric(radius), 0.0F},
         .p3 = {},
         .p4 = {},
         .color = color,
         .lineWidth = scale::Metric(1.0F),
         .points = {}});
}

inline void DrawPolyline(entt::entity entity, std::vector<Vec2> points, Color color, float lineWidth = 1.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity) || points.size() < 2)
        return;
    components::CanvasDrawCommand command;
    command.type = components::CanvasDrawType::POLYLINE;
    command.color = color;
    command.lineWidth = scale::Metric(lineWidth);
    for (auto& point : points)
        point = scale::Metric(point);
    command.points = std::move(points);
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back(std::move(command));
}

inline void DrawCubicBezier(entt::entity entity, Vec2 startPos, Vec2 cp1, Vec2 cp2, Vec2 endPos, Color color,
                            float lineWidth = 1.0F)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(entity))
        return;
    components::CanvasDrawCommand command;
    command.type = components::CanvasDrawType::CUBIC_BEZIER;
    command.p1 = scale::Metric(startPos);
    command.p2 = scale::Metric(cp1);
    command.p3 = scale::Metric(cp2);
    command.p4 = scale::Metric(endPos);
    command.color = color;
    command.lineWidth = scale::Metric(lineWidth);
    reg.get_or_emplace<components::CanvasDrawList>(entity).commands.push_back(std::move(command));
}

}  // namespace ui::detail::canvas