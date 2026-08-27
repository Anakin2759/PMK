#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

#include <ui.hpp>

namespace ui::tests
{
namespace
{

class PublicEventApiTest : public ::testing::Test
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

    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

TEST_F(PublicEventApiTest, RegisterEventReturnsStableIdForName)
{
    const auto first = event::RegisterEvent("public.event.stable");
    const auto second = event::RegisterEvent("public.event.stable");

    ASSERT_NE(first, event::INVALID_EVENT_ID);
    EXPECT_EQ(first, second);
    EXPECT_TRUE(event::IsEventRegistered(first));
    EXPECT_TRUE(event::IsEventRegistered("public.event.stable"));
}

TEST_F(PublicEventApiTest, TriggerInvokesRegisteredCallbackWithPayload)
{
    const auto eventId = event::RegisterEvent("public.event.trigger");
    bool called = false;
    event::EventPayload observed{};

    auto connection = event::On(eventId,
                                [&called, &observed](const event::EventPayload& payload)
                                {
                                    called = true;
                                    observed = payload;
                                });

    event::Trigger(
        eventId,
        event::EventPayload{.source = 7U, .target = 9U, .name = {}, .text = "ok", .intValue = 42, .floatValue = 0.0});

    EXPECT_TRUE(called);
    EXPECT_EQ(observed.source, 7U);
    EXPECT_EQ(observed.target, 9U);
    EXPECT_EQ(observed.text, "ok");
    EXPECT_EQ(observed.intValue, 42);
    EXPECT_TRUE(connection.Connected());
}

TEST_F(PublicEventApiTest, DisconnectStopsFurtherCallbacks)
{
    const auto eventId = event::RegisterEvent("public.event.disconnect");
    int callCount = 0;

    auto connection = event::On(eventId, [&callCount](const event::EventPayload&) { ++callCount; });
    ASSERT_TRUE(connection.Connected());

    connection.Disconnect();
    EXPECT_FALSE(connection.Connected());

    event::Trigger(eventId);
    EXPECT_EQ(callCount, 0);
}

TEST_F(PublicEventApiTest, EnqueueDispatchesOnlyWhenRequested)
{
    int callCount = 0;

    auto connection = event::On("public.event.queued", [&callCount](const event::EventPayload&) { ++callCount; });
    event::Enqueue("public.event.queued");

    EXPECT_EQ(callCount, 0);

    event::DispatchQueued();
    EXPECT_EQ(callCount, 1);
}

TEST(PublicEventApiIsolationTest, RuntimeScopesHaveIndependentEventTables)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;

    event::EventId firstId = event::INVALID_EVENT_ID;

    {
        UiRuntimeScope const scope(firstRuntime);
        firstId = event::RegisterEvent("public.event.isolated");
        ASSERT_NE(firstId, event::INVALID_EVENT_ID);
        EXPECT_TRUE(event::IsEventRegistered(firstId));
    }

    UiRuntimeScope const scope(secondRuntime);
    EXPECT_FALSE(event::IsEventRegistered(firstId));
    EXPECT_FALSE(event::IsEventRegistered("public.event.isolated"));
}

TEST(PublicEventApiIsolationTest, ConnectionCanDisconnectOutsideRuntimeScope)
{
    UiRuntime runtime;
    event::EventConnection connection;
    {
        UiRuntimeScope const scope(runtime);
        connection = event::On("public.event.outside-scope", [](const event::EventPayload&) {});
        ASSERT_TRUE(connection.Connected());
    }

    connection.Disconnect();
    EXPECT_FALSE(connection.Connected());
}

TEST(PublicEventApiIsolationTest, ConnectionBecomesDisconnectedAfterRuntimeDestruction)
{
    event::EventConnection connection;
    {
        auto runtime = std::make_unique<UiRuntime>();
        UiRuntimeScope const scope(*runtime);
        connection = event::On("public.event.runtime-destroyed", [](const event::EventPayload&) {});
        ASSERT_TRUE(connection.Connected());
        runtime.reset();
    }

    EXPECT_FALSE(connection.Connected());
    connection.Disconnect();
}

TEST(PublicEventApiIsolationTest, EqualRuntimeLocalTokensDoNotCrossDisconnect)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;
    event::EventConnection first;
    event::EventConnection second;
    int secondCount = 0;

