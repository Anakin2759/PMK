#include "ui/api/Controls.hpp"

#include "helper/Helper.hpp"

namespace ui::controls
{

void SetSliderRange(UiRuntime& runtime, ui::entity entity, float minValue, float maxValue)
{
    bridge::SetSliderRange(runtime.registry(), detail::ToInternal(entity), minValue, maxValue);
}

void SetSliderValue(UiRuntime& runtime, ui::entity entity, float value)
{
    bridge::SetSliderValue(runtime.registry(), detail::ToInternal(entity), value);
}

void SetSliderStep(UiRuntime& runtime, ui::entity entity, float step)
{
    bridge::SetSliderStep(runtime.registry(), detail::ToInternal(entity), step);
}

void SetSliderOrientation(UiRuntime& runtime, ui::entity entity, policies::Orientation orientation)
{
    bridge::SetSliderOrientation(runtime.registry(), detail::ToInternal(entity), orientation);
}

void SetSliderOnValueChanged(UiRuntime& runtime, ui::entity entity, Callback<float> callback)
{
    bridge::SetSliderOnValueChanged(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetSliderTrackColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    bridge::SetSliderTrackColor(runtime.registry(), detail::ToInternal(entity), color);
}

void SetSliderFillColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    bridge::SetSliderFillColor(runtime.registry(), detail::ToInternal(entity), color);
}

void SetSliderThumbColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    bridge::SetSliderThumbColor(runtime.registry(), detail::ToInternal(entity), color);
}

void SetSliderThumbSize(UiRuntime& runtime, ui::entity entity, float size)
{
    bridge::SetSliderThumbSize(runtime.registry(), detail::ToInternal(entity), size);
}

void SetSliderTrackThickness(UiRuntime& runtime, ui::entity entity, float thickness)
{
    bridge::SetSliderTrackThickness(runtime.registry(), detail::ToInternal(entity), thickness);
}

void SetProgressValue(UiRuntime& runtime, ui::entity entity, float progress)
{
    bridge::SetProgressValue(runtime.registry(), detail::ToInternal(entity), progress);
}

void SetProgressFillColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    bridge::SetProgressFillColor(runtime.registry(), detail::ToInternal(entity), color);
}

void SetProgressBackgroundColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    bridge::SetProgressBackgroundColor(runtime.registry(), detail::ToInternal(entity), color);
}

void SetProgressLabelVisibility(UiRuntime& runtime, ui::entity entity, policies::LabelVisibility visibility)
{
    bridge::SetProgressLabelVisibility(runtime.registry(), detail::ToInternal(entity), visibility);
}

void SetProgressAnimated(UiRuntime& runtime, ui::entity entity, policies::AnimationState animated)
{
    bridge::SetProgressAnimated(runtime.registry(), detail::ToInternal(entity), animated);
}

void SetScrollMode(UiRuntime& runtime, ui::entity entity, policies::Scroll mode)
{
    bridge::SetScrollMode(runtime.registry(), detail::ToInternal(entity), mode);
}

void SetScrollBarPolicy(UiRuntime& runtime, ui::entity entity, policies::ScrollBar policy)
{
    bridge::SetScrollBarPolicy(runtime.registry(), detail::ToInternal(entity), policy);
}

void SetScrollAnchor(UiRuntime& runtime, ui::entity entity, policies::ScrollAnchor anchor)
{
    bridge::SetScrollAnchor(runtime.registry(), detail::ToInternal(entity), anchor);
}

void SetScrollSpeed(UiRuntime& runtime, ui::entity entity, float speed)
{
    bridge::SetScrollSpeed(runtime.registry(), detail::ToInternal(entity), speed);
}

void SetCheckBoxChecked(UiRuntime& runtime, ui::entity entity, bool checked)
{
    bridge::SetCheckBoxChecked(runtime.registry(), detail::ToInternal(entity), checked);
}

void SetCheckBoxOnChanged(UiRuntime& runtime, ui::entity entity, Callback<bool> callback)
{
    bridge::SetCheckBoxOnChanged(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetDropDownOptions(UiRuntime& runtime, ui::entity entity, std::vector<std::string> options)
{
    bridge::SetDropDownOptions(runtime.registry(), detail::ToInternal(entity), std::move(options));
}

void SetDropDownSelected(UiRuntime& runtime, ui::entity entity, int index)
{
    bridge::SetDropDownSelected(runtime.registry(), detail::ToInternal(entity), index);
}

void SetDropDownOnChanged(UiRuntime& runtime, ui::entity entity, Callback<int> callback)
{
    bridge::SetDropDownOnChanged(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetDraggable(UiRuntime& runtime, ui::entity entity, bool enabled)
{
    bridge::SetDraggable(runtime.registry(), detail::ToInternal(entity), enabled);
}

void SetDragLockAxis(UiRuntime& runtime, ui::entity entity, bool lockX, bool lockY)
{
    bridge::SetDragLockAxis(runtime.registry(), detail::ToInternal(entity), lockX, lockY);
}

void SetOnDragStart(UiRuntime& runtime, ui::entity entity, Callback<> callback)
{
    bridge::SetOnDragStart(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnDragEnd(UiRuntime& runtime, ui::entity entity, Callback<> callback)
{
    bridge::SetOnDragEnd(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnDragMove(UiRuntime& runtime, ui::entity entity, Callback<Vec2> callback)
{
    bridge::SetOnDragMove(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetDroppable(UiRuntime& runtime, ui::entity entity, bool enabled)
{
    bridge::SetDroppable(runtime.registry(), detail::ToInternal(entity), enabled);
}

}  // namespace ui::controls
