#include <gtest/gtest.h>

#include <entt/entt.hpp>

#include <memory>
#include <string>
#include <vector>

#include "src/common/Events.hpp"
#include "src/common/GlobalContext.hpp"
#include "src/core/TaskChain.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/utils/Dispatcher.hpp"
#include "src/utils/Registry.hpp"

#include <ui/api/Event.hpp>

namespace ui::tests
{
namespace
{

struct CountingTask
{
    using is_task_tag = void;

    int* callCount;
    int* lastSum;

    void operator()(int left, int right) const
    {
        ++(*callCount);
        *lastSum = left + right;
    }
};

struct SummingTask
{
    using is_task_tag = void;

    int* callCount;

    int operator()(int left, int right) const
    {
        ++(*callCount);
        return left + right;
    }
};

struct NewlyCreatedEvent
{
    using is_event_tag = void;
};

struct PoolCreationListener
{
    Dispatcher* dispatcher;
    int* parentCallCount;

    void OnParent([[maybe_unused]] events::QuitRequested& event) const
    {
        dispatcher->trigger<NewlyCreatedEvent>();
        ++(*parentCallCount);
    }
};

struct NewlyCreatedEventListener
{
    int* callCount;

    void OnEvent([[maybe_unused]] NewlyCreatedEvent& event) const
    {
        ++(*callCount);
    }
};

struct BufferedChainListener
{
    Dispatcher* dispatcher;
    std::vector<int>* order;

    void OnRaw(events::RawPointerMove& event) const
    {
        order->push_back(1);
        dispatcher->enqueue(events::HitPointerMove{.raw = event, .hitEntity = entt::null});
    }

    void OnHit([[maybe_unused]] events::HitPointerMove& event) const
    {
        order->push_back(2);
        dispatcher->enqueue(events::HoverEvent{.entity = entt::null});
    }

    void OnHover([[maybe_unused]] events::HoverEvent& event) const
    {
        order->push_back(3);
    }
};

class TaskChainTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        auto& frame = UiRuntime::current().ensureContext<globalcontext::FrameContext>();
        frame.frameNumber = 0;
        frame.intervalMs = 0;
        frame.layoutUpdateCount = 0;
        frame.renderUpdateCount = 0;
        frame.frameSlot = 0;
    }

    void TearDown() override
    {
        m_scope.reset();
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

TEST_F(TaskChainTest, CombinedBroadcastsArgumentsAndReturnsSecondResult)
{
    int firstCallCount = 0;
    int secondCallCount = 0;
    int lastFirstSum = 0;

    auto combined = tasks::PIPE_COMPOSER(CountingTask{.callCount = &firstCallCount, .lastSum = &lastFirstSum},
                                         SummingTask{&secondCallCount});

    const int result = combined(7, 5);

    EXPECT_EQ(firstCallCount, 1);
    EXPECT_EQ(secondCallCount, 1);
    EXPECT_EQ(lastFirstSum, 12);
    EXPECT_EQ(result, 12);
}

TEST_F(TaskChainTest, WrapArgsBindsArgumentsBeforeExecutingTask)
{
    int observedValue = 0;

    auto task = [&observedValue](int left, int right)
    {
        observedValue = left * right;
        return observedValue;
    };

    auto context = tasks::WrapArgs(6, 7);
    auto bound = context | task;

    EXPECT_EQ(bound(), 42);
    EXPECT_EQ(observedValue, 42);
}

TEST_F(TaskChainTest, TypedUpdateAllowsHandlerToCreateAnotherEventPool)
{
    auto& dispatcher = UiRuntime::current().dispatcher();
    int parentCallCount = 0;
    int createdEventCallCount = 0;
    PoolCreationListener listener{.dispatcher = &dispatcher, .parentCallCount = &parentCallCount};
    NewlyCreatedEventListener createdEventListener{.callCount = &createdEventCallCount};
    dispatcher.sink<events::QuitRequested>().connect<&PoolCreationListener::OnParent>(listener);
    dispatcher.sink<NewlyCreatedEvent>().connect<&NewlyCreatedEventListener::OnEvent>(createdEventListener);
    dispatcher.enqueue<events::QuitRequested>();

    dispatcher.update<events::QuitRequested>();

    EXPECT_EQ(parentCallCount, 1);
    EXPECT_EQ(createdEventCallCount, 1);
    dispatcher.sink<events::QuitRequested>().disconnect<&PoolCreationListener::OnParent>(listener);
    dispatcher.sink<NewlyCreatedEvent>().disconnect<&NewlyCreatedEventListener::OnEvent>(createdEventListener);
}

TEST_F(TaskChainTest, BufferedEventCatalogDefinesMembershipAndCount)
{
    static_assert(events::INTERNAL_BUFFERED_EVENT_COUNT == 13);
    static_assert(events::IS_INTERNAL_BUFFERED_EVENT<events::QuitRequested>);
    static_assert(events::IS_INTERNAL_BUFFERED_EVENT<events::RawPointerMove>);
    static_assert(events::IS_INTERNAL_BUFFERED_EVENT<events::HitPointerMove>);
    static_assert(events::IS_INTERNAL_BUFFERED_EVENT<events::HoverEvent>);
    static_assert(!events::IS_INTERNAL_BUFFERED_EVENT<events::UpdateLayout>);
    static_assert(!events::IS_INTERNAL_BUFFERED_EVENT<events::UpdateRendering>);
    static_assert(!events::IS_INTERNAL_BUFFERED_EVENT<events::UpdateEvent>);
    SUCCEED();
}

TEST_F(TaskChainTest, BufferedCatalogDispatchesRawHitAndHoverInOneFrame)
{
    auto& dispatcher = UiRuntime::current().dispatcher();
    std::vector<int> order;
    BufferedChainListener listener{.dispatcher = &dispatcher, .order = &order};
    dispatcher.sink<events::RawPointerMove>().connect<&BufferedChainListener::OnRaw>(listener);
    dispatcher.sink<events::HitPointerMove>().connect<&BufferedChainListener::OnHit>(listener);
    dispatcher.sink<events::HoverEvent>().connect<&BufferedChainListener::OnHover>(listener);

    dispatcher.enqueue(events::RawPointerMove{
        .position = {}, .delta = {}, .windowID = 1});
    tasks::detail::DispatchInternalQueued(dispatcher);

    EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
    dispatcher.sink<events::RawPointerMove>().disconnect<&BufferedChainListener::OnRaw>(listener);
    dispatcher.sink<events::HitPointerMove>().disconnect<&BufferedChainListener::OnHit>(listener);
    dispatcher.sink<events::HoverEvent>().disconnect<&BufferedChainListener::OnHover>(listener);
}

}  // namespace
}  // namespace ui::tests
