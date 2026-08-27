/**
 * ************************************************************************
 *
 * @file test_Tooltip.cpp
 * @brief Tooltip 单元测试
 *
 * 覆盖：SetTooltip 附加组件/tag/Hoverable、hover 回调调度延迟显示、
 * unhover 回调清除悬停状态。
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

class TooltipTest : public ::testing::Test
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

TEST_F(TooltipTest, SetTooltipAttachesComponents)
{
    const auto buttonResult = factory::CreateButton("btn", "tooltip_btn");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;
    factory::SetTooltip(button, "help text", 300);

    const auto* tooltip = registry().try_get<components::Tooltip>(button);
    ASSERT_NE(tooltip, nullptr);
    EXPECT_EQ(tooltip->text, "help text");
    EXPECT_EQ(tooltip->delayMs, 300);
    EXPECT_TRUE(registry().all_of<components::TooltipTag>(button));
    EXPECT_NE(registry().try_get<components::Hoverable>(button), nullptr);
}

TEST_F(TooltipTest, HoverCallbackSchedulesDelayedDisplay)
{
    const auto buttonResult = factory::CreateButton("btn", "tooltip_btn");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;
    factory::SetTooltip(button, "help text", 300);

    auto* hoverable = registry().try_get<components::Hoverable>(button);
    ASSERT_NE(hoverable, nullptr);
    ASSERT_TRUE(hoverable->onHover);
    hoverable->onHover();

    const auto* tooltip = registry().try_get<components::Tooltip>(button);
    ASSERT_NE(tooltip, nullptr);
    EXPECT_TRUE(tooltip->hovered);
    EXPECT_NE(tooltip->pendingTask, 0U);
}

TEST_F(TooltipTest, UnhoverCallbackClearsHoveredState)
{
    const auto buttonResult = factory::CreateButton("btn", "tooltip_btn");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;
    factory::SetTooltip(button, "help text", 300);

    auto* hoverable = registry().try_get<components::Hoverable>(button);
    ASSERT_NE(hoverable, nullptr);
    ASSERT_TRUE(hoverable->onHover);
    ASSERT_TRUE(hoverable->onUnhover);

    hoverable->onHover();
    hoverable->onUnhover();

    const auto* tooltip = registry().try_get<components::Tooltip>(button);
    ASSERT_NE(tooltip, nullptr);
    EXPECT_FALSE(tooltip->hovered);
    // 显式转换避免 gtest 打印 null_t（会触发其 operator long long → entt_traits<long long> 未定义）
    EXPECT_EQ(tooltip->popupEntity, static_cast<entt::entity>(entt::null));
}

}  // namespace
}  // namespace ui::tests
