#include "Animation.hpp"
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"

namespace ui::animation
{
void StartPositionAnimation(::entt::entity entity, const Vec2& startPos, const Vec2& endPos, float duration)
{
    if (!Registry::Valid(entity)) return;

    auto& time = Registry::GetOrEmplace<components::AnimationTime>(entity);
    time.duration = duration;
    time.elapsed = 0.0F;

    auto& posAnim = Registry::GetOrEmplace<components::AnimationPosition>(entity);
    posAnim.from = startPos;
    posAnim.to = endPos;

    Registry::EmplaceOrReplace<components::AnimatingTag>(entity);
}

void StartAlphaAnimation(::entt::entity entity, float startAlpha, float endAlpha, float duration)
{
    if (!Registry::Valid(entity)) return;

    auto& time = Registry::GetOrEmplace<components::AnimationTime>(entity);
    time.duration = duration;
    time.elapsed = 0.0F;

    auto& alphaAnim = Registry::GetOrEmplace<components::AnimationAlpha>(entity);
    alphaAnim.from = startAlpha;
    alphaAnim.to = endAlpha;

    Registry::EmplaceOrReplace<components::AnimatingTag>(entity);
}

void StopAnimation(::entt::entity entity)
{
    if (!Registry::Valid(entity)) return;
    Registry::Remove<components::AnimatingTag>(entity);
}

} // namespace ui::animation
