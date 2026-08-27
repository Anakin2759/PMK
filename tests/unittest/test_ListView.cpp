/**
 * ************************************************************************
 *
 * @file test_ListView.cpp
 * @brief P2-1 ListView 单元测试
 *
 * 覆盖：
 * - CreateListView 构建结构（ListAreaTag/ListArea/items/选中项）
 * - 单选点击：互斥选中、回调触发、点已选项 no-op
 * - 多选点击：selectedIndices 集合维护
 * - AddListItem 动态追加
 * - 初始选中态
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

#include <ui.hpp>

#include "common/Tags.hpp"
#include "common/components/Data.hpp"
#include "common/components/Interaction.hpp"
#include "common/components/Visual.hpp"
#include "helper/Helper.hpp"

namespace ui::tests
{
namespace
{

class ListViewTest : public ::testing::Test
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

    [[nodiscard]] UiRuntime& runtime() noexcept
    {
        return m_runtime;
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

TEST_F(ListViewTest, CreateListViewBuildsStructureWithSelectedItem)
{
    auto listView = factory::CreateListView(runtime(), {"A", "B", "C"}, 1, "lv");
    ASSERT_NE(listView, ui::null_entity);

    EXPECT_TRUE(registry().any_of<components::ListAreaTag>(listView));
    auto* listArea = registry().try_get<components::ListArea>(listView);
    ASSERT_NE(listArea, nullptr);
    EXPECT_EQ(listArea->texts.size(), 3U);
    EXPECT_EQ(listArea->items.size(), 3U);
    EXPECT_EQ(listArea->selectedIndex, 1) << "初始选中 index=1";

    // 每个 item 都有组件
    for (const entt::entity item : listArea->items)
    {
        EXPECT_TRUE(registry().valid(item));
        EXPECT_TRUE(registry().any_of<components::ListAreaItemTag>(item));
        auto* itemComp = registry().try_get<components::ListAreaItem>(item);
        ASSERT_NE(itemComp, nullptr);
        EXPECT_TRUE(itemComp->owner != entt::null) << "item 应绑定所属 ListArea";
        EXPECT_TRUE(registry().any_of<components::Clickable>(item));
        EXPECT_TRUE(registry().any_of<components::Text>(item));
    }
}

TEST_F(ListViewTest, SingleSelectSwitchesSelectionAndFiresCallback)
{
    auto listView = factory::CreateListView(runtime(), {"A", "B", "C"}, -1, "lv");
    auto* listArea = registry().try_get<components::ListArea>(listView);
    ASSERT_NE(listArea, nullptr);

    std::vector<int> changed;
    listArea->onChanged = [&changed](int index) { changed.push_back(index); };

    // 点击 item B（index=1）
    const entt::entity itemB = listArea->items[1];
    auto* clickable = registry().try_get<components::Clickable>(itemB);
    ASSERT_NE(clickable, nullptr);
    clickable->onClick();

    EXPECT_EQ(listArea->selectedIndex, 1);
    ASSERT_EQ(changed.size(), 1U);
    EXPECT_EQ(changed[0], 1);

    // 点已选中项：no-op（不重复回调）
    clickable->onClick();
    EXPECT_EQ(listArea->selectedIndex, 1);
    EXPECT_EQ(changed.size(), 1U) << "点已选中项不应重复回调";

    // 切到 item A（index=0）
    const entt::entity itemA = listArea->items[0];
    registry().get<components::Clickable>(itemA).onClick();
    EXPECT_EQ(listArea->selectedIndex, 0);
    ASSERT_EQ(changed.size(), 2U);
    EXPECT_EQ(changed[1], 0);
}

TEST_F(ListViewTest, MultiSelectTogglesIndices)
{
    auto listView = factory::CreateListView(runtime(), {"A", "B", "C"}, -1, "lv");
    auto* listArea = registry().try_get<components::ListArea>(listView);
    ASSERT_NE(listArea, nullptr);
    listArea->multiSelect = policies::Selection::MULTI;

    std::vector<std::vector<int>> changes;
    listArea->onMultiChanged = [&changes](const std::vector<int>& indices) { changes.push_back(indices); };

    const auto clickItem = [&](int index)
    {
        registry().get<components::Clickable>(listArea->items[static_cast<std::size_t>(index)]).onClick();
    };

    // 选中 0、2
    clickItem(0);
    clickItem(2);
    ASSERT_EQ(changes.size(), 2U);
    EXPECT_EQ(changes[0], (std::vector<int>{0}));
    EXPECT_EQ(changes[1], (std::vector<int>{0, 2}));

    // 取消 0
    clickItem(0);
    ASSERT_EQ(changes.size(), 3U);
    EXPECT_EQ(changes[2], (std::vector<int>{2}));
    EXPECT_EQ(listArea->selectedIndices, (std::vector<int>{2}));
}

TEST_F(ListViewTest, AddListItemAppendsToExistingList)
{
    auto listView = factory::CreateListView(runtime(), {"A"}, -1, "lv");
    auto* listArea = registry().try_get<components::ListArea>(listView);
    ASSERT_NE(listArea, nullptr);

    const auto newItem = factory::AddListItem(runtime(), listView, "B", "lv_new");
    ASSERT_NE(newItem, ui::null_entity);

    EXPECT_EQ(listArea->items.size(), 2U);
    EXPECT_EQ(listArea->texts.size(), 2U);
    EXPECT_EQ(listArea->texts[1], "B");

    auto* itemComp = registry().try_get<components::ListAreaItem>(newItem);
    ASSERT_NE(itemComp, nullptr);
    EXPECT_EQ(itemComp->itemIndex, 1);

    // 新 item 可点击选中
    registry().get<components::Clickable>(newItem).onClick();
    EXPECT_EQ(listArea->selectedIndex, 1);
}

TEST_F(ListViewTest, SelectedItemGetsHighlightBackground)
{
    auto listView = factory::CreateListView(runtime(), {"A", "B"}, 0, "lv");
    auto* listArea = registry().try_get<components::ListArea>(listView);
    ASSERT_NE(listArea, nullptr);

    const entt::entity itemA = listArea->items[0];
    auto* bg = registry().try_get<components::Background>(itemA);
    ASSERT_NE(bg, nullptr);
    EXPECT_EQ(bg->enabled, policies::Feature::ENABLED) << "选中项应有背景高亮";
    EXPECT_FLOAT_EQ(bg->color.red, listArea->selectedBackground.red);

    // 切到 B 后 A 背景应禁用
    registry().get<components::Clickable>(listArea->items[1]).onClick();
    auto* bgA = registry().try_get<components::Background>(itemA);
    ASSERT_NE(bgA, nullptr);
    EXPECT_EQ(bgA->enabled, policies::Feature::DISABLED);
}

}  // namespace
}  // namespace ui::tests
