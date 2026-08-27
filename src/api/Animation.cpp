#include "ui/api/Animation.hpp"

#include "helper/Helper.hpp"

namespace ui::animation
{

void StartPositionAnimation(UiRuntime& runtime, ui::entity entity, const Vec2& startPos, const Vec2& endPos, const TweenOptions& options)
{
    ui::detail::animation::StartPositionAnimation(runtime.registry(), ui::detail::ToInternal(entity), startPos, endPos, options);
}

void StartAlphaAnimation(UiRuntime& runtime, ui::entity entity, float startAlpha, float endAlpha, const TweenOptions& options)
{
    ui::detail::animation::StartAlphaAnimation(runtime.registry(), ui::detail::ToInternal(entity), startAlpha, endAlpha, options);
}

void StartScaleAnimation(UiRuntime& runtime, ui::entity entity, const Vec2& startScale, const Vec2& endScale, const TweenOptions& options)
{
    ui::detail::animation::StartScaleAnimation(runtime.registry(), ui::detail::ToInternal(entity), startScale, endScale, options);
}

void StartRenderOffsetAnimation(UiRuntime& runtime, ui::entity entity, const Vec2& startOffset, const Vec2& endOffset,
                                const TweenOptions& options)
{
    ui::detail::animation::StartRenderOffsetAnimation(runtime.registry(), ui::detail::ToInternal(entity), startOffset, endOffset, options);
}

void StartColorAnimation(UiRuntime& runtime, ui::entity entity, const Color& startColor, const Color& endColor, const TweenOptions& options)
{
    ui::detail::animation::StartColorAnimation(runtime.registry(), ui::detail::ToInternal(entity), startColor, endColor, options);
}

void StartTransformAnimation(UiRuntime& runtime, ui::entity entity, const std::optional<Vec2>& targetScale,
                             const std::optional<Vec2>& targetOffset, const TweenOptions& options,
                             const Vec2& defaultScale, const Vec2& defaultOffset)
{
    ui::detail::animation::StartTransformAnimation(runtime.registry(), ui::detail::ToInternal(entity), targetScale, targetOffset, options,
                                                   defaultScale, defaultOffset);
}

void StopAnimation(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::animation::StopAnimation(runtime.registry(), ui::detail::ToInternal(entity));
}

void PauseAnimation(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::animation::PauseAnimation(runtime.registry(), ui::detail::ToInternal(entity));
}

void ResumeAnimation(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::animation::ResumeAnimation(runtime.registry(), ui::detail::ToInternal(entity));
}

void FinishAnimation(UiRuntime& runtime, ui::entity entity, bool settleToEnd)
{
    ui::detail::animation::FinishAnimation(runtime.registry(), ui::detail::ToInternal(entity), settleToEnd);
}

void CancelAnimation(UiRuntime& runtime, ui::entity entity, bool settleToEnd)
{
    ui::detail::animation::CancelAnimation(runtime.registry(), ui::detail::ToInternal(entity), settleToEnd);
}

void SetAnimationCallbacks(UiRuntime& runtime, ui::entity entity, ui::Callback<> onComplete, ui::Callback<> onCancel,
                           ui::Callback<> onStart)
{
    ui::detail::animation::SetAnimationCallbacks(runtime.registry(), ui::detail::ToInternal(entity), std::move(onComplete),
                                                 std::move(onCancel), std::move(onStart));
}

}  // namespace ui::animation
