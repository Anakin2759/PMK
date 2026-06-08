#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/core/UiRuntime.hpp"

#include <ui.hpp>

namespace ui::tests
{
namespace
{

class PublicEventApiTest : public ::testing::Test
{
protected:
    void SetUp() override { m_scope = std::make_unique<UiRuntimeScope>(m_runtime); }
    void TearDown() override { m_scope.reset(); }

private:
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

    event::Trigger(eventId,
                   event::EventPayload{.source = 7U,
                                       .target = 9U,
                                       .name = {},
                                       .text = "ok",
                                       .intValue = 42,
                                       .floatValue = 0.0});

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
        UiRuntimeScope const firstScope(firstRuntime);
        firstId = event::RegisterEvent("public.event.isolated");
        ASSERT_NE(firstId, event::INVALID_EVENT_ID);
        EXPECT_TRUE(event::IsEventRegistered(firstId));
    }

    {
        UiRuntimeScope const secondScope(secondRuntime);
        EXPECT_FALSE(event::IsEventRegistered(firstId));
        EXPECT_FALSE(event::IsEventRegistered("public.event.isolated"));
    }
}

} // namespace
} // namespace ui::tests
