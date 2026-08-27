/**
 * ************************************************************************
 *
 * @file test_TabView.cpp
 * @brief TabView 单元测试
 *
 * 覆盖：结构（Tab 头 + 内容面板 + 互斥可见性）、点击切换、GetTabContent。
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <memory>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

#include <ui.hpp>

#include "common/Tags.hpp"
#include "common/components/Data.hpp"
#include "common/components/Interaction.hpp"
#include "common/components/Visual.hpp"

namespace ui::tests
{
namespace
{

class TabViewTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
    }

    void TearDown() override
    {
        m_scope.reset();
    }

    Registry& registry()
    {
        return m_runtime.registry();
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

TEST_F(TabViewTest, CreateTabViewBuildsHeadersAndPanels)
{
    const std::vector<std::string> titles{"Tab A", "Tab B", "Tab C"};
    const auto tabView = factory::CreateTabView(titles, "tabview");

    EXPECT_TRUE(registry().all_of<components::TabViewTag>(tabView));
    const auto* view = registry().try_get<components::TabView>(tabView);
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(view->tabHeaders.size(), 3U);
    EXPECT_EQ(view->contentPanels.size(), 3U);
    EXPECT_EQ(view->selectedIndex, 0);

    // 3 个 Tab 头，仅 index=0 选中
    int headerCount = 0;
    int selectedCount = 0;
    for (const entt::entity entity : registry().view<components::TabItem>())
    {
        const auto& item = registry().get<components::TabItem>(entity);
        EXPECT_EQ(item.owner, static_cast<entt::entity>(tabView));
        ++headerCount;
        if (item.selected)
        {
            ++selectedCount;
        }
    }
    EXPECT_EQ(headerCount, 3);
    EXPECT_EQ(selectedCount, 1);
}

TEST_F(TabViewTest, InitialVisibilityOnlySelectedPanelVisible)
{
    const std::vector<std::string> titles{"A", "B"};
    const auto tabView = factory::CreateTabView(titles, "tabview");

    const auto* view = registry().try_get<components::TabView>(tabView);
    ASSERT_NE(view, nullptr);

    EXPECT_TRUE(registry().all_of<components::VisibleTag>(view->contentPanels[0]));
    EXPECT_FALSE(registry().all_of<components::VisibleTag>(view->contentPanels[1]));
}

TEST_F(TabViewTest, ClickTabSwitchesPanelAndFiresOnChanged)
{
    const std::vector<std::string> titles{"A", "B"};
    const auto tabView = factory::CreateTabView(titles, "tabview");

    int observedIndex = -1;
    registry().get<components::TabView>(tabView).onChanged = [&observedIndex](int index) { observedIndex = index; };

    // 找到 index=1 的 Tab 头并点击
    entt::entity target = entt::null;
    for (const entt::entity entity : registry().view<components::TabItem>())
    {
        if (registry().get<components::TabItem>(entity).tabIndex == 1)
        {
            target = entity;
            break;
        }
    }
    ASSERT_NE(target, static_cast<entt::entity>(entt::null));

    auto* clickable = registry().try_get<components::Clickable>(target);
    ASSERT_NE(clickable, nullptr);
    ASSERT_TRUE(clickable->onClick);
    clickable->onClick();

    const auto* view = registry().try_get<components::TabView>(tabView);
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(view->selectedIndex, 1);
    EXPECT_EQ(observedIndex, 1);

    // 可见性翻转
    EXPECT_FALSE(registry().all_of<components::VisibleTag>(view->contentPanels[0]));
    EXPECT_TRUE(registry().all_of<components::VisibleTag>(view->contentPanels[1]));
}

TEST_F(TabViewTest, GetTabContentReturnsPanel)
{
    const std::vector<std::string> titles{"A", "B"};
    const auto tabView = factory::CreateTabView(titles, "tabview");

    const auto panel = factory::GetTabContent(tabView, 1);
    const auto* view = registry().try_get<components::TabView>(tabView);
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(static_cast<entt::entity>(panel), view->contentPanels[1]);

    // 越界返回 null
    EXPECT_EQ(factory::GetTabContent(tabView, 99), ui::null_entity);
}

}  // namespace
}  // namespace ui::tests
