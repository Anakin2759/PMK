#pragma once

#include "common/Policies.hpp"

namespace ui::animation
{
struct TweenOptions
{
    float duration = 200.0F;
    policies::Easing easing = policies::Easing::EASE_OUT_QUAD;
    policies::Play mode = policies::Play::ONCE;
    bool autoCleanup = true;
};
} // namespace ui::animation