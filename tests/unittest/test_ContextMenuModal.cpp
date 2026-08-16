/**
 * ************************************************************************
 *
 * @file test_ContextMenuModal.cpp
 * @brief P2-2 ContextMenu 与 ModalDialog 单元测试
 *
 * 覆盖：
 * - ContextMenu：创建/添加菜单项、Show（自动挂载窗口 + 入浮层栈）、
 *   菜单项点击执行回调并自动关闭、Close 出栈
 * - ModalDialog：创建（遮罩+内容）、Show（入栈 + 遮罩定位）、
 *   点击遮罩关闭、Close 出栈
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <vector>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/systems/OverlaySystem.hpp"

#include <ui.hpp>

#include "common/GlobalContext.hpp"
#include "common/Tags.hpp"
#include "common/components/Data.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Overlay.hpp"
#include "common/components/Visual.hpp"
#include "helper/Helper.hpp"

namespace ui::tests
{
namespace
{

class ContextMenuModalTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        m_system = std::make_unique<systems::OverlaySystem>(m_runtime);
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

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
    std::unique_ptr<systems::OverlaySystem> m_system;
};

TEST_F(ContextMenuModalTest, CreateContextMenuBuildsMenuStructure)
{
    auto menu = factory::CreateContextMenu("ctx");
    EXPECT_NE(menu, ui::null_entity);
    EXPECT_TRUE(registry().any_of<components::ContextMenuTag>(menu));
    EXPECT_TRUE(registry().any_of<components::ContextMenu>(menu));
    EXPECT_FALSE(registry().any_of<components::VisibleTag>(menu)) << "创建后默认不可见";
}

TEST_F(ContextMenuModalTest, AddItemAndShowOpensOverlay)
{
    auto menu = factory::CreateContextMenu("ctx");
    std::atomic<bool> clicked{false};
    factory::AddContextMenuItem(menu, "复制", [&clicked]() { clicked.store(true); });

    // 需要一个窗口根（FindWindowRoot 要求实体在窗口树中）
    auto window = factory::CreateBaseWidget("win");
    registry().emplace<components::WindowTag>(window);
    registry().get_or_emplace<components::Hierarchy>(window);
    registry().get_or_emplace<components::Size>(window);
    registry().get_or_emplace<components::Position>(window);

    factory::ShowContextMenu(menu, Vec2{10.0F, 20.0F}, window);

    auto* menuComp = registry().try_get<components::ContextMenu>(menu);
    ASSERT_NE(menuComp, nullptr);
    EXPECT_TRUE(menuComp->open) << "Show 后应标记打开";
    EXPECT_TRUE(registry().any_of<components::VisibleTag>(menu));

    // 已入浮层栈
    const auto& stack = overlayContext().stack;
    EXPECT_TRUE(std::ranges::find(stack, detail::ToInternal(menu)) != stack.end()) << "菜单应已入浮层栈";

    // 已挂载到窗口树
    const auto* menuHier = registry().try_get<components::Hierarchy>(menu);
    ASSERT_NE(menuHier, nullptr);
    EXPECT_NE(menuHier->parent, static_cast<entt::entity>(entt::null)) << "Show 应自动挂载菜单到窗口";
}

TEST_F(ContextMenuModalTest, MenuItemClickExecutesCallbackAndCloses)
{
    auto menu = factory::CreateContextMenu("ctx");
    std::atomic<int> callbackCount{0};
    auto item = factory::AddContextMenuItem(menu, "删除", [&callbackCount]() { callbackCount.fetch_add(1); });

    auto window = factory::CreateBaseWidget("win");
    registry().emplace<components::WindowTag>(window);
    registry().get_or_emplace<components::Hierarchy>(window);
    registry().get_or_emplace<components::Size>(window);
    registry().get_or_emplace<components::Position>(window);

    factory::ShowContextMenu(menu, Vec2{0.0F, 0.0F}, window);
    ASSERT_TRUE(registry().try_get<components::ContextMenu>(menu)->open);

    // 触发菜单项点击
    auto* clickable = registry().try_get<components::Clickable>(item);
    ASSERT_NE(clickable, nullptr);
    clickable->onClick();

    EXPECT_EQ(callbackCount.load(), 1) << "点击菜单项应执行回调";
    EXPECT_FALSE(registry().try_get<components::ContextMenu>(menu)->open) << "点击后应自动关闭菜单";
    EXPECT_FALSE(registry().any_of<components::VisibleTag>(menu));
}

TEST_F(ContextMenuModalTest, CloseContextMenuPopsOverlay)
{
    auto menu = factory::CreateContextMenu("ctx");
    auto window = factory::CreateBaseWidget("win");
    registry().emplace<components::WindowTag>(window);
    registry().get_or_emplace<components::Hierarchy>(window);
    registry().get_or_emplace<components::Size>(window);
    registry().get_or_emplace<components::Position>(window);

    factory::ShowContextMenu(menu, Vec2{5.0F, 5.0F}, window);
    EXPECT_FALSE(overlayContext().stack.empty());

    factory::CloseContextMenu(menu);
    EXPECT_FALSE(registry().try_get<components::ContextMenu>(menu)->open);
    const auto& stack = overlayContext().stack;
    EXPECT_TRUE(std::ranges::find(stack, detail::ToInternal(menu)) == stack.end()) << "关闭后应出栈";
}

TEST_F(ContextMenuModalTest, CreateModalDialogBuildsMaskAndContent)
{
    auto window = factory::CreateBaseWidget("win");
    registry().emplace<components::WindowTag>(window);
    registry().get_or_emplace<components::Hierarchy>(window);
    registry().get_or_emplace<components::Size>(window);
    registry().get_or_emplace<components::Position>(window);

    auto dialog = factory::CreateModalDialog(window, "dlg");
    EXPECT_NE(dialog, ui::null_entity);
    EXPECT_TRUE(registry().any_of<components::ModalDialogTag>(dialog));
    auto* dialogComp = registry().try_get<components::ModalDialog>(dialog);
    ASSERT_NE(dialogComp, nullptr);
    EXPECT_NE(dialogComp->popupEntity, static_cast<entt::entity>(entt::null)) << "应创建遮罩容器";
    EXPECT_TRUE(registry().valid(dialogComp->popupEntity));
    EXPECT_FALSE(registry().any_of<components::VisibleTag>(dialog)) << "创建后默认不可见";
}

TEST_F(ContextMenuModalTest, ShowModalDialogOpensOverlayAndClosePops)
{
    auto window = factory::CreateBaseWidget("win");
    registry().emplace<components::WindowTag>(window);
    registry().get_or_emplace<components::Hierarchy>(window);
    registry().get_or_emplace<components::Size>(window);
    registry().get_or_emplace<components::Position>(window);

    auto dialog = factory::CreateModalDialog(window, "dlg");
    factory::ShowModalDialog(dialog);

    auto* dialogComp = registry().try_get<components::ModalDialog>(dialog);
    ASSERT_NE(dialogComp, nullptr);
    EXPECT_TRUE(dialogComp->open);
    EXPECT_TRUE(registry().any_of<components::VisibleTag>(dialog));

    const auto& stack = overlayContext().stack;
    EXPECT_TRUE(std::ranges::find(stack, detail::ToInternal(dialog)) != stack.end()) << "对话框应已入浮层栈";

    factory::CloseModalDialog(dialog);
    EXPECT_FALSE(dialogComp->open);
    EXPECT_TRUE(std::ranges::find(overlayContext().stack, detail::ToInternal(dialog)) == stack.end())
        << "关闭后应出栈";
}

TEST_F(ContextMenuModalTest, MaskClickClosesModalDialog)
{
    auto window = factory::CreateBaseWidget("win");
    registry().emplace<components::WindowTag>(window);
    registry().get_or_emplace<components::Hierarchy>(window);
    registry().get_or_emplace<components::Size>(window);
    registry().get_or_emplace<components::Position>(window);

    auto dialog = factory::CreateModalDialog(window, "dlg");
    factory::ShowModalDialog(dialog);

    auto* dialogComp = registry().try_get<components::ModalDialog>(dialog);
    ASSERT_NE(dialogComp, nullptr);
    ASSERT_TRUE(dialogComp->open);

    // 点击遮罩应关闭对话框
    const entt::entity mask = dialogComp->popupEntity;
    auto* maskClickable = registry().try_get<components::Clickable>(mask);
    ASSERT_NE(maskClickable, nullptr);
    maskClickable->onClick();

    EXPECT_FALSE(dialogComp->open) << "点击遮罩应关闭对话框";
}

}  // namespace
}  // namespace ui::tests
