#include <ui/Callback.hpp>
#include <ui/Color.hpp>
#include <ui/Geometry.hpp>
#include <ui/MathTypes.hpp>
#include <ui/TweenOptions.hpp>
#include <ui/api/Animation.hpp>
#include <ui/api/Icon.hpp>
#include <ui/api/Image.hpp>
#include <ui/api/Table.hpp>
#include <ui/api/Layout.hpp>
#include <ui/api/Query.hpp>
#include <ui/api/Size.hpp>
#include <ui/api/Text.hpp>
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

TEST(PublicMathTypesTest, Vec2IsPortableAndKeepsBasicArithmetic)
{
    constexpr float ONE = 1.0F;
    constexpr float TWO = 2.0F;
    constexpr float THREE = 3.0F;
    constexpr float FOUR = 4.0F;
    constexpr float SIX = 6.0F;
    constexpr float EIGHT = 8.0F;
    constexpr float TWENTY_FIVE = 25.0F;
    static_assert(std::is_standard_layout_v<Vec2>);
    static_assert(std::is_trivially_copyable_v<Vec2>);
    static_assert(sizeof(Vec2) == sizeof(float) * 2U);
    static_assert(alignof(Vec2) == alignof(float));

    constexpr Vec2 zero;
    static_assert(zero == Vec2{0.0F, 0.0F});

    constexpr Vec2 value{THREE, FOUR};
    static_assert(value.x() == THREE);
    static_assert(value.y() == FOUR);
    static_assert(LengthSquared(value) == TWENTY_FIVE);
    static_assert(value + Vec2{ONE, TWO} == Vec2{FOUR, SIX});
    static_assert(value - Vec2{ONE, TWO} == Vec2{TWO, TWO});
    static_assert(-value == Vec2{-THREE, -FOUR});
    static_assert(value * TWO == Vec2{SIX, EIGHT});
    static_assert(TWO * value == Vec2{SIX, EIGHT});

    constexpr auto mutated = [=]
    {
        Vec2 result{ONE, TWO};
        result.x() = THREE;
        result.y() = FOUR;
        result += Vec2{ONE, ONE};
        result -= Vec2{TWO, ONE};
        result *= TWO;
        return result;
    }();
    static_assert(mutated == Vec2{FOUR, EIGHT});
    SUCCEED();
}

TEST(PublicMathTypesTest, RectKeepsFourFloatLayoutAndClosedBounds)
{
    constexpr float OUTSIDE_LEFT = 0.9F;
    constexpr float LEFT = 1.0F;
    constexpr float TOP = 2.0F;
    constexpr float WIDTH = 3.0F;
    constexpr float HEIGHT = 4.0F;
    constexpr float RIGHT = 4.0F;
    constexpr float OUTSIDE_RIGHT = 4.1F;
    constexpr float BOTTOM = 6.0F;
    static_assert(std::is_standard_layout_v<Rect>);
    static_assert(std::is_trivially_copyable_v<Rect>);
    static_assert(sizeof(Rect) == sizeof(float) * 4U);
    static_assert(alignof(Rect) == alignof(float));

    constexpr Rect rect{LEFT, TOP, WIDTH, HEIGHT};
    static_assert(rect.x() == LEFT);
    static_assert(rect.y() == TOP);
    static_assert(rect.width() == WIDTH);
    static_assert(rect.height() == HEIGHT);
    static_assert(rect.left() == LEFT);
    static_assert(rect.top() == TOP);
    static_assert(rect.right() == RIGHT);
    static_assert(rect.bottom() == BOTTOM);
    static_assert(rect.contains({LEFT, TOP}));
    static_assert(rect.contains({RIGHT, BOTTOM}));
    static_assert(!rect.contains({OUTSIDE_LEFT, TOP}));
    static_assert(!rect.contains({OUTSIDE_RIGHT, BOTTOM}));

    constexpr Rect vectorRect{Vec2{LEFT, TOP}, Vec2{WIDTH, HEIGHT}};
    static_assert(vectorRect.position == Vec2{LEFT, TOP});
    static_assert(vectorRect.size == Vec2{WIDTH, HEIGHT});
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
