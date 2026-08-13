#include <gtest/gtest.h>

#include "src/core/ApplicationLifecycle.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace ui::tests
{
namespace
{

TEST(ApplicationLifecycleTest, UnarmedLifecycleDoesNotQuit)
{
    int quitCount = 0;
    {
        detail::ApplicationLifecycle lifecycle;
        static_cast<void>(quitCount);
    }

    EXPECT_EQ(quitCount, 0);
}

TEST(ApplicationLifecycleTest, ConstructionFailureAfterArmQuitsExactlyOnce)
{
    int quitCount = 0;
    bool failureObserved = false;
    try
    {
        detail::ApplicationLifecycle lifecycle;
        lifecycle.ArmSdl([&quitCount] { ++quitCount; });
        throw std::runtime_error("later construction failed");
    }
    catch (const std::runtime_error&)
    {
        failureObserved = true;
    }

    EXPECT_TRUE(failureObserved);
    EXPECT_EQ(quitCount, 1);
}

TEST(ApplicationLifecycleTest, ShutdownDestroysSystemsBeforeQuitAndIsIdempotent)
{
    std::vector<std::string> operations;
    detail::ApplicationLifecycle lifecycle;
    lifecycle.ArmSdl([&operations] { operations.emplace_back("quit"); });

    lifecycle.Shutdown([&operations] { operations.emplace_back("systems"); });
    lifecycle.Shutdown([&operations] { operations.emplace_back("unexpected"); });

    EXPECT_EQ(operations, (std::vector<std::string>{"systems", "quit"}));
}

TEST(ApplicationLifecycleTest, ShutdownStillQuitsWhenSystemDestructionThrows)
{
    int quitCount = 0;
    detail::ApplicationLifecycle lifecycle;
    lifecycle.ArmSdl([&quitCount] { ++quitCount; });

    EXPECT_NO_THROW(lifecycle.Shutdown([] { throw std::runtime_error("cleanup failed"); }));
    EXPECT_EQ(quitCount, 1);
}

}  // namespace
}  // namespace ui::tests