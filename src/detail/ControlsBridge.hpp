#pragma once

#include <string>
#include <vector>

#include "common/Policies.hpp"
#include "common/Types.hpp"
#include "common/components/Interaction.hpp"
#include "entt/entity/fwd.hpp"

namespace ui::controls::bridge
{

void SetSliderRange(entt::entity entity, float minValue, float maxValue);
void SetSliderValue(entt::entity entity, float value);
void SetSliderStep(entt::entity entity, float step);
void SetSliderOrientation(entt::entity entity, policies::Orientation orientation);
void SetSliderOnValueChanged(entt::entity entity, components::on_event<float> callback);
void SetSliderTrackColor(entt::entity entity, const Color& color);
void SetSliderFillColor(entt::entity entity, const Color& color);
void SetSliderThumbColor(entt::entity entity, const Color& color);
void SetSliderThumbSize(entt::entity entity, float size);
void SetSliderTrackThickness(entt::entity entity, float thickness);

void SetProgressValue(entt::entity entity, float progress);
void SetProgressFillColor(entt::entity entity, const Color& color);
void SetProgressBackgroundColor(entt::entity entity, const Color& color);
void SetProgressLabelVisibility(entt::entity entity, policies::LabelVisibility visibility);
void SetProgressAnimated(entt::entity entity, policies::AnimationState animated);

void SetScrollMode(entt::entity entity, policies::Scroll mode);
void SetScrollBarPolicy(entt::entity entity, policies::ScrollBar policy);
void SetScrollAnchor(entt::entity entity, policies::ScrollAnchor anchor);
void SetScrollSpeed(entt::entity entity, float speed);

void SetCheckBoxChecked(entt::entity entity, bool checked);
void SetCheckBoxOnChanged(entt::entity entity, components::on_event<bool> callback);

void SetDropDownOptions(entt::entity entity, std::vector<std::string> options);
void SetDropDownSelected(entt::entity entity, int index);
void SetDropDownOnChanged(entt::entity entity, components::on_event<int> callback);

void SetDraggable(entt::entity entity, bool enabled);
void SetDragLockAxis(entt::entity entity, bool lockX, bool lockY);
void SetOnDragStart(entt::entity entity, components::on_event<> callback);
void SetOnDragEnd(entt::entity entity, components::on_event<> callback);
void SetOnDragMove(entt::entity entity, components::on_event<Vec2> callback);
void SetDroppable(entt::entity entity, bool enabled);

} // namespace ui::controls::bridge
