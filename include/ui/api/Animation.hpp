/**
 * ************************************************************************
 *
 * @file Animation.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-27
 * @version 0.1
 * @brief 动画 API 封装
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <concepts>
#include <optional>
#include <type_traits>

#include "ui/Callback.hpp"
#include "ui/Color.hpp"
#include "ui/MathTypes.hpp"
#include "ui/TweenOptions.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui::animation
{
void StartPositionAnimation(ui::entity entity, const Vec2& startPos, const Vec2& endPos,
                            const TweenOptions& options = {});
void StartAlphaAnimation(ui::entity entity, float startAlpha, float endAlpha,
                         const TweenOptions& options = {});
void StartScaleAnimation(ui::entity entity, const Vec2& startScale, const Vec2& endScale,
                         const TweenOptions& options = {});
void StartRenderOffsetAnimation(ui::entity entity, const Vec2& startOffset, const Vec2& endOffset,
                                const TweenOptions& options = {});
void StartColorAnimation(ui::entity entity, const Color& startColor, const Color& endColor,
                         const TweenOptions& options = {});
void StartTransformAnimation(ui::entity entity, const std::optional<Vec2>& targetScale,
                             const std::optional<Vec2>& targetOffset, const TweenOptions& options = {},
                             const Vec2& defaultScale = {1.0F, 1.0F}, const Vec2& defaultOffset = {0.0F, 0.0F});
void StopAnimation(ui::entity entity);
void PauseAnimation(ui::entity entity);
void ResumeAnimation(ui::entity entity);
void FinishAnimation(ui::entity entity, bool settleToEnd = false);
void CancelAnimation(ui::entity entity, bool settleToEnd = false);
void SetAnimationCallbacks(ui::entity entity, ui::Callback<> onComplete,
                           ui::Callback<> onCancel = {}, ui::Callback<> onStart = {});

}  // namespace ui::animation

namespace ui::actions::animation
{
inline constexpr EntityAction<&ui::animation::StartPositionAnimation> START_POSITION_ANIMATION_ACTION{};
inline constexpr EntityAction<&ui::animation::StartAlphaAnimation> START_ALPHA_ANIMATION_ACTION{};
inline constexpr EntityAction<&ui::animation::StartScaleAnimation> START_SCALE_ANIMATION_ACTION{};
inline constexpr EntityAction<&ui::animation::StartRenderOffsetAnimation> START_RENDER_OFFSET_ANIMATION_ACTION{};
inline constexpr EntityAction<&ui::animation::StartColorAnimation> START_COLOR_ANIMATION_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, const std::optional<Vec2>&,
                                                   const std::optional<Vec2>&, const ui::animation::TweenOptions&,
                                                   const Vec2&, const Vec2&)>(
    &ui::animation::StartTransformAnimation)>
    START_TRANSFORM_ANIMATION_ACTION{};
inline constexpr EntityAction<&ui::animation::StopAnimation> STOP_ANIMATION_ACTION{};
}  // namespace ui::actions::animation

namespace ui::chains
{
inline auto StartPositionAnimation(const Vec2& startPosition, const Vec2& endPosition,
                                   const ui::animation::TweenOptions& options = {})
{
    return ui::actions::animation::START_POSITION_ANIMATION_ACTION.bind(startPosition, endPosition, options);
}

inline auto StartAlphaAnimation(float startAlpha, float endAlpha, const ui::animation::TweenOptions& options = {})
{
    return ui::actions::animation::START_ALPHA_ANIMATION_ACTION.bind(startAlpha, endAlpha, options);
}

inline auto StartScaleAnimation(const Vec2& startScale, const Vec2& endScale,
                                const ui::animation::TweenOptions& options = {})
{
    return ui::actions::animation::START_SCALE_ANIMATION_ACTION.bind(startScale, endScale, options);
}

inline auto StartRenderOffsetAnimation(const Vec2& startOffset, const Vec2& endOffset,
                                       const ui::animation::TweenOptions& options = {})
{
    return ui::actions::animation::START_RENDER_OFFSET_ANIMATION_ACTION.bind(startOffset, endOffset, options);
}

inline auto StartColorAnimation(const Color& startColor, const Color& endColor,
                                const ui::animation::TweenOptions& options = {})
{
    return ui::actions::animation::START_COLOR_ANIMATION_ACTION.bind(startColor, endColor, options);
}

inline auto StartTransformAnimation(const std::optional<Vec2>& targetScale, const std::optional<Vec2>& targetOffset,
                                    const ui::animation::TweenOptions& options = {},
                                    const Vec2& defaultScale = {1.0F, 1.0F}, const Vec2& defaultOffset = {0.0F, 0.0F})
{
    return ui::actions::animation::START_TRANSFORM_ANIMATION_ACTION.bind(targetScale, targetOffset, options,
                                                                         defaultScale, defaultOffset);
}

inline auto StopAnimation()
{
    return ui::actions::animation::STOP_ANIMATION_ACTION.bind();
}
}  // namespace ui::chains