    {
        UiRuntimeScope const scope(firstRuntime);
        first = event::On("public.event.token-isolation", [](const event::EventPayload&) {});
    }
    {
        UiRuntimeScope const scope(secondRuntime);
        second = event::On("public.event.token-isolation", [&](const event::EventPayload&) { ++secondCount; });
    }

    first.Disconnect();
    EXPECT_FALSE(first.Connected());
    ASSERT_TRUE(second.Connected());

    UiRuntimeScope const scope(secondRuntime);
    event::Trigger("public.event.token-isolation");
    EXPECT_EQ(secondCount, 1);
}

TEST_F(PublicEventApiTest, MoveTransfersConnectionOwnershipAndDisconnectIsIdempotent)
{
    int callCount = 0;
    auto original = event::On("public.event.move-ownership",
                              [&](const event::EventPayload&) { ++callCount; });
    event::EventConnection moved{std::move(original)};

    EXPECT_FALSE(original.Connected());
    ASSERT_TRUE(moved.Connected());
    moved.Disconnect();
    moved.Disconnect();

    event::Trigger("public.event.move-ownership");
    EXPECT_EQ(callCount, 0);
}

TEST_F(PublicEventApiTest, CallbackConnectedDuringDispatchStartsNextDispatch)
{
    int firstCount = 0;
    int secondCount = 0;
    event::EventConnection second;
    auto first = event::On("public.event.snapshot",
                           [&](const event::EventPayload&)
                           {
                               ++firstCount;
                               if (!second.Connected())
                               {
                                   second = event::On("public.event.snapshot",
                                                      [&](const event::EventPayload&) { ++secondCount; });
                               }
                           });

    event::Trigger("public.event.snapshot");
    EXPECT_EQ(firstCount, 1);
    EXPECT_EQ(secondCount, 0);

    event::Trigger("public.event.snapshot");
    EXPECT_EQ(firstCount, 2);
    EXPECT_EQ(secondCount, 1);
}

TEST_F(PublicEventApiTest, DisconnectDuringDispatchSkipsPendingCallback)
{
    int secondCount = 0;
    event::EventConnection second;
    auto first = event::On("public.event.disconnect-during-dispatch",
                           [&](const event::EventPayload&) { second.Disconnect(); });
    second = event::On("public.event.disconnect-during-dispatch",
                       [&](const event::EventPayload&) { ++secondCount; });

    event::Trigger("public.event.disconnect-during-dispatch");
    EXPECT_EQ(secondCount, 0);
    EXPECT_FALSE(second.Connected());
}

TEST_F(PublicEventApiTest, CallbackCanDisconnectItself)
{
    int callCount = 0;
    event::EventConnection connection;
    connection = event::On("public.event.self-disconnect",
                           [&](const event::EventPayload&)
                           {
                               ++callCount;
                               connection.Disconnect();
                           });

    event::Trigger("public.event.self-disconnect");
    event::Trigger("public.event.self-disconnect");

    EXPECT_EQ(callCount, 1);
    EXPECT_FALSE(connection.Connected());
}

TEST_F(PublicEventApiTest, NestedTriggerUsesIndependentSnapshot)
{
    int callCount = 0;
    bool nested = false;
    auto connection = event::On("public.event.nested",
                                [&](const event::EventPayload&)
                                {
                                    ++callCount;
                                    if (!nested)
                                    {
                                        nested = true;
                                        event::Trigger("public.event.nested");
                                    }
                                });

    event::Trigger("public.event.nested");
    EXPECT_EQ(callCount, 2);
    EXPECT_TRUE(connection.Connected());
}

}  // namespace
}  // namespace ui::tests
