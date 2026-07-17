#include <gtest/gtest.h>

#include <memory>
#include <ui.hpp>

#include <entt/entt.hpp>
#include "common/components/Data.hpp"
#include "common/components/Interaction.hpp"
#include "common/components/Layout.hpp"
#include "ui/api/Hierarchy.hpp"
#include "src/api/Utils.hpp"
#include "src/common/Tags.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

namespace ui::tests
{

namespace
{

Registry& ActiveRegistry()
{
    return UiRuntime::current().registry();
}

class UtilsTest : public ::testing::Test
{
protected:
    void SetUp() override { m_scope = std::make_unique<UiRuntimeScope>(m_runtime); }
    void TearDown() override { m_scope.reset(); }

private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

} // namespace

// ===================== MarkLayoutChanged =====================

TEST_F(UtilsTest, MarkLayoutChangedTagsEntityWithLayoutDirtyTag)
{
    const auto entity = factory::CreateLabel("L", "util_label_1");
    auto& registry = ActiveRegistry();
    registry.remove<components::LayoutDirtyTag>(entity);

    utils::MarkLayoutChanged(entity);

    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(entity));
}

TEST_F(UtilsTest, MarkLayoutChangedBubblesToParentAndGrandparent)
{
    const auto rootLayout = factory::CreateVBoxLayout("gp_layout");
    const auto nestedLayout = factory::CreateVBoxLayout("p_layout");
    const auto leafLabel = factory::CreateLabel("C", "child_layout");

    hierarchy::AddChild(rootLayout, nestedLayout);
    hierarchy::AddChild(nestedLayout, leafLabel);

    auto& registry = ActiveRegistry();
    for (auto entity : {rootLayout, nestedLayout, leafLabel})
    {
        registry.remove<components::LayoutDirtyTag>(entity);
    }

    utils::MarkLayoutChanged(leafLabel);

    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(leafLabel));
    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(nestedLayout));
    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(rootLayout));
}

TEST_F(UtilsTest, MarkLayoutChangedOnLeafDoesNotTagSibling)
{
    const auto parent = factory::CreateVBoxLayout("sib_parent");
    const auto child1 = factory::CreateLabel("C1", "child1_layout");
    const auto child2 = factory::CreateLabel("C2", "child2_layout");

    hierarchy::AddChild(parent, child1);
    hierarchy::AddChild(parent, child2);

    auto& registry = ActiveRegistry();
    for (auto entity : {parent, child1, child2})
    {
        registry.remove<components::LayoutDirtyTag>(entity);
    }

    utils::MarkLayoutChanged(child1);

    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(child1));
    EXPECT_FALSE(registry.all_of<components::LayoutDirtyTag>(child2));
}

TEST_F(UtilsTest, MarkLayoutChangedOnNullEntityIsNoOp)
{
    EXPECT_NO_FATAL_FAILURE(utils::MarkLayoutChanged(entt::null));
}

// ===================== MarkVisualChanged =====================

TEST_F(UtilsTest, MarkVisualChangedTagsEntityWithRenderDirtyTag)
{
    const auto entity = factory::CreateLabel("V", "util_vis_1");
    auto& registry = ActiveRegistry();
    registry.remove<components::RenderDirtyTag>(entity);

    utils::MarkVisualChanged(entity);

    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(entity));
}

TEST_F(UtilsTest, MarkVisualChangedAlsoTagsAncestorWindowEntity)
{
    auto& registry = ActiveRegistry();
    const auto windowEntity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::BaseInfo>(windowEntity).alias = "win_root";
    registry.emplace<components::WindowTag>(windowEntity);
    registry.emplace<components::Hierarchy>(windowEntity);

    const auto child = factory::CreateLabel("CW", "child_under_window");
    hierarchy::AddChild(windowEntity, child);

    registry.remove<components::RenderDirtyTag>(windowEntity);
    registry.remove<components::RenderDirtyTag>(child);

    utils::MarkVisualChanged(child);

    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(child));
    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(windowEntity));
}

TEST_F(UtilsTest, MarkVisualChangedOnEntityWithoutWindowAncestorOnlyTagsSelf)
{
    const auto entity = factory::CreateLabel("Alone", "util_vis_alone");
    auto& registry = ActiveRegistry();
    registry.remove<components::RenderDirtyTag>(entity);

    utils::MarkVisualChanged(entity);

    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(entity));
}

