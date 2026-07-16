#include <ui/Callback.hpp>
#include <ui/api/Icon.hpp>
#include <ui/api/Layout.hpp>
#include <ui/api/Query.hpp>
#include <ui/api/Size.hpp>

#include <gtest/gtest.h>

#include "src/common/components/Interaction.hpp"

#include <type_traits>

namespace ui::tests
{
namespace
{

TEST(PublicLeafHeadersTest, HeadersCompileFromStableIncludePaths)
{
    SUCCEED();
}

TEST(PublicCallbackTest, InternalCompatibilityAliasKeepsMoveOnlyCallbackType)
{
    static_assert(std::is_same_v<Callback<int>, components::on_event<int>>);
    static_assert(std::is_move_constructible_v<Callback<>>);
    static_assert(!std::is_copy_constructible_v<Callback<>>);
    SUCCEED();
}

} // namespace
} // namespace ui::tests
