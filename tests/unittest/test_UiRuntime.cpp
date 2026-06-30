#include <gtest/gtest.h>

#include <entt/entt.hpp>

#include "common/components/Window.hpp"
#include "src/api/Factory.hpp"
#include "src/common/Events.hpp"
#include "src/common/GlobalContext.hpp"
#include "src/core/RuntimeFacade.hpp"
#include "src/core/UiRuntime.hpp"

namespace ui::tests
{
namespace
{

struct UpdateFlagHandler
{
    bool* triggered;

    void onUpdate(const events::UpdateEvent& event) const
    {
        (void)event;
        *triggered = true;
    }
};

TEST(UiRuntimeTest, NestedRuntimeScopesSwitchRegistryAndDispatcherIndependently)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;
    bool firstTriggered = false;
    bool secondTriggered = false;

    {
        UiRuntimeScope const firstScope(firstRuntime);
        RuntimeFacade::current().ensureContext<globalcontext::FrameContext>().intervalMs = 11;

        UpdateFlagHandler firstHandler{&firstTriggered};
        auto firstConnection = entt::scoped_connection{
            RuntimeFacade::current().sink<events::UpdateEvent>().template connect<&UpdateFlagHandler::onUpdate>(
                firstHandler)};

        {
            UiRuntimeScope const secondScope(secondRuntime);

            EXPECT_EQ(RuntimeFacade::current().tryContext<globalcontext::FrameContext>(), nullptr);

            UpdateFlagHandler secondHandler{&secondTriggered};
            auto secondConnection = entt::scoped_connection{
                RuntimeFacade::current().sink<events::UpdateEvent>().template connect<&UpdateFlagHandler::onUpdate>(
                    secondHandler)};

            RuntimeFacade::current().enqueue<events::UpdateEvent>({});
            RuntimeFacade::current().update();

            EXPECT_FALSE(firstTriggered);
            EXPECT_TRUE(secondTriggered);

            RuntimeFacade::current().ensureContext<globalcontext::FrameContext>().intervalMs = 22;
            EXPECT_EQ(RuntimeFacade::current().context<globalcontext::FrameContext>().intervalMs, 22U);
        }

        ASSERT_NE(RuntimeFacade::current().tryContext<globalcontext::FrameContext>(), nullptr);
        EXPECT_EQ(RuntimeFacade::current().context<globalcontext::FrameContext>().intervalMs, 11U);

        RuntimeFacade::current().enqueue<events::UpdateEvent>({});
        RuntimeFacade::current().update();

        EXPECT_TRUE(firstTriggered);
    }
}

TEST(UiRuntimeTest, RuntimeFacadeFollowsActiveRuntimeScope)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;

    {
        UiRuntimeScope const firstScope(firstRuntime);
        RuntimeFacade::current().ensureContext<globalcontext::FrameContext>().intervalMs = 33;

        EXPECT_EQ(RuntimeFacade::current().frame().intervalMs, 33U);
        EXPECT_NE(RuntimeFacade::current().tryFrame(), nullptr);

        {
            UiRuntimeScope const secondScope(secondRuntime);

            EXPECT_EQ(RuntimeFacade::current().tryFrame(), nullptr);

            RuntimeFacade::current().ensureContext<globalcontext::FrameContext>().intervalMs = 44;
            EXPECT_EQ(RuntimeFacade::current().frame().intervalMs, 44U);
        }

        EXPECT_EQ(RuntimeFacade::current().frame().intervalMs, 33U);
    }
}

TEST(UiRuntimeTest, RuntimeTokenIdentifiesRuntimeOwnership)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;

    const RuntimeToken emptyToken{};
    const RuntimeToken firstToken = firstRuntime.token();
    const RuntimeToken secondToken = secondRuntime.token();

    EXPECT_FALSE(emptyToken.valid());
    EXPECT_TRUE(firstToken.valid());
    EXPECT_TRUE(secondToken.valid());
    EXPECT_NE(firstToken, secondToken);
    EXPECT_TRUE(SameRuntime(firstToken, firstToken));
    EXPECT_FALSE(SameRuntime(firstToken, secondToken));
    EXPECT_FALSE(SameRuntime(emptyToken, firstToken));
}

