#include "ui/api/Controls.hpp"

#include "helper/Helper.hpp"

namespace ui::controls
{

namespace
{
Registry& CurrentRegistry()
{
    return UiRuntime::current().registry();
}
}  // namespace

void SetSliderRange(ui::entity entity, float minValue, float maxValue)
{
    bridge::SetSliderRange(CurrentRegistry(), detail::ToInternal(entity), minValue, maxValue);
}

void SetSliderValue(ui::entity entity, float value)
{
    bridge::SetSliderValue(CurrentRegistry(), detail::ToInternal(entity), value);
}

void SetSliderStep(ui::entity entity, float step)
{
    bridge::SetSliderStep(CurrentRegistry(), detail::ToInternal(entity), step);
}

void SetSliderOrientation(ui::entity entity, policies::Orientation orientation)
{
    bridge::SetSliderOrientation(CurrentRegistry(), detail::ToInternal(entity), orientation);
}

void SetSliderOnValueChanged(ui::entity entity, Callback<float> callback)
{
    bridge::SetSliderOnValueChanged(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetSliderTrackColor(ui::entity entity, const Color& color)
{
    bridge::SetSliderTrackColor(CurrentRegistry(), detail::ToInternal(entity), color);
}

void SetSliderFillColor(ui::entity entity, const Color& color)
{
    bridge::SetSliderFillColor(CurrentRegistry(), detail::ToInternal(entity), color);
}

void SetSliderThumbColor(ui::entity entity, const Color& color)
{
    bridge::SetSliderThumbColor(CurrentRegistry(), detail::ToInternal(entity), color);
}

void SetSliderThumbSize(ui::entity entity, float size)
{
    bridge::SetSliderThumbSize(CurrentRegistry(), detail::ToInternal(entity), size);
}

void SetSliderTrackThickness(ui::entity entity, float thickness)
{
    bridge::SetSliderTrackThickness(CurrentRegistry(), detail::ToInternal(entity), thickness);
}

void SetProgressValue(ui::entity entity, float progress)
{
    bridge::SetProgressValue(CurrentRegistry(), detail::ToInternal(entity), progress);
}

void SetProgressFillColor(ui::entity entity, const Color& color)
{
    bridge::SetProgressFillColor(CurrentRegistry(), detail::ToInternal(entity), color);
}

void SetProgressBackgroundColor(ui::entity entity, const Color& color)
{
    bridge::SetProgressBackgroundColor(CurrentRegistry(), detail::ToInternal(entity), color);
}

void SetProgressLabelVisibility(ui::entity entity, policies::LabelVisibility visibility)
{
    bridge::SetProgressLabelVisibility(CurrentRegistry(), detail::ToInternal(entity), visibility);
}

void SetProgressAnimated(ui::entity entity, policies::AnimationState animated)
{
    bridge::SetProgressAnimated(CurrentRegistry(), detail::ToInternal(entity), animated);
}

void SetScrollMode(ui::entity entity, policies::Scroll mode)
{
    bridge::SetScrollMode(CurrentRegistry(), detail::ToInternal(entity), mode);
}

void SetScrollBarPolicy(ui::entity entity, policies::ScrollBar policy)
{
    bridge::SetScrollBarPolicy(CurrentRegistry(), detail::ToInternal(entity), policy);
}

void SetScrollAnchor(ui::entity entity, policies::ScrollAnchor anchor)
{
    bridge::SetScrollAnchor(CurrentRegistry(), detail::ToInternal(entity), anchor);
}

void SetScrollSpeed(ui::entity entity, float speed)
{
    bridge::SetScrollSpeed(CurrentRegistry(), detail::ToInternal(entity), speed);
}

void SetCheckBoxChecked(ui::entity entity, bool checked)
{
    bridge::SetCheckBoxChecked(CurrentRegistry(), detail::ToInternal(entity), checked);
}

void SetCheckBoxOnChanged(ui::entity entity, Callback<bool> callback)
{
    bridge::SetCheckBoxOnChanged(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetDropDownOptions(ui::entity entity, std::vector<std::string> options)
{
    bridge::SetDropDownOptions(CurrentRegistry(), detail::ToInternal(entity), std::move(options));
}

void SetDropDownSelected(ui::entity entity, int index)
{
    bridge::SetDropDownSelected(CurrentRegistry(), detail::ToInternal(entity), index);
}

void SetDropDownOnChanged(ui::entity entity, Callback<int> callback)
{
    bridge::SetDropDownOnChanged(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetDraggable(ui::entity entity, bool enabled)
{
    bridge::SetDraggable(CurrentRegistry(), detail::ToInternal(entity), enabled);
}

void SetDragLockAxis(ui::entity entity, bool lockX, bool lockY)
{
    bridge::SetDragLockAxis(CurrentRegistry(), detail::ToInternal(entity), lockX, lockY);
}

void SetOnDragStart(ui::entity entity, Callback<> callback)
{
    bridge::SetOnDragStart(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnDragEnd(ui::entity entity, Callback<> callback)
{
    bridge::SetOnDragEnd(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnDragMove(ui::entity entity, Callback<Vec2> callback)
{
    bridge::SetOnDragMove(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetDroppable(ui::entity entity, bool enabled)
{
    bridge::SetDroppable(CurrentRegistry(), detail::ToInternal(entity), enabled);
}

}  // namespace ui::controls
