#include <gtest/gtest.h>

#include <memory>
#include <ui.hpp>

#include "common/components/Layout.hpp"
#include "common/components/Animation.hpp"
#include "common/components/Visual.hpp"
#include "src/common/GlobalContext.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/helper/Helper.hpp"
#include "src/systems/ActionSystem.hpp"
#include "src/systems/TweenSystem.hpp"

namespace ui::tests
{
namespace
{

Registry& ActiveRegistry()
{
    return UiRuntime::current().registry();
}

void TriggerUpdate()
{
    UiRuntime::current().dispatcher().trigger<events::UpdateEvent>({});
}

void TriggerHover(ui::entity entity)
{
    UiRuntime::current().dispatcher().trigger<events::HoverEvent>({detail::ToInternal(entity)});
}

void TriggerUnhover(ui::entity entity)
{
    UiRuntime::current().dispatcher().trigger<events::UnhoverEvent>({detail::ToInternal(entity)});
}

class UiTweenSystemTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        UiRuntime::current().ensureContext<globalcontext::FrameContext>().intervalMs = 16;
    }

    void TearDown() override
    {
        m_scope.reset();
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

TEST_F(UiTweenSystemTest, PositionTweenCompletesAndCleansUp)
{
    systems::TweenSystem tweenSystem{UiRuntime::current()};
    tweenSystem.registerHandlers();

    const auto entity = factory::CreateLabel("Tween", "tween_label");
    auto& registry = ActiveRegistry();
    auto& position = registry.get<components::Position>(entity);
    position.value = {4.0F, 6.0F};

    animation::TweenOptions options;
    options.duration = 16.0F;

    animation::StartPositionAnimation(entity, position.value, {24.0F, 36.0F}, options);

    ASSERT_TRUE(registry.all_of<components::AnimatingTag>(entity));
    ASSERT_NE(registry.try_get<components::AnimationTime>(entity), nullptr);

    TriggerUpdate();

    EXPECT_FLOAT_EQ(position.value.x(), 24.0F);
    EXPECT_FLOAT_EQ(position.value.y(), 36.0F);
    EXPECT_FALSE(registry.all_of<components::AnimatingTag>(entity));
    EXPECT_EQ(registry.try_get<components::AnimationTime>(entity), nullptr);
    EXPECT_EQ(registry.try_get<components::AnimationPosition>(entity), nullptr);

    tweenSystem.unregisterHandlers();
}

TEST_F(UiTweenSystemTest, InteractiveAnimationFlowsThroughTweenPipeline)
{
    systems::ActionSystem actionSystem{UiRuntime::current()};
    systems::TweenSystem tweenSystem{UiRuntime::current()};
    actionSystem.registerHandlers();
    tweenSystem.registerHandlers();

    const auto entity = factory::CreateButton("Hover", "hover_btn");
    auto& registry = ActiveRegistry();
    auto& interaction = registry.emplace<components::InteractiveAnimation>(entity);
    interaction.hoverScale = Vec2{1.15F, 1.15F};
    interaction.hoverOffset = Vec2{0.0F, -3.0F};
    interaction.hoverDuration = 16.0F;
    interaction.normalScale = Vec2{1.0F, 1.0F};
    interaction.normalOffset = Vec2{0.0F, 0.0F};

    TriggerHover(entity);

    ASSERT_TRUE(registry.all_of<components::AnimatingTag>(entity));
    ASSERT_NE(registry.try_get<components::AnimationTime>(entity), nullptr);
    ASSERT_NE(registry.try_get<components::AnimationScale>(entity), nullptr);
    ASSERT_NE(registry.try_get<components::AnimationRenderOffset>(entity), nullptr);

    TriggerUpdate();

    const auto* scale = registry.try_get<components::Scale>(entity);
    const auto* offset = registry.try_get<components::RenderOffset>(entity);
    ASSERT_NE(scale, nullptr);
    ASSERT_NE(offset, nullptr);
    EXPECT_FLOAT_EQ(scale->value.x(), 1.15F);
    EXPECT_FLOAT_EQ(scale->value.y(), 1.15F);
    EXPECT_FLOAT_EQ(offset->value.x(), 0.0F);
    EXPECT_FLOAT_EQ(offset->value.y(), -3.0F);
    EXPECT_EQ(registry.try_get<components::AnimationTime>(entity), nullptr);

    TriggerUnhover(entity);
    ASSERT_TRUE(registry.all_of<components::AnimatingTag>(entity));

    TriggerUpdate();

    EXPECT_FLOAT_EQ(registry.get<components::Scale>(entity).value.x(), 1.0F);
    EXPECT_FLOAT_EQ(registry.get<components::Scale>(entity).value.y(), 1.0F);
    EXPECT_FLOAT_EQ(registry.get<components::RenderOffset>(entity).value.x(), 0.0F);
    EXPECT_FLOAT_EQ(registry.get<components::RenderOffset>(entity).value.y(), 0.0F);

    tweenSystem.unregisterHandlers();
    actionSystem.unregisterHandlers();
}

// Phase 2 architecture-protection test: Tween updates against the active runtime registry
// only, and do not leak across UiRuntimeScope boundaries.
TEST(UiTweenSystemRuntimeIsolationTest, TweenStaysWithinActiveRuntimeScope)
{
    UiRuntime defaultRuntime;
    UiRuntime alternateRuntime;

    {
        UiRuntimeScope const defaultScope(defaultRuntime);

        // ---- Setup entity & animation in the default runtime ----
        UiRuntime::current().ensureContext<globalcontext::FrameContext>().intervalMs = 16;

        systems::TweenSystem defaultTween{UiRuntime::current()};
        defaultTween.registerHandlers();

        const auto defaultEntity = factory::CreateLabel("Tween-Default", "tween_default");
        auto& defaultRegistry = ActiveRegistry();
        auto& defaultPos = defaultRegistry.get<components::Position>(defaultEntity);
        defaultPos.value = {0.0F, 0.0F};

        animation::TweenOptions opts;
        opts.duration = 16.0F;
        animation::StartPositionAnimation(defaultEntity, defaultPos.value, {10.0F, 20.0F}, opts);

        ASSERT_TRUE(defaultRegistry.all_of<components::AnimatingTag>(defaultEntity));

        // ---- Switch to alternate runtime: it must not see the default-runtime entity ----
        {
            UiRuntimeScope const altScope(alternateRuntime);

            // The alternate runtime is empty: facade-routed entt::registry must report so.
            EXPECT_FALSE(UiRuntime::current().registry().valid(defaultEntity));

            // Triggering UpdateEvent in the alternate runtime must NOT advance the default
            // runtime animation — the handler is bound to default's dispatcher.
            UiRuntime::current().ensureContext<globalcontext::FrameContext>().intervalMs = 16;
            TriggerUpdate();
        }

        // Default runtime entity should still be in mid-animation (nothing advanced it).
        ASSERT_TRUE(defaultRegistry.all_of<components::AnimatingTag>(defaultEntity));
        EXPECT_FLOAT_EQ(defaultPos.value.x(), 0.0F);
        EXPECT_FLOAT_EQ(defaultPos.value.y(), 0.0F);

        // Now advance the default runtime explicitly — animation should complete.
        TriggerUpdate();

        EXPECT_FLOAT_EQ(defaultPos.value.x(), 10.0F);
        EXPECT_FLOAT_EQ(defaultPos.value.y(), 20.0F);
        EXPECT_FALSE(defaultRegistry.all_of<components::AnimatingTag>(defaultEntity));

        defaultTween.unregisterHandlers();
    }
}

}  // namespace
}  // namespace ui::tests