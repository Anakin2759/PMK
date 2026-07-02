#include <gtest/gtest.h>

#include <memory>
#include <ui.hpp>

#include <entt/entt.hpp>
#include "common/components/Layout.hpp"
#include "common/components/Visual.hpp"
#include "src/api/Visibility.hpp"
#include "src/common/Policies.hpp"
#include "src/common/Tags.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/core/WindowSync.hpp"

#include <limits>

namespace ui::tests
{

namespace
{

class VisibilityTest : public ::testing::Test
{
protected:
    void SetUp() override { m_scope = std::make_unique<UiRuntimeScope>(m_runtime); }
    void TearDown() override { m_scope.reset(); }

private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

} // namespace

// ===================== Show / Hide =====================

TEST_F(VisibilityTest, ShowAddsVisibleTag)
{
    const auto entity = factory::CreateLabel("Lbl", "vis_show_1");
    auto& registry = UiRuntime::current().registry();
    registry.remove<components::VisibleTag>(entity);

    visibility::Show(entity);

    EXPECT_TRUE(registry.all_of<components::VisibleTag>(entity));
}

TEST_F(VisibilityTest, ShowTriggersDirtyMarks)
{
    const auto entity = factory::CreateLabel("Lbl", "vis_show_dirty");
    auto& registry = UiRuntime::current().registry();
    registry.remove<components::LayoutDirtyTag>(entity);
    registry.remove<components::RenderDirtyTag>(entity);

    visibility::Show(entity);

    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(entity));
    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(entity));
}

TEST_F(VisibilityTest, HideRemovesVisibleTag)
{
    const auto entity = factory::CreateLabel("Lbl", "vis_hide_1");
    auto& registry = UiRuntime::current().registry();
    registry.emplace_or_replace<components::VisibleTag>(entity);

    visibility::Hide(entity);

    EXPECT_FALSE(registry.all_of<components::VisibleTag>(entity));
}

TEST_F(VisibilityTest, HideTriggersDirtyMarks)
{
    const auto entity = factory::CreateLabel("Lbl", "vis_hide_dirty");
    auto& registry = UiRuntime::current().registry();
    registry.remove<components::LayoutDirtyTag>(entity);
    registry.remove<components::RenderDirtyTag>(entity);

    visibility::Hide(entity);

    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(entity));
    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(entity));
}

TEST_F(VisibilityTest, SetVisibleTrueEquivalentToShow)
{
    const auto entity = factory::CreateLabel("Lbl", "vis_set_true");
    auto& registry = UiRuntime::current().registry();
    registry.remove<components::VisibleTag>(entity);

    visibility::SetVisible(entity, true);

    EXPECT_TRUE(registry.all_of<components::VisibleTag>(entity));
}

TEST_F(VisibilityTest, SetVisibleFalseEquivalentToHide)
{
    const auto entity = factory::CreateLabel("Lbl", "vis_set_false");
    auto& registry = UiRuntime::current().registry();
    registry.emplace_or_replace<components::VisibleTag>(entity);

    visibility::SetVisible(entity, false);

    EXPECT_FALSE(registry.all_of<components::VisibleTag>(entity));
}

TEST_F(VisibilityTest, ShowOnInvalidEntityIsNoOp)
{
    EXPECT_NO_FATAL_FAILURE(visibility::Show(entt::null));
}

TEST_F(VisibilityTest, HideOnInvalidEntityIsNoOp)
{
    EXPECT_NO_FATAL_FAILURE(visibility::Hide(entt::null));
}

TEST_F(VisibilityTest, WindowSizeTargetUsesPositiveEcsSize)
{
    components::Size size{};
    size.size = Eigen::Vector2f{160.0F, 300.0F};

    int width = 0;
    int height = 0;

    EXPECT_TRUE(window_sync::TryGetWindowSizeTarget(size, width, height));
    EXPECT_EQ(width, 160);
    EXPECT_EQ(height, 300);
}

TEST_F(VisibilityTest, WindowSizeTargetRejectsNonPositiveSize)
{
    components::Size size{};
    int width = 0;
    int height = 0;

    size.size = Eigen::Vector2f{0.0F, 300.0F};
    EXPECT_FALSE(window_sync::TryGetWindowSizeTarget(size, width, height));

    size.size = Eigen::Vector2f{160.0F, -1.0F};
    EXPECT_FALSE(window_sync::TryGetWindowSizeTarget(size, width, height));
}

TEST_F(VisibilityTest, WindowSizeTargetRejectsNonFiniteSize)
{
    components::Size size{};
    int width = 0;
    int height = 0;

    size.size = Eigen::Vector2f{std::numeric_limits<float>::infinity(), 300.0F};
    EXPECT_FALSE(window_sync::TryGetWindowSizeTarget(size, width, height));

    size.size = Eigen::Vector2f{160.0F, std::numeric_limits<float>::quiet_NaN()};
    EXPECT_FALSE(window_sync::TryGetWindowSizeTarget(size, width, height));
}

// ===================== SetAlpha =====================

