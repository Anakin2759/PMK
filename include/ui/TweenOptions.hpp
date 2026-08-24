#pragma once

#include "ui/Policies.hpp"

namespace ui::animation
{

inline constexpr float DEFAULT_TWEEN_DURATION = 200.0F;

/**
 * @brief 动画时间、缓动和播放策略的稳定公共值类型。
 */
struct TweenOptions
{
    float duration = DEFAULT_TWEEN_DURATION;  // 动画时长（毫秒）
    float startDelayMs = 0.0F;                // 启动前延迟（毫秒）；0 无延迟
    policies::Easing easing = policies::Easing::EASE_OUT_QUAD;
    policies::Play mode = policies::Play::ONCE;
    bool autoCleanup = true;
};

}  // namespace ui::animation
