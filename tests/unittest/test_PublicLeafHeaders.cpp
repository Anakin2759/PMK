#include <ui/Callback.hpp>
#include <ui/Color.hpp>
#include <ui/Geometry.hpp>
#include <ui/TweenOptions.hpp>
#include <ui/api/Icon.hpp>
#include <ui/api/Layout.hpp>
#include <ui/api/Query.hpp>
#include <ui/api/Size.hpp>
#include <ui/api/Visibility.hpp>

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

TEST(PublicColorTest, ColorIsPortableAndKeepsScalarBehavior)
{
    constexpr float HALF = 0.5F;
    constexpr float GREEN_UPPER_BOUND = 0.51F;
    constexpr std::uint32_t PACKED_RGBA = 0xFF800040U;
    static_assert(std::is_standard_layout_v<Color>);
    static_assert(std::is_trivially_copyable_v<Color>);
    static_assert(sizeof(Color) == sizeof(float) * 4U);
    static_assert(alignof(Color) == alignof(float));

    constexpr auto color = Color::fromRGBA(255U, 128U, 0U, 64U);
    static_assert(color.red == 1.0F);
    static_assert(color.green > HALF && color.green < GREEN_UPPER_BOUND);
    static_assert(color.blue == 0.0F);
    static_assert(color.toSDLColor() == PACKED_RGBA);

    constexpr auto transparentRed = Color::Red().withAlpha(0.0F);
    static_assert(transparentRed.red == 1.0F);
    static_assert(transparentRed.alpha == 0.0F);

    constexpr auto midpoint = Lerp(Color::Black(), Color::White(), HALF);
    static_assert(midpoint.red == HALF);
    static_assert(midpoint.alpha == 1.0F);
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
