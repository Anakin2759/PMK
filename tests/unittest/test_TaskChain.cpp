#include <gtest/gtest.h>

#include <entt/entt.hpp>

#include <memory>
#include <optional>
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
        UiRuntime::current().dispatcher().update();
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

}  // namespace
}  // namespace ui::tests