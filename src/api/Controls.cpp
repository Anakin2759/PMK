#include "Controls.hpp"

#include "detail/ControlsBridge.hpp"
#include "detail/EntityCast.hpp"

namespace ui::controls
{

void SetSliderRange(ui::entity entity, float minValue, float maxValue)
{
    bridge::SetSliderRange(detail::ToInternal(entity), minValue, maxValue);
}

void SetSliderValue(ui::entity entity, float value)
{
    bridge::SetSliderValue(detail::ToInternal(entity), value);
}

void SetSliderStep(ui::entity entity, float step)
{
    bridge::SetSliderStep(detail::ToInternal(entity), step);
}

void SetSliderOrientation(ui::entity entity, policies::Orientation orientation)
{
    bridge::SetSliderOrientation(detail::ToInternal(entity), orientation);
}

void SetSliderOnValueChanged(ui::entity entity, components::on_event<float> callback)
{
    bridge::SetSliderOnValueChanged(detail::ToInternal(entity), std::move(callback));
}

void SetSliderTrackColor(ui::entity entity, const Color& color)
{
    bridge::SetSliderTrackColor(detail::ToInternal(entity), color);
}

void SetSliderFillColor(ui::entity entity, const Color& color)
{
    bridge::SetSliderFillColor(detail::ToInternal(entity), color);
}

void SetSliderThumbColor(ui::entity entity, const Color& color)
{
    bridge::SetSliderThumbColor(detail::ToInternal(entity), color);
}

void SetSliderThumbSize(ui::entity entity, float size)
{
    bridge::SetSliderThumbSize(detail::ToInternal(entity), size);
}

void SetSliderTrackThickness(ui::entity entity, float thickness)
{
    bridge::SetSliderTrackThickness(detail::ToInternal(entity), thickness);
}

void SetProgressValue(ui::entity entity, float progress)
{
    bridge::SetProgressValue(detail::ToInternal(entity), progress);
}

void SetProgressFillColor(ui::entity entity, const Color& color)
{
    bridge::SetProgressFillColor(detail::ToInternal(entity), color);
}

void SetProgressBackgroundColor(ui::entity entity, const Color& color)
{
    bridge::SetProgressBackgroundColor(detail::ToInternal(entity), color);
}

void SetProgressLabelVisibility(ui::entity entity, policies::LabelVisibility visibility)
{
    bridge::SetProgressLabelVisibility(detail::ToInternal(entity), visibility);
}

void SetProgressAnimated(ui::entity entity, policies::AnimationState animated)
{
    bridge::SetProgressAnimated(detail::ToInternal(entity), animated);
}

void SetScrollMode(ui::entity entity, policies::Scroll mode)
{
    bridge::SetScrollMode(detail::ToInternal(entity), mode);
}

void SetScrollBarPolicy(ui::entity entity, policies::ScrollBar policy)
{
    bridge::SetScrollBarPolicy(detail::ToInternal(entity), policy);
}

void SetScrollAnchor(ui::entity entity, policies::ScrollAnchor anchor)
{
    bridge::SetScrollAnchor(detail::ToInternal(entity), anchor);
}

void SetScrollSpeed(ui::entity entity, float speed)
{
    bridge::SetScrollSpeed(detail::ToInternal(entity), speed);
}

void SetCheckBoxChecked(ui::entity entity, bool checked)
{
    bridge::SetCheckBoxChecked(detail::ToInternal(entity), checked);
}

void SetCheckBoxOnChanged(ui::entity entity, components::on_event<bool> callback)
{
    bridge::SetCheckBoxOnChanged(detail::ToInternal(entity), std::move(callback));
}

void SetDropDownOptions(ui::entity entity, std::vector<std::string> options)
{
    bridge::SetDropDownOptions(detail::ToInternal(entity), std::move(options));
}

void SetDropDownSelected(ui::entity entity, int index)
{
    bridge::SetDropDownSelected(detail::ToInternal(entity), index);
}

void SetDropDownOnChanged(ui::entity entity, components::on_event<int> callback)
{
    bridge::SetDropDownOnChanged(detail::ToInternal(entity), std::move(callback));
}

void SetDraggable(ui::entity entity, bool enabled)
{
    bridge::SetDraggable(detail::ToInternal(entity), enabled);
}

void SetDragLockAxis(ui::entity entity, bool lockX, bool lockY)
{
    bridge::SetDragLockAxis(detail::ToInternal(entity), lockX, lockY);
}

void SetOnDragStart(ui::entity entity, components::on_event<> callback)
{
    bridge::SetOnDragStart(detail::ToInternal(entity), std::move(callback));
}

void SetOnDragEnd(ui::entity entity, components::on_event<> callback)
{
    bridge::SetOnDragEnd(detail::ToInternal(entity), std::move(callback));
}

void SetOnDragMove(ui::entity entity, components::on_event<Vec2> callback)
{
    bridge::SetOnDragMove(detail::ToInternal(entity), std::move(callback));
}

void SetDroppable(ui::entity entity, bool enabled)
{
    bridge::SetDroppable(detail::ToInternal(entity), enabled);
}

} // namespace ui::controls
