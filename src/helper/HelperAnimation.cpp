/**
 * ************************************************************************
 *
 * @file HelperAnimation.cpp
 * @brief ui::detail::animation 动画辅助函数实现（非 inline）
 *
 * 这些函数原本是 Helper.hpp 中的 inline 实现，每个包含 Helper.hpp 的 TU
 * 都会实例化大量 entt storage 模板（get_or_emplace / try_get / remove），
 * 显著抬高编译内存峰值（clang-cl 在低内存环境下曾触发 LLVM OOM）。
 * 拆到本 .cpp 后，模板只实例化一次，编译内存大幅下降。
 *
 * ************************************************************************
 */
#include "helper/Helper.hpp"

namespace ui::detail::animation
{

void MarkRenderDirtyInternal(Registry& reg, entt::entity entity)
{
    ui::utils::MarkRenderDirty(reg.runtime(), ui::detail::ToPublic(entity));
}

void ConfigureTiming(Registry& reg, entt::entity entity, const ui::animation::TweenOptions& options)
{
    auto& time = reg.get_or_emplace<components::AnimationTime>(entity);
    time.duration = options.duration;
    time.elapsed = 0.0F;
    time.startDelayMs = options.startDelayMs;
    time.easing = options.easing;
    time.mode = options.mode;
    time.state = policies::AnimationState::PLAYING;
    time.autoCleanup = options.autoCleanup;
    reg.emplace_or_replace<components::AnimatingTag>(entity);
}

void StartPositionAnimation(Registry& reg, entt::entity entity, const Vec2& from, const Vec2& to,
                            const ui::animation::TweenOptions& options)
{
    if (!reg.valid(entity))
        return;
    auto& value = reg.get_or_emplace<components::AnimationPosition>(entity);
    value.from = from;
    value.to = to;
    ConfigureTiming(reg, entity, options);
}

void StartAlphaAnimation(Registry& reg, entt::entity entity, float from, float to, const ui::animation::TweenOptions& options)
{
    if (!reg.valid(entity))
        return;
    auto& value = reg.get_or_emplace<components::AnimationAlpha>(entity);
    value.from = from;
    value.to = to;
    ConfigureTiming(reg, entity, options);
}

void StartScaleAnimation(Registry& reg, entt::entity entity, const Vec2& from, const Vec2& to,
                         const ui::animation::TweenOptions& options)
{
    if (!reg.valid(entity))
        return;
    auto& value = reg.get_or_emplace<components::AnimationScale>(entity);
    value.from = from;
    value.to = to;
    ConfigureTiming(reg, entity, options);
}

void StartRenderOffsetAnimation(Registry& reg, entt::entity entity, const Vec2& from, const Vec2& to,
                                const ui::animation::TweenOptions& options)
{
    if (!reg.valid(entity))
        return;
    auto& value = reg.get_or_emplace<components::AnimationRenderOffset>(entity);
    value.from = from;
    value.to = to;
    ConfigureTiming(reg, entity, options);
}

void StartColorAnimation(Registry& reg, entt::entity entity, const Color& from, const Color& to,
                         const ui::animation::TweenOptions& options)
{
    if (!reg.valid(entity))
        return;
    auto& value = reg.get_or_emplace<components::AnimationColor>(entity);
    value.from = from;
    value.to = to;
    ConfigureTiming(reg, entity, options);
}

void StartTransformAnimation(Registry& reg, entt::entity entity, const std::optional<Vec2>& targetScale,
                             const std::optional<Vec2>& targetOffset, const ui::animation::TweenOptions& options,
                             const Vec2& defaultScale, const Vec2& defaultOffset)
{
    if (!reg.valid(entity))
        return;
    bool changed = false;
    if (targetScale)
    {
        auto& value = reg.get_or_emplace<components::AnimationScale>(entity);
        const auto* current = reg.try_get<components::Scale>(entity);
        value.from = current ? current->value : defaultScale;
        value.to = *targetScale;
        changed = true;
    }
    if (targetOffset)
    {
        auto& value = reg.get_or_emplace<components::AnimationRenderOffset>(entity);
        const auto* current = reg.try_get<components::RenderOffset>(entity);
        value.from = current ? current->value : defaultOffset;
        value.to = *targetOffset;
        changed = true;
    }
    if (changed)
        ConfigureTiming(reg, entity, options);
}

void StopAnimation(Registry& reg, entt::entity entity)
{
    if (!reg.valid(entity))
        return;
    // P2-4：停止视为取消，触发 onCancel（若有）
    if (auto* time = reg.try_get<components::AnimationTime>(entity); time != nullptr && time->onCancel)
    {
        auto callback = std::move(time->onCancel);
        time->onCancel = {};
        callback();
    }
    reg.remove<components::AnimatingTag>(entity);
    reg.remove<components::AnimationTime>(entity);
    reg.remove<components::AnimationPosition>(entity);
    reg.remove<components::AnimationAlpha>(entity);
    reg.remove<components::AnimationScale>(entity);
    reg.remove<components::AnimationRenderOffset>(entity);
    reg.remove<components::AnimationColor>(entity);
}

// ==================== P2-4：暂停/恢复/完成/取消 + 回调 ====================

void PauseAnimation(Registry& reg, entt::entity entity)
{
    if (auto* time = reg.try_get<components::AnimationTime>(entity);
        time != nullptr && time->state == policies::AnimationState::PLAYING)
    {
        time->state = policies::AnimationState::PAUSED;
    }
}

void ResumeAnimation(Registry& reg, entt::entity entity)
{
    if (auto* time = reg.try_get<components::AnimationTime>(entity);
        time != nullptr && time->state == policies::AnimationState::PAUSED)
    {
        time->state = policies::AnimationState::PLAYING;
    }
}

/// 立即跳到终值（settle=true）并完成；否则等同 StopAnimation（取消）。
void FinishAnimation(Registry& reg, entt::entity entity, bool settleToEnd)
{
    if (!reg.valid(entity))
        return;
    auto* time = reg.try_get<components::AnimationTime>(entity);
    if (time == nullptr)
        return;

    if (settleToEnd)
    {
        // 以 val=1.0 写终值（复用 TweenSystem 的更新逻辑不可行——这里是 helper，
        // 直接手动写各属性 to 值）。
        time->elapsed = time->duration;
        if (auto* pos = reg.try_get<components::AnimationPosition>(entity); pos != nullptr)
        {
            if (auto* comp = reg.try_get<components::Position>(entity); comp != nullptr)
            {
                comp->value = pos->to;
                ui::utils::MarkLayoutDirty(reg.runtime(), ui::detail::ToPublic(entity));
            }
        }
        if (auto* alpha = reg.try_get<components::AnimationAlpha>(entity); alpha != nullptr)
        {
            const float value = alpha->to;
            if (auto* comp = reg.try_get<components::Alpha>(entity); comp != nullptr)
            {
                comp->value = value;
            }
            if (auto* bg = reg.try_get<components::Background>(entity); bg != nullptr)
            {
                bg->color.alpha = value;
            }
        }
        if (auto* scale = reg.try_get<components::AnimationScale>(entity); scale != nullptr)
        {
            reg.get_or_emplace<components::Scale>(entity).value = scale->to;
        }
        if (auto* offset = reg.try_get<components::AnimationRenderOffset>(entity); offset != nullptr)
        {
            if (auto* comp = reg.try_get<components::RenderOffset>(entity); comp != nullptr)
            {
                comp->value = offset->to;
            }
        }
        if (auto* color = reg.try_get<components::AnimationColor>(entity); color != nullptr)
        {
            if (auto* bg = reg.try_get<components::Background>(entity); bg != nullptr)
            {
                bg->color = color->to;
            }
        }
        MarkRenderDirtyInternal(reg, entity);
    }
    else if (time->onCancel)
    {
        auto callback = std::move(time->onCancel);
        time->onCancel = {};
        callback();
    }

    // 触发 onComplete（settle 或已处于完成态）
    if (settleToEnd)
    {
        time->state = policies::AnimationState::STOPPED;
    }
    // 统一走系统清理：保留组件让 TweenSystem 的 finishAnimation 触发 onComplete。
    // 若 settle 已把状态置 STOPPED，下一帧系统会完成；此处直接调用辅助清理。
    if (time->onComplete)
    {
        auto callback = std::move(time->onComplete);
        time->onComplete = {};
        callback();
    }
    reg.remove<components::AnimatingTag>(entity);
    if (time->autoCleanup)
    {
        reg.remove<components::AnimationPosition>(entity);
        reg.remove<components::AnimationAlpha>(entity);
        reg.remove<components::AnimationScale>(entity);
        reg.remove<components::AnimationRenderOffset>(entity);
        reg.remove<components::AnimationColor>(entity);
        reg.remove<components::AnimationTime>(entity);
    }
}

/// 取消动画：settleToEnd=true 时先跳终值并触发 onComplete；false 触发 onCancel 并清理。
void CancelAnimation(Registry& reg, entt::entity entity, bool settleToEnd)
{
    FinishAnimation(reg, entity, settleToEnd);
}

/// 设置动画生命周期回调（onComplete/onCancel/onStart）。
void SetAnimationCallbacks(Registry& reg, entt::entity entity, ui::Callback<> onComplete, ui::Callback<> onCancel,
                           ui::Callback<> onStart)
{
    auto* time = reg.try_get<components::AnimationTime>(entity);
    if (time == nullptr)
        return;
    time->onComplete = std::move(onComplete);
    time->onCancel = std::move(onCancel);
    time->onStart = std::move(onStart);
}

}  // namespace ui::detail::animation