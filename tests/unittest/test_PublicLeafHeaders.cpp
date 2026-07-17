#include <ui/Callback.hpp>
#include <ui/Geometry.hpp>
#include <ui/TweenOptions.hpp>
#include <ui/api/Icon.hpp>
#include <ui/api/Layout.hpp>
#include <ui/api/Query.hpp>
#include <ui/api/Size.hpp>

#include <gtest/gtest.h>

#include "src/common/Animation.hpp" // IWYU pragma: keep -- compatibility include path
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

TEST(PublicGeometryTest, GeometryTypesArePortableValues)
{
    static_assert(std::is_standard_layout_v<GeometryRect>);
    static_assert(std::is_trivially_copyable_v<GeometryRect>);
    static_assert(std::is_standard_layout_v<VerticalScrollbarGeometry>);
    static_assert(std::is_trivially_copyable_v<VerticalScrollbarGeometry>);

    constexpr GeometryRect rect{1.0F, 2.0F, 3.0F, 4.0F};
    static_assert(rect.Contains(1.0F, 2.0F));
    static_assert(rect.Contains(4.0F, 6.0F));
    static_assert(!rect.Contains(4.1F, 6.0F));
    SUCCEED();
}

TEST(PublicTweenOptionsTest, DefaultsAndValueSemanticsRemainStable)
{
    static_assert(std::is_standard_layout_v<animation::TweenOptions>);
    static_assert(std::is_trivially_copyable_v<animation::TweenOptions>);

    constexpr animation::TweenOptions options;
    static_assert(options.duration == animation::DEFAULT_TWEEN_DURATION);
    static_assert(options.easing == policies::Easing::EASE_OUT_QUAD);
    static_assert(options.mode == policies::Play::ONCE);
    static_assert(options.autoCleanup);
    SUCCEED();
}

} // namespace
} // namespace ui::tests
