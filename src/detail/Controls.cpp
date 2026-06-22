#include "ControlsBridge.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <string>
#include "Utils.hpp"
#include "common/Scale.hpp"
#include "core/RuntimeFacade.hpp"
#include "entt/entity/fwd.hpp"
#include "common/components/Data.hpp"
#include "common/Policies.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Interaction.hpp"
#include "common/Types.hpp"

namespace
{
namespace controls_impl
{
using namespace ui;

namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}
} // namespace

void SetSliderRange(::entt::entity entity, float minValue, float maxValue)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.minValue = std::min(minValue, maxValue);
    slider.maxValue = std::max(minValue, maxValue);
    slider.currentValue = std::clamp(slider.currentValue, slider.minValue, slider.maxValue);

    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetSliderValue(::entt::entity entity, float value)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    const float clamped = std::clamp(value, slider.minValue, slider.maxValue);
    if (std::abs(slider.currentValue - clamped) < 0.0001F) return;

    slider.currentValue = clamped;
    if (slider.onValueChanged)
    {
        slider.onValueChanged(slider.currentValue);
    }

    ui::utils::MarkVisualChanged(entity);
}

void SetSliderStep(::entt::entity entity, float step)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.step = std::max(0.0F, step);
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderOrientation(::entt::entity entity, policies::Orientation orientation)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.vertical = orientation;

    auto& size = reg.get_or_emplace<components::Size>(entity);
    if (orientation == policies::Orientation::VERTICAL)
    {
        size.size = {scale::Metric(28.0F), scale::Metric(200.0F)};
    }
    else
    {
        size.size = {scale::Metric(200.0F), scale::Metric(28.0F)};
    }
    size.sizePolicy = ui::policies::Size::FIXED;

    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetSliderOnValueChanged(::entt::entity entity, components::on_event<float> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.onValueChanged = std::move(callback);
}

void SetSliderTrackColor(::entt::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).trackColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderFillColor(::entt::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).fillColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderThumbColor(::entt::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).thumbColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderThumbSize(::entt::entity entity, float size)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.thumbSize = std::max(scale::Metric(4.0F), scale::Metric(size));
    slider.thumbRadius = slider.thumbSize * 0.5F; // 保持圆形比例
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderTrackThickness(::entt::entity entity, float thickness)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).trackThickness =
        std::max(scale::Metric(2.0F), scale::Metric(thickness));
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressValue(::entt::entity entity, float progress)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.progress = std::clamp(progress, 0.0F, 1.0F);
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressFillColor(::entt::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.fillColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressBackgroundColor(::entt::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.backgroundColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressLabelVisibility(::entt::entity entity, policies::LabelVisibility visibility)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.showLabel = visibility;
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressAnimated(::entt::entity entity, policies::AnimationState animated)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.animated = animated;
    ui::utils::MarkVisualChanged(entity);
}

void SetScrollMode(::entt::entity entity, policies::Scroll mode)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.scroll = mode;
    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetScrollBarPolicy(::entt::entity entity, policies::ScrollBar policy)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.scrollBar = policy;
    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetScrollAnchor(::entt::entity entity, policies::ScrollAnchor anchor)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.anchor = anchor;
    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetScrollSpeed(::entt::entity entity, float speed)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.scrollSpeed = std::max(1.0F, speed);
    ui::utils::MarkVisualChanged(entity);
}

void SetCheckBoxChecked(::entt::entity entity, bool checked)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* checkBox = reg.try_get<components::CheckBox>(entity);
    if (checkBox == nullptr) return;
    checkBox->checked = checked;
    ui::utils::MarkVisualChanged(entity);
}

void SetCheckBoxOnChanged(::entt::entity entity, components::on_event<bool> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* checkBox = reg.try_get<components::CheckBox>(entity);
    if (checkBox == nullptr) return;
    checkBox->onChanged = std::move(callback);
}

void SetDropDownOptions(::entt::entity entity, std::vector<std::string> options)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* dropDown = reg.try_get<components::DropDown>(entity);
    if (dropDown == nullptr) return;
    dropDown->options = std::move(options);
    dropDown->selectedIndex = 0;
    if (auto* text = reg.try_get<components::Text>(entity))
    {
        text->content = dropDown->selectedText();
    }
    ui::utils::MarkVisualChanged(entity);
}

void SetDropDownSelected(::entt::entity entity, int index)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* dropDown = reg.try_get<components::DropDown>(entity);
    if (dropDown == nullptr) return;
    dropDown->selectedIndex =
        dropDown->options.empty() ? 0 : std::clamp(index, 0, static_cast<int>(dropDown->options.size()) - 1);
    if (auto* text = reg.try_get<components::Text>(entity))
    {
        text->content = dropDown->selectedText();
    }
    ui::utils::MarkVisualChanged(entity);
}

void SetDropDownOnChanged(::entt::entity entity, components::on_event<int> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* dropDown = reg.try_get<components::DropDown>(entity);
    if (dropDown == nullptr) return;
    dropDown->onChanged = std::move(callback);
}

// ─── Drag / Drop ─────────────────────────────────────────────────────────────

void SetDraggable(::entt::entity entity, bool enabled)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.enabled = enabled ? policies::Feature::ENABLED : policies::Feature::DISABLED;
}

