#include <gtest/gtest.h>

#include <memory>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

namespace ui::tests
{
namespace
{

TEST(UiRuntimeTest, TryCurrentReturnsNullWithoutActiveScope)
{
    ASSERT_EQ(UiRuntime::tryCurrent(), nullptr);

    UiRuntime runtime;

    EXPECT_EQ(UiRuntime::tryCurrent(), nullptr);
}

TEST(UiRuntimeTest, ConstructingRuntimeDoesNotReplaceActiveRuntime)
{
    UiRuntime firstRuntime;
    UiRuntimeScope const firstScope(firstRuntime);
    ASSERT_EQ(UiRuntime::tryCurrent(), &firstRuntime);

    UiRuntime secondRuntime;

    EXPECT_EQ(UiRuntime::tryCurrent(), &firstRuntime);
}

TEST(UiRuntimeTest, NestedScopesRestorePreviousRuntime)
{
    UiRuntime firstRuntime;
    UiRuntime secondRuntime;

    {
        UiRuntimeScope const firstScope(firstRuntime);
        ASSERT_EQ(UiRuntime::tryCurrent(), &firstRuntime);

        {
            UiRuntimeScope const secondScope(secondRuntime);
            EXPECT_EQ(UiRuntime::tryCurrent(), &secondRuntime);
            EXPECT_EQ(&UiRuntime::current(), &secondRuntime);
        }

        EXPECT_EQ(UiRuntime::tryCurrent(), &firstRuntime);
        EXPECT_EQ(&UiRuntime::current(), &firstRuntime);
    }

    EXPECT_EQ(UiRuntime::tryCurrent(), nullptr);
}

TEST(UiRuntimeTest, DestroyingInactiveRuntimeDoesNotChangeCurrent)
{
    UiRuntime activeRuntime;
    UiRuntimeScope const activeScope(activeRuntime);

    {
        UiRuntime inactiveRuntime;
        ASSERT_EQ(UiRuntime::tryCurrent(), &activeRuntime);
    }

    EXPECT_EQ(UiRuntime::tryCurrent(), &activeRuntime);
}

TEST(UiRuntimeTest, DestroyingActiveRuntimeClearsStaleCurrent)
{
    auto runtime = std::make_unique<UiRuntime>();
    auto scope = std::make_unique<UiRuntimeScope>(*runtime);
    ASSERT_EQ(UiRuntime::tryCurrent(), runtime.get());

    runtime.reset();

    EXPECT_EQ(UiRuntime::tryCurrent(), nullptr);
    scope.reset();
    EXPECT_EQ(UiRuntime::tryCurrent(), nullptr);
}

} // namespace
} // namespace ui::tests