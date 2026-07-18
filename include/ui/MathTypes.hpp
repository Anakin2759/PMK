#pragma once

#include <type_traits>

namespace ui
{

/**
 * @brief 与数学后端无关的二维浮点向量值。
 *
 * 该公共叶子类型不依赖 Eigen、SDL、EnTT 或 Runtime。
 */
struct Vec2
{
    constexpr Vec2() noexcept = default;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- x/y 顺序是稳定公共契约。
    constexpr Vec2(float xValue, float yValue) noexcept : m_x(xValue), m_y(yValue) {}

    [[nodiscard]] constexpr float& x() noexcept { return m_x; }
    [[nodiscard]] constexpr float x() const noexcept { return m_x; }
    [[nodiscard]] constexpr float& y() noexcept { return m_y; }
    [[nodiscard]] constexpr float y() const noexcept { return m_y; }

    constexpr Vec2& operator+=(const Vec2& other) noexcept
    {
        m_x += other.m_x;
        m_y += other.m_y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& other) noexcept
    {
        m_x -= other.m_x;
        m_y -= other.m_y;
        return *this;
    }

    constexpr Vec2& operator*=(float scalar) noexcept
    {
        m_x *= scalar;
        m_y *= scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Vec2&) const noexcept = default;

private:
    float m_x = 0.0F;
    float m_y = 0.0F;
};

/**
 * @brief 与数学后端无关的四维浮点向量值。
 *
 * 主要用于圆角、边距等四分量公共值；内部 Eigen 运算应在边界显式转换。
 */
struct Vec4
{
    constexpr Vec4() noexcept = default;
    constexpr Vec4(float xValue, float yValue, float zValue, float wValue) noexcept
        : m_x(xValue), m_y(yValue), m_z(zValue), m_w(wValue)
    {
    }

    [[nodiscard]] constexpr float& x() noexcept { return m_x; }
    [[nodiscard]] constexpr float x() const noexcept { return m_x; }
    [[nodiscard]] constexpr float& y() noexcept { return m_y; }
    [[nodiscard]] constexpr float y() const noexcept { return m_y; }
    [[nodiscard]] constexpr float& z() noexcept { return m_z; }
    [[nodiscard]] constexpr float z() const noexcept { return m_z; }
    [[nodiscard]] constexpr float& w() noexcept { return m_w; }
    [[nodiscard]] constexpr float w() const noexcept { return m_w; }

    [[nodiscard]] constexpr bool operator==(const Vec4&) const noexcept = default;

private:
    float m_x = 0.0F;
    float m_y = 0.0F;
    float m_z = 0.0F;
    float m_w = 0.0F;
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 left, const Vec2& right) noexcept
{
    left += right;
    return left;
}

[[nodiscard]] constexpr Vec2 operator-(Vec2 left, const Vec2& right) noexcept
{
    left -= right;
    return left;
}

[[nodiscard]] constexpr Vec2 operator-(const Vec2& value) noexcept
{
    return {-value.x(), -value.y()};
}

[[nodiscard]] constexpr Vec2 operator*(Vec2 value, float scalar) noexcept
{
    value *= scalar;
    return value;
}

[[nodiscard]] constexpr Vec2 operator*(float scalar, Vec2 value) noexcept
{
    value *= scalar;
    return value;
}

[[nodiscard]] constexpr float LengthSquared(const Vec2& value) noexcept
{
    return (value.x() * value.x()) + (value.y() * value.y());
}

/**
 * @brief 与数学后端无关的轴对齐矩形。
 */
struct Rect
{
    Vec2 position;
    Vec2 size;

    constexpr Rect() noexcept = default;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- x/y/width/height 顺序是稳定公共契约。
    constexpr Rect(float xValue, float yValue, float widthValue, float heightValue) noexcept
        : position(xValue, yValue), size(widthValue, heightValue)
    {
    }
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- position/size 顺序是稳定公共契约。
    constexpr Rect(Vec2 positionValue, Vec2 sizeValue) noexcept : position(positionValue), size(sizeValue) {}

    [[nodiscard]] constexpr float x() const noexcept { return position.x(); }
    [[nodiscard]] constexpr float y() const noexcept { return position.y(); }
    [[nodiscard]] constexpr float width() const noexcept { return size.x(); }
    [[nodiscard]] constexpr float height() const noexcept { return size.y(); }

    [[nodiscard]] constexpr float left() const noexcept { return position.x(); }
    [[nodiscard]] constexpr float top() const noexcept { return position.y(); }
    [[nodiscard]] constexpr float right() const noexcept { return position.x() + size.x(); }
    [[nodiscard]] constexpr float bottom() const noexcept { return position.y() + size.y(); }

    [[nodiscard]] constexpr bool contains(Vec2 point) const noexcept
    {
        return point.x() >= left() && point.x() <= right() && point.y() >= top() && point.y() <= bottom();
    }
};

static_assert(std::is_standard_layout_v<Vec2>);
static_assert(std::is_trivially_copyable_v<Vec2>);
static_assert(sizeof(Vec2) == 2U * sizeof(float));
static_assert(alignof(Vec2) == alignof(float));
static_assert(std::is_standard_layout_v<Vec4>);
static_assert(std::is_trivially_copyable_v<Vec4>);
static_assert(sizeof(Vec4) == 4U * sizeof(float));
static_assert(alignof(Vec4) == alignof(float));
static_assert(std::is_standard_layout_v<Rect>);
static_assert(std::is_trivially_copyable_v<Rect>);
static_assert(sizeof(Rect) == 4U * sizeof(float));
static_assert(alignof(Rect) == alignof(float));

} // namespace ui
