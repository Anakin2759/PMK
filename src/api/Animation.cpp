#include "Animation.hpp"

#include "detail/Animation.hpp"
#include "detail/EntityCast.hpp"

namespace ui::animation
{

void StartPositionAnimation(ui::entity entity,
                            const Vec2& startPos,
                            const Vec2& endPos,
                            const TweenOptions& options)
{
    ui::detail::animation::StartPositionAnimation(ui::detail::ToInternal(entity), startPos, endPos, options);
}

void StartAlphaAnimation(ui::entity entity, float startAlpha, float endAlpha, const TweenOptions& options)
{
    ui::detail::animation::StartAlphaAnimation(ui::detail::ToInternal(entity), startAlpha, endAlpha, options);
}

void StartScaleAnimation(ui::entity entity,
                         const Vec2& startScale,
                         const Vec2& endScale,
                         const TweenOptions& options)
{
    ui::detail::animation::StartScaleAnimation(ui::detail::ToInternal(entity), startScale, endScale, options);
}

void StartRenderOffsetAnimation(ui::entity entity,
                                const Vec2& startOffset,
                                const Vec2& endOffset,
                                const TweenOptions& options)
{
    ui::detail::animation::StartRenderOffsetAnimation(ui::detail::ToInternal(entity), startOffset, endOffset, options);
}

void StartColorAnimation(ui::entity entity,
                         const Color& startColor,
                         const Color& endColor,
                         const TweenOptions& options)
{
    ui::detail::animation::StartColorAnimation(ui::detail::ToInternal(entity), startColor, endColor, options);
}

void StartTransformAnimation(ui::entity entity,
                             const std::optional<Vec2>& targetScale,
                             const std::optional<Vec2>& targetOffset,
                             const TweenOptions& options,
                             const Vec2& defaultScale,
                             const Vec2& defaultOffset)
{
    ui::detail::animation::StartTransformAnimation(
        ui::detail::ToInternal(entity), targetScale, targetOffset, options, defaultScale, defaultOffset);
}

void StopAnimation(ui::entity entity)
{
    ui::detail::animation::StopAnimation(ui::detail::ToInternal(entity));
}

} // namespace ui::animation
