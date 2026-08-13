/**
 * ************************************************************************
 *
 * @file test_FocusNavigation.cpp
 * @brief FocusNavigationSystem 单元测试
 *
 * 覆盖：Tab/Shift+Tab 顺序导航（含环绕）、Disabled/Invisible 跳过、
 * 方向键空间导航（Up/Down/Left/Right）。
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <SDL3/SDL_keycode.h>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/systems/FocusNavigationSystem.hpp"

#include "common/GlobalContext.hpp"
#include "common/Tags.hpp"
#include "common/components/Layout.hpp"

namespace ui::tests
{
namespace
{

class FocusNavigationTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        m_system = std::make_unique<systems::FocusNavigationSystem>(m_runtime);
        m_system->registerHandlers();
        m_runtime.dispatcher().sink<events::FocusChangeRequest>().connect<&FocusNavigationTest::onFocusChange>(*this);
    }

    void TearDown() override
    {
        m_runtime.dispatcher().sink<events::FocusChangeRequest>().disconnect<&FocusNavigationTest::onFocusChange>(*this);
        m_system->unregisterHandlers();
        m_system.reset();
        m_scope.reset();
    }

    void onFocusChange(const events::FocusChangeRequest& event)
    {
        m_requests.push_back(event.target);
    }

    Registry& registry()
    {
        return m_runtime.registry();
    }

    void setFocused(entt::entity entity)
    {
        auto& reg = registry();
        auto* state = reg.ctx().find<globalcontext::StateContext>();
        if (state == nullptr)
        {
            state = &reg.ctx().emplace<globalcontext::StateContext>();
        }
        state->focusedEntity = entity;
    }

    void pressKey(int32_t key, uint16_t modifiers = 0)
    {
        m_runtime.dispatcher().trigger<events::RawKeyInput>(events::RawKeyInput{key, true, false, modifiers});
    }

    std::vector<entt::entity> takeRequests()
    {
        auto requests = std::move(m_requests);
        m_requests.clear();
        return requests;
    }

    /// 创建一个可聚焦控件（含 FocusableTag / VisibleTag / Position / Size）。
    entt::entity makeFocusable(float x, float y, float width, float height)
    {
        auto& reg = registry();
        const entt::entity entity = reg.create();
        reg.emplace<components::FocusableTag>(entity);
        reg.emplace<components::VisibleTag>(entity);
        reg.emplace<components::Position>(entity).value = Vec2{x, y};
        reg.emplace<components::Size>(entity).size = Vec2{width, height};
        return entity;
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
    std::unique_ptr<systems::FocusNavigationSystem> m_system;
    std::vector<entt::entity> m_requests;
};

TEST_F(FocusNavigationTest, TabFocusesFirstFocusableWhenNoFocus)
{
    const entt::entity first = makeFocusable(0.0F, 0.0F, 100.0F, 30.0F);
    makeFocusable(0.0F, 40.0F, 100.0F, 30.0F);

    pressKey(SDLK_TAB);

    const auto requests = takeRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests[0], first);
}

TEST_F(FocusNavigationTest, TabWrapsAroundFromLastToFirst)
{
    const entt::entity first = makeFocusable(0.0F, 0.0F, 100.0F, 30.0F);
    const entt::entity last = makeFocusable(0.0F, 40.0F, 100.0F, 30.0F);

    setFocused(last);
    pressKey(SDLK_TAB);

    const auto requests = takeRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests[0], first);
}

TEST_F(FocusNavigationTest, ShiftTabWrapsAroundFromFirstToLast)
{
    const entt::entity first = makeFocusable(0.0F, 0.0F, 100.0F, 30.0F);
    const entt::entity last = makeFocusable(0.0F, 40.0F, 100.0F, 30.0F);

    setFocused(first);
    pressKey(SDLK_TAB, SDL_KMOD_SHIFT);

    const auto requests = takeRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests[0], last);
}

TEST_F(FocusNavigationTest, DisabledAndInvisibleAreSkipped)
{
    const entt::entity first = makeFocusable(0.0F, 0.0F, 100.0F, 30.0F);
    const entt::entity disabled = makeFocusable(0.0F, 40.0F, 100.0F, 30.0F);
    const entt::entity third = makeFocusable(0.0F, 80.0F, 100.0F, 30.0F);
    const entt::entity invisible = makeFocusable(0.0F, 120.0F, 100.0F, 30.0F);

    registry().emplace<components::DisabledTag>(disabled);
    registry().remove<components::VisibleTag>(invisible);

    setFocused(first);
    pressKey(SDLK_TAB);

    const auto requests = takeRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests[0], third);  // 跳过 disabled，且 invisible 不在序列中
}

TEST_F(FocusNavigationTest, ArrowKeyNavigatesSpatially)
{
    const entt::entity top = makeFocusable(0.0F, 0.0F, 100.0F, 30.0F);
    const entt::entity bottom = makeFocusable(0.0F, 100.0F, 100.0F, 30.0F);
    const entt::entity left = makeFocusable(-150.0F, 100.0F, 100.0F, 30.0F);
    const entt::entity right = makeFocusable(150.0F, 100.0F, 100.0F, 30.0F);

    // 从 top 向下 → bottom
    setFocused(top);
    pressKey(SDLK_DOWN);
    {
        const auto requests = takeRequests();
        ASSERT_EQ(requests.size(), 1U);
        EXPECT_EQ(requests[0], bottom);
    }

    // 从 bottom 向左 → left
    setFocused(bottom);
    pressKey(SDLK_LEFT);
    {
        const auto requests = takeRequests();
        ASSERT_EQ(requests.size(), 1U);
        EXPECT_EQ(requests[0], left);
    }

    // 从 bottom 向右 → right
    setFocused(bottom);
    pressKey(SDLK_RIGHT);
    {
        const auto requests = takeRequests();
        ASSERT_EQ(requests.size(), 1U);
        EXPECT_EQ(requests[0], right);
    }

    // 从 bottom 向上 → top
    setFocused(bottom);
    pressKey(SDLK_UP);
    {
        const auto requests = takeRequests();
        ASSERT_EQ(requests.size(), 1U);
        EXPECT_EQ(requests[0], top);
    }
}

}  // namespace
}  // namespace ui::tests