TEST_F(UtilsTest, MarkVisualChangedOnNullEntityIsNoOp)
{
    EXPECT_NO_FATAL_FAILURE(utils::MarkVisualChanged(entt::null));
}

// ===================== MarkLayoutAndVisualChanged =====================

TEST_F(UtilsTest, MarkLayoutAndVisualChangedSetsBothDirtyTags)
{
    const auto entity = factory::CreateLabel("Both", "util_both_dirty");
    auto& registry = ActiveRegistry();
    registry.remove<components::LayoutDirtyTag>(entity);
    registry.remove<components::RenderDirtyTag>(entity);

    utils::MarkLayoutAndVisualChanged(entity);

    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(entity));
    EXPECT_TRUE(registry.all_of<components::RenderDirtyTag>(entity));
}

// ===================== HasAlignment =====================

TEST_F(UtilsTest, HasAlignmentReturnsTrueForExactMatch)
{
    EXPECT_TRUE(utils::HasAlignment(policies::Alignment::LEFT, policies::Alignment::LEFT));
}

TEST_F(UtilsTest, HasAlignmentReturnsTrueForCombinedFlag)
{
    // CENTER = HCENTER | VCENTER, 测试其中一个子标志
    EXPECT_TRUE(utils::HasAlignment(policies::Alignment::CENTER, policies::Alignment::HCENTER));
    EXPECT_TRUE(utils::HasAlignment(policies::Alignment::CENTER, policies::Alignment::VCENTER));
}

TEST_F(UtilsTest, HasAlignmentReturnsFalseWhenFlagAbsent)
{
    EXPECT_FALSE(utils::HasAlignment(policies::Alignment::LEFT, policies::Alignment::RIGHT));
    EXPECT_FALSE(utils::HasAlignment(policies::Alignment::TOP, policies::Alignment::BOTTOM));
}

// ===================== VerticalScrollbarGeometry =====================

TEST_F(UtilsTest, VerticalScrollbarGeometryDefaultsToInvisibleWithoutScrollArea)
{
    auto& registry = ActiveRegistry();
    const auto entity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::Position>(entity).value = {10.0F, 20.0F};
    registry.emplace<components::Size>(entity).size = {100.0F, 80.0F};

    const auto geometry = utils::GetVerticalScrollbarGeometry(entity);

    EXPECT_FALSE(geometry.visible);
    EXPECT_FLOAT_EQ(geometry.maxScroll, 0.0F);
}

TEST_F(UtilsTest, VerticalScrollbarGeometryDefaultsToInvisibleForHorizontalScroll)
{
    auto& registry = ActiveRegistry();
    const auto entity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::Position>(entity).value = {10.0F, 20.0F};
    registry.emplace<components::Size>(entity).size = {100.0F, 80.0F};
    auto& scrollArea = registry.emplace<components::ScrollArea>(entity);
    scrollArea.scroll = policies::Scroll::HORIZONTAL;
    scrollArea.contentSize = {200.0F, 200.0F};

    EXPECT_FALSE(utils::GetVerticalScrollbarGeometry(entity).visible);
}

TEST_F(UtilsTest, VerticalScrollbarGeometryStaysInvisibleWithoutOverflow)
{
    auto& registry = ActiveRegistry();
    const auto entity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::Position>(entity);
    registry.emplace<components::Size>(entity).size = {100.0F, 80.0F};
    auto& scrollArea = registry.emplace<components::ScrollArea>(entity);
    scrollArea.contentSize = {100.0F, 80.0F};

    const auto geometry = utils::GetVerticalScrollbarGeometry(entity);

    EXPECT_FALSE(geometry.visible);
    EXPECT_FLOAT_EQ(geometry.maxScroll, 0.0F);
}