void SetDragLockAxis(::entt::entity entity, bool lockX, bool lockY)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.lockX = lockX;
    draggable.lockY = lockY;
}

void SetOnDragStart(::entt::entity entity, components::on_event<> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.onDragStart = std::move(callback);
}

void SetOnDragEnd(::entt::entity entity, components::on_event<> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.onDragEnd = std::move(callback);
}

void SetOnDragMove(::entt::entity entity, components::on_event<Vec2> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.onDragMove = std::move(callback);
}

void SetDroppable(::entt::entity entity, bool enabled)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& droppable = reg.get_or_emplace<components::Droppable>(entity);
    droppable.enabled = enabled ? policies::Feature::ENABLED : policies::Feature::DISABLED;
}

} // namespace controls_impl
} // namespace

namespace ui::controls::bridge
{

void SetSliderRange(entt::entity entity, float minValue, float maxValue)
{
    controls_impl::SetSliderRange(entity, minValue, maxValue);
}

void SetSliderValue(entt::entity entity, float value)
{
    controls_impl::SetSliderValue(entity, value);
}

void SetSliderStep(entt::entity entity, float step)
{
    controls_impl::SetSliderStep(entity, step);
}

void SetSliderOrientation(entt::entity entity, policies::Orientation orientation)
{
    controls_impl::SetSliderOrientation(entity, orientation);
}

void SetSliderOnValueChanged(entt::entity entity, components::on_event<float> callback)
{
    controls_impl::SetSliderOnValueChanged(entity, std::move(callback));
}

void SetSliderTrackColor(entt::entity entity, const Color& color)
{
    controls_impl::SetSliderTrackColor(entity, color);
}

void SetSliderFillColor(entt::entity entity, const Color& color)
{
    controls_impl::SetSliderFillColor(entity, color);
}

void SetSliderThumbColor(entt::entity entity, const Color& color)
{
    controls_impl::SetSliderThumbColor(entity, color);
}

void SetSliderThumbSize(entt::entity entity, float size)
{
    controls_impl::SetSliderThumbSize(entity, size);
}

void SetSliderTrackThickness(entt::entity entity, float thickness)
{
    controls_impl::SetSliderTrackThickness(entity, thickness);
}

void SetProgressValue(entt::entity entity, float progress)
{
    controls_impl::SetProgressValue(entity, progress);
}

void SetProgressFillColor(entt::entity entity, const Color& color)
{
    controls_impl::SetProgressFillColor(entity, color);
}

void SetProgressBackgroundColor(entt::entity entity, const Color& color)
{
    controls_impl::SetProgressBackgroundColor(entity, color);
}

void SetProgressLabelVisibility(entt::entity entity, policies::LabelVisibility visibility)
{
    controls_impl::SetProgressLabelVisibility(entity, visibility);
}

void SetProgressAnimated(entt::entity entity, policies::AnimationState animated)
{
    controls_impl::SetProgressAnimated(entity, animated);
}

void SetScrollMode(entt::entity entity, policies::Scroll mode)
{
    controls_impl::SetScrollMode(entity, mode);
}

void SetScrollBarPolicy(entt::entity entity, policies::ScrollBar policy)
{
    controls_impl::SetScrollBarPolicy(entity, policy);
}

void SetScrollAnchor(entt::entity entity, policies::ScrollAnchor anchor)
{
    controls_impl::SetScrollAnchor(entity, anchor);
}

void SetScrollSpeed(entt::entity entity, float speed)
{
    controls_impl::SetScrollSpeed(entity, speed);
}

void SetCheckBoxChecked(entt::entity entity, bool checked)
{
    controls_impl::SetCheckBoxChecked(entity, checked);
}

void SetCheckBoxOnChanged(entt::entity entity, components::on_event<bool> callback)
{
    controls_impl::SetCheckBoxOnChanged(entity, std::move(callback));
}

void SetDropDownOptions(entt::entity entity, std::vector<std::string> options)
{
    controls_impl::SetDropDownOptions(entity, std::move(options));
}

void SetDropDownSelected(entt::entity entity, int index)
{
    controls_impl::SetDropDownSelected(entity, index);
}

void SetDropDownOnChanged(entt::entity entity, components::on_event<int> callback)
{
    controls_impl::SetDropDownOnChanged(entity, std::move(callback));
}

void SetDraggable(entt::entity entity, bool enabled)
{
    controls_impl::SetDraggable(entity, enabled);
}

void SetDragLockAxis(entt::entity entity, bool lockX, bool lockY)
{
    controls_impl::SetDragLockAxis(entity, lockX, lockY);
}

void SetOnDragStart(entt::entity entity, components::on_event<> callback)
{
    controls_impl::SetOnDragStart(entity, std::move(callback));
}

void SetOnDragEnd(entt::entity entity, components::on_event<> callback)
{
    controls_impl::SetOnDragEnd(entity, std::move(callback));
}

void SetOnDragMove(entt::entity entity, components::on_event<Vec2> callback)
{
    controls_impl::SetOnDragMove(entity, std::move(callback));
}

void SetDroppable(entt::entity entity, bool enabled)
{
    controls_impl::SetDroppable(entity, enabled);
}

} // namespace ui::controls::bridge

