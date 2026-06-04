#include "Controls.hpp"
#include "Scale.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <string>

#include "core/RuntimeFacade.hpp"
#include "entt/entity/fwd.hpp"
#include "common/components/Data.hpp"
#include "common/Policies.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Interaction.hpp"
#include "common/Types.hpp"

#include "Utils.hpp"

namespace ui::controls
{
namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}
} // namespace

void SetSliderRange(ui::entity entity, float minValue, float maxValue)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.minValue = std::min(minValue, maxValue);
    slider.maxValue = std::max(minValue, maxValue);
    slider.currentValue = std::clamp(slider.currentValue, slider.minValue, slider.maxValue);

    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetSliderValue(ui::entity entity, float value)
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

void SetSliderStep(ui::entity entity, float step)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.step = std::max(0.0F, step);
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderOrientation(ui::entity entity, policies::Orientation orientation)
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

void SetSliderOnValueChanged(ui::entity entity, components::on_event<float> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.onValueChanged = std::move(callback);
}

void SetSliderTrackColor(ui::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).trackColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderFillColor(ui::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).fillColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderThumbColor(ui::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).thumbColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderThumbSize(ui::entity entity, float size)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& slider = reg.get_or_emplace<components::SliderInfo>(entity);
    slider.thumbSize = std::max(scale::Metric(4.0F), scale::Metric(size));
    slider.thumbRadius = slider.thumbSize * 0.5F; // 保持圆形比例
    ui::utils::MarkVisualChanged(entity);
}

void SetSliderTrackThickness(ui::entity entity, float thickness)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    reg.get_or_emplace<components::SliderInfo>(entity).trackThickness =
        std::max(scale::Metric(2.0F), scale::Metric(thickness));
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressValue(ui::entity entity, float progress)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.progress = std::clamp(progress, 0.0F, 1.0F);
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressFillColor(ui::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.fillColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressBackgroundColor(ui::entity entity, const Color& color)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.backgroundColor = color;
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressLabelVisibility(ui::entity entity, policies::LabelVisibility visibility)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.showLabel = visibility;
    ui::utils::MarkVisualChanged(entity);
}

void SetProgressAnimated(ui::entity entity, policies::AnimationState animated)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& bar = reg.get_or_emplace<components::ProgressBar>(entity);
    bar.animated = animated;
    ui::utils::MarkVisualChanged(entity);
}

void SetScrollMode(ui::entity entity, policies::Scroll mode)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.scroll = mode;
    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetScrollBarPolicy(ui::entity entity, policies::ScrollBar policy)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.scrollBar = policy;
    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetScrollAnchor(ui::entity entity, policies::ScrollAnchor anchor)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.anchor = anchor;
    ui::utils::MarkLayoutAndVisualChanged(entity);
}

void SetScrollSpeed(ui::entity entity, float speed)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& scrollArea = reg.get_or_emplace<components::ScrollArea>(entity);
    scrollArea.scrollSpeed = std::max(1.0F, speed);
    ui::utils::MarkVisualChanged(entity);
}

void SetCheckBoxChecked(ui::entity entity, bool checked)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* checkBox = reg.try_get<components::CheckBox>(entity);
    if (checkBox == nullptr) return;
    checkBox->checked = checked;
    ui::utils::MarkVisualChanged(entity);
}

void SetCheckBoxOnChanged(ui::entity entity, components::on_event<bool> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* checkBox = reg.try_get<components::CheckBox>(entity);
    if (checkBox == nullptr) return;
    checkBox->onChanged = std::move(callback);
}

void SetDropDownOptions(ui::entity entity, std::vector<std::string> options)
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

void SetDropDownSelected(ui::entity entity, int index)
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

void SetDropDownOnChanged(ui::entity entity, components::on_event<int> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto* dropDown = reg.try_get<components::DropDown>(entity);
    if (dropDown == nullptr) return;
    dropDown->onChanged = std::move(callback);
}

// ─── Drag / Drop ─────────────────────────────────────────────────────────────

void SetDraggable(ui::entity entity, bool enabled)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.enabled = enabled ? policies::Feature::ENABLED : policies::Feature::DISABLED;
}

void SetDragLockAxis(ui::entity entity, bool lockX, bool lockY)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.lockX = lockX;
    draggable.lockY = lockY;
}

void SetOnDragStart(ui::entity entity, components::on_event<> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.onDragStart = std::move(callback);
}

void SetOnDragEnd(ui::entity entity, components::on_event<> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.onDragEnd = std::move(callback);
}

void SetOnDragMove(ui::entity entity, components::on_event<Vec2> callback)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& draggable = reg.get_or_emplace<components::Draggable>(entity);
    draggable.onDragMove = std::move(callback);
}

void SetDroppable(ui::entity entity, bool enabled)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& droppable = reg.get_or_emplace<components::Droppable>(entity);
    droppable.enabled = enabled ? policies::Feature::ENABLED : policies::Feature::DISABLED;
}

} // namespace ui::controls
