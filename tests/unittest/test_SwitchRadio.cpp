/**
 * ************************************************************************
 *
 * @file test_SwitchRadio.cpp
 * @brief Switch 与 RadioGroup 单元测试
 *
 * 覆盖：Switch 组件/tag 附加与点击切换；RadioGroup 组互斥选择。
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

namespace ui::tests
{
namespace
{

class SwitchRadioTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
    }

    void TearDown() override
    {
        UiRuntime::current().dispatcher().update();
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

TEST_F(SwitchRadioTest, CreateSwitchAttachesComponents)
{
    const auto sw = factory::CreateSwitch(true, "switch");
    EXPECT_TRUE(registry().all_of<components::SwitchTag>(sw));
    EXPECT_TRUE(registry().all_of<components::FocusableTag>(sw));
    const auto* switchComp = registry().try_get<components::Switch>(sw);
    ASSERT_NE(switchComp, nullptr);
    EXPECT_TRUE(switchComp->checked);
}

TEST_F(SwitchRadioTest, SwitchClickTogglesChecked)
{
    const auto sw = factory::CreateSwitch(false, "switch");
    bool observed = false;
    registry().get<components::Switch>(sw).onChanged = [&observed](bool value) { observed = value; };

    auto* clickable = registry().try_get<components::Clickable>(sw);
    ASSERT_NE(clickable, nullptr);
    ASSERT_TRUE(clickable->onClick);

    clickable->onClick();
    EXPECT_TRUE(registry().get<components::Switch>(sw).checked);
    EXPECT_TRUE(observed);

    clickable->onClick();
    EXPECT_FALSE(registry().get<components::Switch>(sw).checked);
    EXPECT_FALSE(observed);
}

TEST_F(SwitchRadioTest, CreateRadioGroupBuildsMutuallyExclusiveOptions)
{
    const std::vector<std::string> options{"A", "B", "C"};
    const auto group = factory::CreateRadioGroup(options, 1, "group");

    EXPECT_TRUE(registry().all_of<components::RadioGroupTag>(group));
    const auto* groupComp = registry().try_get<components::RadioGroup>(group);
    ASSERT_NE(groupComp, nullptr);
    EXPECT_EQ(groupComp->selectedIndex, 1);

    // 三个子 RadioButton，仅 index=1 选中
    int radioCount = 0;
    int checkedCount = 0;
    for (const entt::entity entity : registry().view<components::RadioButton>())
    {
        const auto& rb = registry().get<components::RadioButton>(entity);
        EXPECT_EQ(rb.group, static_cast<entt::entity>(group));
        ++radioCount;
        if (rb.checked)
        {
            ++checkedCount;
        }
    }
    EXPECT_EQ(radioCount, 3);
    EXPECT_EQ(checkedCount, 1);
}

TEST_F(SwitchRadioTest, RadioGroupClickMovesSelectionToClickedOption)
{
    const std::vector<std::string> options{"A", "B", "C"};
    const auto group = factory::CreateRadioGroup(options, 0, "group");

    int observedIndex = -1;
    registry().get<components::RadioGroup>(group).onChanged = [&observedIndex](int index)
    { observedIndex = index; };

    // 找到 index=2 的 RadioButton 并点击
    entt::entity target = entt::null;
    for (const entt::entity entity : registry().view<components::RadioButton>())
    {
        if (registry().get<components::RadioButton>(entity).optionIndex == 2)
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

    EXPECT_EQ(registry().get<components::RadioGroup>(group).selectedIndex, 2);
    EXPECT_EQ(observedIndex, 2);

    // 互斥：仅 index=2 选中
    int checkedCount = 0;
    for (const entt::entity entity : registry().view<components::RadioButton>())
    {
        if (registry().get<components::RadioButton>(entity).checked)
        {
            ++checkedCount;
            EXPECT_EQ(registry().get<components::RadioButton>(entity).optionIndex, 2);
        }
    }
    EXPECT_EQ(checkedCount, 1);
}

}  // namespace
}  // namespace ui::tests
