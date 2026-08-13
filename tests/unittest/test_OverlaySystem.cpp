/**
 * ************************************************************************
 *
 * @file test_OverlaySystem.cpp
 * @brief OverlaySystem 单元测试
 *
 * 覆盖：浮层打开（统一 z-order 分配 + 入栈）、关闭（出栈 + 焦点恢复）、
 * 外部点击关闭顶层浮层。
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <SDL3/SDL_mouse.h>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/systems/OverlaySystem.hpp"

#include "common/GlobalContext.hpp"
#include "common/Tags.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Overlay.hpp"

namespace ui::tests
{
namespace
{

class OverlaySystemTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        m_system = std::make_unique<systems::OverlaySystem>(m_runtime);
        m_system->registerHandlers();
        m_runtime.dispatcher().sink<events::FocusChangeRequest>().connect<&OverlaySystemTest::onFocusChange>(*this);
    }

    void TearDown() override
    {
        m_runtime.dispatcher().sink<events::FocusChangeRequest>().disconnect<&OverlaySystemTest::onFocusChange>(*this);
        m_system->unregisterHandlers();
        m_system.reset();
        m_scope.reset();
    }

    void onFocusChange(const events::FocusChangeRequest& event)
    {
        m_focusRequests.push_back(event.target);
    }

    Registry& registry()
    {
        return m_runtime.registry();
    }

    Dispatcher& dispatcher()
    {
        return m_runtime.dispatcher();
    }

    globalcontext::OverlayContext& overlayContext()
    {
        auto& reg = registry();
        auto* ctx = reg.ctx().find<globalcontext::OverlayContext>();
        if (ctx == nullptr)
        {
            ctx = &reg.ctx().emplace<globalcontext::OverlayContext>();
        }
        return *ctx;
    }

    std::vector<entt::entity> takeFocusRequests()
    {
        auto requests = std::move(m_focusRequests);
        m_focusRequests.clear();
        return requests;
    }

    /// 创建一个带 FocusableTag 的实体（作为浮层 owner，用于焦点恢复）。
    entt::entity makeOwner()
    {
        const entt::entity entity = registry().create();
        registry().emplace<components::FocusableTag>(entity);
        return entity;
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
    std::unique_ptr<systems::OverlaySystem> m_system;
    std::vector<entt::entity> m_focusRequests;
};

TEST_F(OverlaySystemTest, OpenRequestAssignsZOrderAndPushesStack)
{
    const entt::entity owner = makeOwner();
    const entt::entity popup = registry().create();

    dispatcher().trigger<events::OverlayOpenRequest>(events::OverlayOpenRequest{popup, owner});

    const auto* zOrder = registry().try_get<components::ZOrderIndex>(popup);
    ASSERT_NE(zOrder, nullptr);
    EXPECT_EQ(zOrder->value, globalcontext::OverlayContext::kDefaultZBase);

    const auto* layer = registry().try_get<components::OverlayLayer>(popup);
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->owner, owner);
    EXPECT_EQ(layer->zLevel, 0);

    ASSERT_EQ(overlayContext().stack.size(), 1U);
    EXPECT_EQ(overlayContext().stack.front(), popup);
}

TEST_F(OverlaySystemTest, CloseRequestPopsStackAndRestoresFocus)
{
    const entt::entity owner = makeOwner();
    const entt::entity popup = registry().create();

    dispatcher().trigger<events::OverlayOpenRequest>(events::OverlayOpenRequest{popup, owner});
    ASSERT_EQ(overlayContext().stack.size(), 1U);

    dispatcher().trigger<events::OverlayCloseRequest>(events::OverlayCloseRequest{popup});

    EXPECT_TRUE(overlayContext().stack.empty());

    const auto requests = takeFocusRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests[0], owner);  // 焦点恢复到 owner
}

TEST_F(OverlaySystemTest, OutsideClickClosesTopOverlayAndRestoresFocus)
{
    const entt::entity owner = makeOwner();
    const entt::entity popup = registry().create();

    dispatcher().trigger<events::OverlayOpenRequest>(events::OverlayOpenRequest{popup, owner});
    ASSERT_EQ(overlayContext().stack.size(), 1U);

    // 命中点在浮层外（hitEntity = null），应关闭顶层浮层
    events::HitPointerButton click;
    click.raw.pressed = true;
    click.raw.button = SDL_BUTTON_LEFT;
    click.raw.position = Vec2{0.0F, 0.0F};
    click.raw.windowID = 0;
    click.hitEntity = entt::null;

    dispatcher().trigger<events::HitPointerButton>(std::move(click));

    EXPECT_TRUE(overlayContext().stack.empty());

    const auto requests = takeFocusRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests[0], owner);
}

}  // namespace
}  // namespace ui::tests
