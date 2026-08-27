/**
 * ************************************************************************
 *
 * @file test_AnimationLifecycle.cpp
 * @brief P2-4 动画完整化测试
 *
 * 覆盖：
 * - Pause/Resume：暂停期间 elapsed 不推进；恢复后续播
 * - onComplete：ONCE 模式完成时触发；FinishAnimation(settle=true) 也触发
 * - onCancel：StopAnimation 触发取消回调
 * - startDelayMs：延迟期内不推进
 * - SetAnimationCallbacks 设置回调
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <atomic>
#include <memory>

#include <ui.hpp>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/systems/TweenSystem.hpp"

#include "common/GlobalContext.hpp"
#include "common/Tags.hpp"
#include "common/components/Animation.hpp"
#include "common/components/Visual.hpp"

namespace ui::tests
{
namespace
{

class AnimationLifecycleTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        m_system = std::make_unique<systems::TweenSystem>(m_runtime);
        m_system->registerHandlers();
    }

    void TearDown() override
    {
        m_system->unregisterHandlers();
        m_system.reset();
        m_scope.reset();
    }

    Registry& registry()
    {
        return m_runtime.registry();
    }

    UiRuntime& runtime()
    {
        return m_runtime;
    }

    /// 驱动一帧动画（deltaMs 毫秒）
    void tick(float deltaMs)
    {
        auto& frameContext = registry().getOrEmplaceInCtx<globalcontext::FrameContext>();
        frameContext.intervalMs = deltaMs;
        UiRuntime::current().dispatcher().trigger<events::UpdateEvent>({});
    }

    /// 创建一个带 Alpha 组件的基础实体
    ui::entity makeEntity()
    {
        auto entityResult = factory::CreateBaseWidget(m_runtime, "anim_entity");
        EXPECT_TRUE(entityResult.has_value());
        if (!entityResult.has_value())
        {
            return ui::null_entity;
        }
        const auto entity = entityResult->raw;
        auto& alpha = registry().get<components::Alpha>(entity);
        alpha.value = 0.0F;
        return entity;
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
    std::unique_ptr<systems::TweenSystem> m_system;
};

TEST_F(AnimationLifecycleTest, PauseStopsProgressAndResumeContinues)
{
    auto entity = makeEntity();
    animation::StartAlphaAnimation(runtime(), entity, 0.0F, 1.0F,
                                   ui::animation::TweenOptions{.duration = 100.0F, .easing = policies::Easing::LINEAR});

    // 推进 30ms
    tick(30.0F);
    auto* time = registry().try_get<components::AnimationTime>(entity);
    ASSERT_NE(time, nullptr);
    EXPECT_NEAR(time->elapsed, 30.0F, 0.1F);

    // 暂停：再 tick 不应推进
    animation::PauseAnimation(runtime(), entity);
    EXPECT_EQ(time->state, policies::AnimationState::PAUSED);
    const float pausedElapsed = time->elapsed;
    tick(30.0F);
    EXPECT_NEAR(time->elapsed, pausedElapsed, 0.1F) << "暂停期间 elapsed 不应推进";

    // 恢复：继续推进
    animation::ResumeAnimation(runtime(), entity);
    EXPECT_EQ(time->state, policies::AnimationState::PLAYING);
    tick(30.0F);
    EXPECT_NEAR(time->elapsed, pausedElapsed + 30.0F, 0.1F) << "恢复后续播";
}

TEST_F(AnimationLifecycleTest, OnCompleteFiresWhenOnceFinishes)
{
    auto entity = makeEntity();
    std::atomic<int> completed{0};
    animation::StartAlphaAnimation(runtime(), entity, 0.0F, 1.0F,
                                   ui::animation::TweenOptions{.duration = 50.0F, .easing = policies::Easing::LINEAR});
    animation::SetAnimationCallbacks(runtime(), entity, [&completed]() { completed.fetch_add(1); });

    tick(50.0F);  // 一帧完成
    EXPECT_EQ(completed.load(), 1) << "ONCE 模式完成应触发 onComplete";
    EXPECT_FALSE(registry().any_of<components::AnimatingTag>(entity)) << "完成后移除 AnimatingTag";
}

TEST_F(AnimationLifecycleTest, StopAnimationFiresOnCancel)
{
    auto entity = makeEntity();
    std::atomic<int> cancelled{0};
    animation::StartAlphaAnimation(runtime(), entity, 0.0F, 1.0F, {});
    animation::SetAnimationCallbacks(runtime(), entity, {}, [&cancelled]() { cancelled.fetch_add(1); });

    animation::StopAnimation(runtime(), entity);
    EXPECT_EQ(cancelled.load(), 1) << "Stop 应触发 onCancel";
}

TEST_F(AnimationLifecycleTest, FinishWithSettleWritesEndValueAndFiresComplete)
{
    auto entity = makeEntity();
    std::atomic<int> completed{0};
    std::atomic<int> cancelled{0};
    animation::StartAlphaAnimation(runtime(), entity, 0.0F, 1.0F,
                                   ui::animation::TweenOptions{.duration = 1000.0F, .easing = policies::Easing::LINEAR});
    animation::SetAnimationCallbacks(runtime(), entity, [&completed]() { completed.fetch_add(1); },
                                     [&cancelled]() { cancelled.fetch_add(1); });

    // 只推进一半，然后 settle 到终值
    tick(10.0F);
    animation::FinishAnimation(runtime(), entity, true);

    auto* alpha = registry().try_get<components::Alpha>(entity);
    ASSERT_NE(alpha, nullptr);
    EXPECT_NEAR(alpha->value, 1.0F, 0.001F) << "settle 应写终值";
    EXPECT_EQ(completed.load(), 1) << "settle 完成应触发 onComplete";
    EXPECT_EQ(cancelled.load(), 0) << "settle 不应触发 onCancel";
}

TEST_F(AnimationLifecycleTest, StartDelayHoldsProgressUntilElapsed)
{
    auto entity = makeEntity();
    animation::StartAlphaAnimation(runtime(), entity, 0.0F, 1.0F,
                                   ui::animation::TweenOptions{.duration = 100.0F,
                                                               .startDelayMs = 40.0F,
                                                               .easing = policies::Easing::LINEAR});

    // 前 30ms 处于延迟期：elapsed 不推进
    tick(30.0F);
    auto* time = registry().try_get<components::AnimationTime>(entity);
    ASSERT_NE(time, nullptr);
    EXPECT_NEAR(time->elapsed, 0.0F, 0.1F) << "延迟期内不应推进";
    EXPECT_NEAR(time->startDelayMs, 10.0F, 0.1F);

    // 再 30ms：扣除剩余延迟后推进 20ms
    tick(30.0F);
    EXPECT_NEAR(time->elapsed, 20.0F, 0.1F) << "延迟扣除后应正常推进";
}

}  // namespace
}  // namespace ui::tests