TEST(UiRuntimeTest, EntityAndWindowHandlesCarryRuntimeOwnership)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;

    const auto firstEntity = MakeEntityHandle(firstRuntime.token(), 7U);
    const auto firstEntitySibling = MakeEntityHandle(firstRuntime.token(), 8U);
    const auto secondEntity = MakeEntityHandle(secondRuntime.token(), 7U);
    const EntityHandle emptyEntity{};

    EXPECT_TRUE(firstEntity.valid());
    EXPECT_FALSE(emptyEntity.valid());
    EXPECT_EQ(firstEntity.id(), 7U);
    EXPECT_TRUE(SameRuntime(firstEntity, firstEntitySibling));
    EXPECT_FALSE(SameRuntime(firstEntity, secondEntity));

    const auto firstWindow = MakeWindowHandle(firstRuntime.token(), 11U, 101U);
    const auto secondWindow = MakeWindowHandle(secondRuntime.token(), 11U, 101U);
    const WindowHandle emptyWindow{};

    EXPECT_TRUE(firstWindow.valid());
    EXPECT_FALSE(emptyWindow.valid());
    EXPECT_EQ(firstWindow.id(), 11U);
    EXPECT_EQ(firstWindow.platformWindowId(), 101U);
    EXPECT_FALSE(SameRuntime(firstWindow, secondWindow));
}

TEST(UiRuntimeTest, SameRuntimeWindowHandlesKeepIndependentEntityAndPlatformIds)
{
    UiRuntime runtime;

    const auto firstWindow = MakeWindowHandle(runtime.token(), 11U, 101U);
    const auto secondWindow = MakeWindowHandle(runtime.token(), 12U, 202U);

    EXPECT_TRUE(firstWindow.valid());
    EXPECT_TRUE(secondWindow.valid());
    EXPECT_TRUE(SameRuntime(firstWindow, secondWindow));
    EXPECT_NE(firstWindow.id(), secondWindow.id());
    EXPECT_NE(firstWindow.platformWindowId(), secondWindow.platformWindowId());
}

TEST(UiRuntimeTest, ExplicitRuntimeFactoryCreatesOwnedButtonHandle)
{
    UiRuntime runtime;

    auto button = factory::CreateButton(runtime, "Owned", "ownedButton");

    ASSERT_TRUE(button.has_value());
    EXPECT_TRUE(button->valid());
    EXPECT_EQ(button->runtime(), runtime.token());
    EXPECT_TRUE(runtime.registry().valid(button->id()));
}

TEST(UiRuntimeTest, WindowLookupCacheIsolatedPerRuntime)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;
    entt::entity firstWindow = entt::null;

    {
        UiRuntimeScope const firstScope(firstRuntime);
        auto& registry = RuntimeFacade::current().registry();
        firstWindow = registry.create();
        registry.emplace<components::Window>(firstWindow).windowID = 101;
        RuntimeFacade::current().windowLookup().remember(firstWindow);

        EXPECT_EQ(RuntimeFacade::current().windowLookup().findById(101), firstWindow);

        {
            UiRuntimeScope const secondScope(secondRuntime);

            auto& secondRegistry = RuntimeFacade::current().registry();
            EXPECT_FALSE(secondRegistry.valid(RuntimeFacade::current().windowLookup().findById(101)));

            const auto secondWindow = secondRegistry.create();
            secondRegistry.emplace<components::Window>(secondWindow).windowID = 101;
            RuntimeFacade::current().windowLookup().remember(secondWindow);

            EXPECT_EQ(RuntimeFacade::current().windowLookup().findById(101), secondWindow);
        }

        EXPECT_EQ(RuntimeFacade::current().windowLookup().findById(101), firstWindow);
    }
}

TEST(UiRuntimeTest, WindowLookupCacheRecoversFromDestroyedEntity)
{
    UiRuntime runtime;
    {
        UiRuntimeScope const scope(runtime);
        auto& registry = RuntimeFacade::current().registry();

        const auto firstWindow = registry.create();
        registry.emplace<components::Window>(firstWindow).windowID = 202;
        RuntimeFacade::current().windowLookup().remember(firstWindow);

        EXPECT_EQ(RuntimeFacade::current().windowLookup().findById(202), firstWindow);

        registry.destroy(firstWindow);

        const auto secondWindow = registry.create();
        registry.emplace<components::Window>(secondWindow).windowID = 202;

        EXPECT_EQ(RuntimeFacade::current().windowLookup().findById(202), secondWindow);
    }
}

} // namespace
} // namespace ui::tests