TEST_F(VisibilityTest, SetAlphaStoresValue)
{
    const auto entity = factory::CreateLabel("A", "vis_alpha_1");
    auto& registry = UiRuntime::current().registry();

    visibility::SetAlpha(entity, 0.5F);

    const auto* alpha = registry.try_get<components::Alpha>(entity);
    ASSERT_NE(alpha, nullptr);
    EXPECT_FLOAT_EQ(alpha->value, 0.5F);
}

TEST_F(VisibilityTest, SetAlphaClampsBelowZero)
{
    const auto entity = factory::CreateLabel("A", "vis_alpha_clamp_low");
    auto& registry = UiRuntime::current().registry();

    visibility::SetAlpha(entity, -0.5F);

    const auto* alpha = registry.try_get<components::Alpha>(entity);
    ASSERT_NE(alpha, nullptr);
    EXPECT_FLOAT_EQ(alpha->value, 0.0F);
}

TEST_F(VisibilityTest, SetAlphaClampsAboveOne)
{
    const auto entity = factory::CreateLabel("A", "vis_alpha_clamp_high");
    auto& registry = UiRuntime::current().registry();

    visibility::SetAlpha(entity, 1.8F);

    const auto* alpha = registry.try_get<components::Alpha>(entity);
    ASSERT_NE(alpha, nullptr);
    EXPECT_FLOAT_EQ(alpha->value, 1.0F);
}

TEST_F(VisibilityTest, SetAlphaOnInvalidEntityIsNoOp)
{
    EXPECT_NO_FATAL_FAILURE(visibility::SetAlpha(entt::null, 0.5F));
}

// ===================== SetBackgroundColor =====================

TEST_F(VisibilityTest, SetBackgroundColorStoresColorAndEnablesBackground)
{
    const auto entity = factory::CreateLabel("B", "vis_bg_1");
    const Color red{1.0F, 0.0F, 0.0F, 1.0F};
    auto& registry = UiRuntime::current().registry();

    visibility::SetBackgroundColor(entity, red);

    const auto* background = registry.try_get<components::Background>(entity);
    ASSERT_NE(background, nullptr);
    EXPECT_EQ(background->color.red, 1.0F);
    EXPECT_EQ(background->color.green, 0.0F);
    EXPECT_EQ(background->color.blue, 0.0F);
    EXPECT_EQ(background->enabled, policies::Feature::ENABLED);
}

TEST_F(VisibilityTest, SetBackgroundColorTriggersRenderDirty)
{
    const auto entity = factory::CreateLabel("B", "vis_bg_dirty");
    auto& registry = UiRuntime::current().registry();
    registry.remove<components::RenderDirtyTag>(entity);

    visibility::SetBackgroundColor(entity, {0.2F, 0.4F, 0.6F, 1.0F});

    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(entity));
}

// ===================== SetBorderRadius =====================

TEST_F(VisibilityTest, SetBorderRadiusAppliesUniformRadiusToBackground)
{
    const auto entity = factory::CreateLabel("R", "vis_radius_1");
    auto& registry = UiRuntime::current().registry();

    visibility::SetBorderRadius(entity, 8.0F);

    const auto* background = registry.try_get<components::Background>(entity);
    ASSERT_NE(background, nullptr);
    EXPECT_FLOAT_EQ(background->borderRadius.x(), 8.0F);
    EXPECT_FLOAT_EQ(background->borderRadius.y(), 8.0F);
    EXPECT_FLOAT_EQ(background->borderRadius.z(), 8.0F);
    EXPECT_FLOAT_EQ(background->borderRadius.w(), 8.0F);
}

TEST_F(VisibilityTest, SetBorderRadiusNegativeValueClampsToZero)
{
    const auto entity = factory::CreateLabel("R", "vis_radius_clamp");
    auto& registry = UiRuntime::current().registry();

    visibility::SetBorderRadius(entity, -5.0F);

    const auto* background = registry.try_get<components::Background>(entity);
    ASSERT_NE(background, nullptr);
    EXPECT_FLOAT_EQ(background->borderRadius.x(), 0.0F);
}

// ===================== SetBorderColor =====================

TEST_F(VisibilityTest, SetBorderColorStoresColorAndEnablesBorder)
{
    const auto entity = factory::CreateLabel("BC", "vis_border_color");
    const Color blue{0.0F, 0.0F, 1.0F, 1.0F};
    auto& registry = UiRuntime::current().registry();

    visibility::SetBorderColor(entity, blue);

    const auto* border = registry.try_get<components::Border>(entity);
    ASSERT_NE(border, nullptr);
    EXPECT_FLOAT_EQ(border->color.blue, 1.0F);
    EXPECT_EQ(border->enabled, policies::Feature::ENABLED);
}

// ===================== SetBorderThickness =====================

TEST_F(VisibilityTest, SetBorderThicknessStoresValueAndEnablesBorder)
{
    const auto entity = factory::CreateLabel("BT", "vis_border_thick");
    auto& registry = UiRuntime::current().registry();

    visibility::SetBorderThickness(entity, 3.0F);

    const auto* border = registry.try_get<components::Border>(entity);
    ASSERT_NE(border, nullptr);
    EXPECT_FLOAT_EQ(border->thickness, 3.0F);
    EXPECT_EQ(border->enabled, policies::Feature::ENABLED);
}

} // namespace ui::tests