TEST_F(UtilsTest, VerticalScrollbarGeometryPreservesExistingOverflowFormula)
{
    auto& registry = ActiveRegistry();
    const auto entity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::Position>(entity).value = {10.0F, 20.0F};
    registry.emplace<components::Size>(entity).size = {100.0F, 80.0F};
    registry.emplace<components::Padding>(entity).values = {5.0F, 7.0F, 5.0F, 3.0F};
    auto& scrollArea = registry.emplace<components::ScrollArea>(entity);
    scrollArea.contentSize = {90.0F, 140.0F};
    scrollArea.scrollOffset.y() = 35.0F;

    const auto geometry = utils::GetVerticalScrollbarGeometry(entity);

    EXPECT_TRUE(geometry.visible);
    EXPECT_FLOAT_EQ(geometry.containerRect.x, 10.0F);
    EXPECT_FLOAT_EQ(geometry.containerRect.y, 20.0F);
    EXPECT_FLOAT_EQ(geometry.containerRect.width, 100.0F);
    EXPECT_FLOAT_EQ(geometry.containerRect.height, 80.0F);
    EXPECT_FLOAT_EQ(geometry.viewportRect.x, 13.0F);
    EXPECT_FLOAT_EQ(geometry.viewportRect.y, 25.0F);
    EXPECT_FLOAT_EQ(geometry.viewportRect.width, 90.0F);
    EXPECT_FLOAT_EQ(geometry.viewportRect.height, 70.0F);
    EXPECT_FLOAT_EQ(geometry.maxScroll, 70.0F);
    EXPECT_FLOAT_EQ(geometry.trackRect.x, 96.0F);
    EXPECT_FLOAT_EQ(geometry.trackRect.y, 20.0F);
    EXPECT_FLOAT_EQ(geometry.trackRect.width, 12.0F);
    EXPECT_FLOAT_EQ(geometry.trackRect.height, 80.0F);
    EXPECT_FLOAT_EQ(geometry.thumbHeight, 40.0F);
    EXPECT_FLOAT_EQ(geometry.thumbRect.x, 97.0F);
    EXPECT_FLOAT_EQ(geometry.thumbRect.y, 42.0F);
    EXPECT_FLOAT_EQ(geometry.thumbRect.width, 10.0F);
    EXPECT_FLOAT_EQ(geometry.thumbRect.height, 36.0F);
}

TEST_F(UtilsTest, VerticalScrollbarGeometryClampsThumbToSmallTrack)
{
    auto& registry = ActiveRegistry();
    const auto entity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::Position>(entity);
    registry.emplace<components::Size>(entity).size = {40.0F, 10.0F};
    auto& scrollArea = registry.emplace<components::ScrollArea>(entity);
    scrollArea.contentSize = {40.0F, 100.0F};
    scrollArea.scrollOffset.y() = 1000.0F;

    const auto geometry = utils::GetVerticalScrollbarGeometry(entity);

    EXPECT_TRUE(geometry.visible);
    EXPECT_FLOAT_EQ(geometry.thumbHeight, 10.0F);
    EXPECT_FLOAT_EQ(geometry.thumbRect.y, 2.0F);
    EXPECT_FLOAT_EQ(geometry.thumbRect.height, 6.0F);
}

TEST_F(UtilsTest, VerticalScrollbarGeometryClampsNegativeOffsetToTrackStart)
{
    auto& registry = ActiveRegistry();
    const auto entity = static_cast<ui::entity>(registry.create());
    registry.emplace<components::Position>(entity).value = {0.0F, 10.0F};
    registry.emplace<components::Size>(entity).size = {100.0F, 100.0F};
    auto& scrollArea = registry.emplace<components::ScrollArea>(entity);
    scrollArea.contentSize = {100.0F, 200.0F};
    scrollArea.scrollOffset.y() = -50.0F;

    const auto geometry = utils::GetVerticalScrollbarGeometry(entity);

    EXPECT_TRUE(geometry.visible);
    EXPECT_FLOAT_EQ(geometry.thumbRect.y, 12.0F);
}

// ===================== IsEntityExist =====================

TEST_F(UtilsTest, IsEntityExistReturnsTrueForRegisteredAlias)
{
    factory::CreateLabel("Exists", "exist_check_alias");

    EXPECT_TRUE(utils::IsEntityExist("exist_check_alias"));
}

TEST_F(UtilsTest, IsEntityExistReturnsFalseForUnknownAlias)
{
    EXPECT_FALSE(utils::IsEntityExist("totally_unknown_alias_xyz"));
}

TEST_F(UtilsTest, IsEntityExistReturnsFalseAfterRegistryClear)
{
    factory::CreateLabel("Gone", "gone_alias");
    ActiveRegistry().clear();

    EXPECT_FALSE(utils::IsEntityExist("gone_alias"));
}

} // namespace ui::tests
