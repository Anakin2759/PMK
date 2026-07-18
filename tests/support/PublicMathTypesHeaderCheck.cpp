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

constexpr ui::Vec2 VECTOR{3.0F, 4.0F};
static_assert(ui::LengthSquared(VECTOR) == 25.0F);

constexpr ui::Rect RECT{1.0F, 2.0F, 3.0F, 4.0F};
static_assert(RECT.contains({1.0F, 2.0F}));
static_assert(RECT.contains({4.0F, 6.0F}));
static_assert(!RECT.contains({4.1F, 6.0F}));
