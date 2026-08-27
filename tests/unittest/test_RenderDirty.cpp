#include <gtest/gtest.h>

#include "src/common/Tags.hpp"
#include "src/common/components/Layout.hpp"
#include "src/systems/render/RenderDirty.hpp"
#include "src/utils/Registry.hpp"

namespace ui::tests
{
namespace
{

TEST(RenderDirtyTest, ClearsOnlySuccessfulWindowSubtree)
{
    Registry registry;
    const auto successfulWindow = registry.create();
    const auto successfulChild = registry.create();
    const auto failedWindow = registry.create();
    const auto failedChild = registry.create();
    const auto detached = registry.create();

    registry.emplace<components::Hierarchy>(successfulWindow, entt::null,
                                             std::vector<entt::entity>{successfulChild});
    registry.emplace<components::Hierarchy>(successfulChild, successfulWindow, std::vector<entt::entity>{});
    registry.emplace<components::Hierarchy>(failedWindow, entt::null, std::vector<entt::entity>{failedChild});
    registry.emplace<components::Hierarchy>(failedChild, failedWindow, std::vector<entt::entity>{});
    registry.emplace<components::RenderDirtyTag>(successfulWindow);
    registry.emplace<components::RenderDirtyTag>(successfulChild);
    registry.emplace<components::RenderDirtyTag>(failedWindow);
    registry.emplace<components::RenderDirtyTag>(failedChild);
    registry.emplace<components::RenderDirtyTag>(detached);

    systems::render_detail::ClearRenderDirtySubtree(registry, successfulWindow);

    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(successfulWindow));
    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(successfulChild));
    EXPECT_TRUE(registry.any_of<components::RenderDirtyTag>(failedWindow));
    EXPECT_TRUE(registry.any_of<components::RenderDirtyTag>(failedChild));
    EXPECT_TRUE(registry.any_of<components::RenderDirtyTag>(detached));
}

TEST(RenderDirtyTest, IgnoresInvalidChildrenAndHierarchyCycles)
{
    Registry registry;
    const auto window = registry.create();
    const auto child = registry.create();
    const auto invalidChild = registry.create();
    registry.destroy(invalidChild);

    registry.emplace<components::Hierarchy>(window, entt::null, std::vector<entt::entity>{child, invalidChild});
    registry.emplace<components::Hierarchy>(child, window, std::vector<entt::entity>{window});
    registry.emplace<components::RenderDirtyTag>(window);
    registry.emplace<components::RenderDirtyTag>(child);

    systems::render_detail::ClearRenderDirtySubtree(registry, window);

    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(window));
    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(child));
}

TEST(RenderDirtyTest, FailedSubmissionKeepsDirtyAndLaterSuccessClearsWithoutRemarking)
{
    Registry registry;
    const auto window = registry.create();
    const auto child = registry.create();
    registry.emplace<components::Hierarchy>(window, entt::null, std::vector<entt::entity>{child});
    registry.emplace<components::Hierarchy>(child, window, std::vector<entt::entity>{});
    registry.emplace<components::RenderDirtyTag>(window);
    registry.emplace<components::RenderDirtyTag>(child);

    const auto failed = ui::Err(ui::UiErrc::SWAPCHAIN_UNAVAILABLE, "injected first-frame failure");
    EXPECT_FALSE(systems::render_detail::CommitRenderDirtyOnSuccess(registry, window, failed));
    EXPECT_TRUE(registry.any_of<components::RenderDirtyTag>(window));
    EXPECT_TRUE(registry.any_of<components::RenderDirtyTag>(child));

    EXPECT_TRUE(systems::render_detail::CommitRenderDirtyOnSuccess(registry, window, ui::Ok()));
    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(window));
    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(child));
}

TEST(RenderDirtyTest, MixedWindowResultsClearOnlySuccessAndFailedWindowCanRecover)
{
    Registry registry;
    const auto successfulWindow = registry.create();
    const auto failedWindow = registry.create();
    registry.emplace<components::RenderDirtyTag>(successfulWindow);
    registry.emplace<components::RenderDirtyTag>(failedWindow);

    EXPECT_TRUE(systems::render_detail::CommitRenderDirtyOnSuccess(registry, successfulWindow, ui::Ok()));
    EXPECT_FALSE(systems::render_detail::CommitRenderDirtyOnSuccess(
        registry, failedWindow, ui::Err(ui::UiErrc::SWAPCHAIN_UNAVAILABLE)));

    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(successfulWindow));
    EXPECT_TRUE(registry.any_of<components::RenderDirtyTag>(failedWindow));

    EXPECT_TRUE(systems::render_detail::CommitRenderDirtyOnSuccess(registry, failedWindow, ui::Ok()));
    EXPECT_FALSE(registry.any_of<components::RenderDirtyTag>(failedWindow));
}

}  // namespace
}  // namespace ui::tests