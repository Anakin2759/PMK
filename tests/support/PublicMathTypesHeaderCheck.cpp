#include <ui/MathTypes.hpp>

#include <type_traits>

static_assert(std::is_standard_layout_v<ui::Vec2>);
static_assert(std::is_trivially_copyable_v<ui::Vec2>);
static_assert(sizeof(ui::Vec2) == 2U * sizeof(float));
static_assert(alignof(ui::Vec2) == alignof(float));
static_assert(std::is_standard_layout_v<ui::Rect>);
static_assert(std::is_trivially_copyable_v<ui::Rect>);
static_assert(sizeof(ui::Rect) == 4U * sizeof(float));
static_assert(alignof(ui::Rect) == alignof(float));

// 这些字面量是公共数学类型 ABI 与编译期边界测试数据，不是业务参数。
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
constexpr float VECTOR_X = 3.0F;
constexpr float VECTOR_Y = 4.0F;
constexpr float VECTOR_LENGTH_SQUARED = 25.0F;
constexpr float RECT_LEFT = 1.0F;
constexpr float RECT_TOP = 2.0F;
constexpr float RECT_WIDTH = 3.0F;
constexpr float RECT_RIGHT = 4.0F;
constexpr float RECT_BOTTOM = 6.0F;
constexpr float OUTSIDE_RECT_RIGHT = 4.1F;
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

constexpr ui::Vec2 VECTOR{VECTOR_X, VECTOR_Y};
static_assert(ui::LengthSquared(VECTOR) == VECTOR_LENGTH_SQUARED);

constexpr ui::Rect RECT{RECT_LEFT, RECT_TOP, RECT_WIDTH, RECT_RIGHT};
static_assert(RECT.contains({RECT_LEFT, RECT_TOP}));
static_assert(RECT.contains({RECT_RIGHT, RECT_BOTTOM}));
static_assert(!RECT.contains({OUTSIDE_RECT_RIGHT, RECT_BOTTOM}));
