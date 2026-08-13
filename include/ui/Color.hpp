#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>

namespace ui
{

/**
 * @brief 与渲染后端无关的 RGBA 颜色值，通道范围通常为 0.0F～1.0F。
 *
 * 该公共叶子类型不依赖 Eigen、SDL、EnTT 或 Runtime。
 */
struct Color
{
    static constexpr std::uint8_t MAX_CHANNEL_BYTE = 255U;
    static constexpr float MAX_CHANNEL_FLOAT = 255.0F;
    static constexpr std::uint32_t CHANNEL_MASK = 0xFFU;
    static constexpr std::uint32_t RED_SHIFT = 24U;
    static constexpr std::uint32_t GREEN_SHIFT = 16U;
    static constexpr std::uint32_t BLUE_SHIFT = 8U;
    static constexpr float HALF_CHANNEL = 0.5F;

    float red = 1.0F;
    float green = 1.0F;
    float blue = 1.0F;
    float alpha = 1.0F;

    constexpr Color() = default;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- RGBA 顺序是稳定公共契约。
    constexpr Color(float red, float green, float blue, float alpha = 1.0F)
        : red(red), green(green), blue(blue), alpha(alpha)
    {
    }

    /**
     * @brief 从提供 x()/y()/z()/w() 的四通道向量构造。
     *
     * 保留对 Eigen::Vector4f 等既有 vector-like 调用方的源码兼容，
     * 同时避免公共颜色头依赖具体数学库。
     */
    template <typename Vector>
        requires requires(const Vector& vector) {
            { vector.x() } -> std::convertible_to<float>;
            { vector.y() } -> std::convertible_to<float>;
            { vector.z() } -> std::convertible_to<float>;
            { vector.w() } -> std::convertible_to<float>;
        }
    explicit constexpr Color(const Vector& vector)
        : red(static_cast<float>(vector.x())),
          green(static_cast<float>(vector.y())),
          blue(static_cast<float>(vector.z())),
          alpha(static_cast<float>(vector.w()))
    {
    }

    [[nodiscard]] constexpr std::uint32_t toSDLColor() const
    {
        auto clamp = [](float value) -> std::uint8_t
        { return static_cast<std::uint8_t>(std::clamp(value, 0.0F, 1.0F) * MAX_CHANNEL_FLOAT); };
        return (static_cast<std::uint32_t>(clamp(red)) << RED_SHIFT) |
               (static_cast<std::uint32_t>(clamp(green)) << GREEN_SHIFT) |
               (static_cast<std::uint32_t>(clamp(blue)) << BLUE_SHIFT) | static_cast<std::uint32_t>(clamp(alpha));
    }

    [[nodiscard]] static constexpr Color fromSDLColor(std::uint32_t sdlColor)
    {
        return {static_cast<float>((sdlColor >> RED_SHIFT) & CHANNEL_MASK) / MAX_CHANNEL_FLOAT,
                static_cast<float>((sdlColor >> GREEN_SHIFT) & CHANNEL_MASK) / MAX_CHANNEL_FLOAT,
                static_cast<float>((sdlColor >> BLUE_SHIFT) & CHANNEL_MASK) / MAX_CHANNEL_FLOAT,
                static_cast<float>(sdlColor & CHANNEL_MASK) / MAX_CHANNEL_FLOAT};
    }

    [[nodiscard]] static constexpr Color fromRGBA(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                                                  std::uint8_t alpha = MAX_CHANNEL_BYTE)
    {
        return {static_cast<float>(red) / MAX_CHANNEL_FLOAT, static_cast<float>(green) / MAX_CHANNEL_FLOAT,
                static_cast<float>(blue) / MAX_CHANNEL_FLOAT, static_cast<float>(alpha) / MAX_CHANNEL_FLOAT};
    }

    [[nodiscard]] constexpr Color withAlpha(float newAlpha) const
    {
        return {red, green, blue, newAlpha};
    }
    [[nodiscard]] constexpr Color multiplyAlpha(float factor) const
    {
        return {red, green, blue, alpha * factor};
    }

    static constexpr Color White()
    {
        return {1.0F, 1.0F, 1.0F, 1.0F};
    }
    static constexpr Color Black()
    {
        return {0.0F, 0.0F, 0.0F, 1.0F};
    }
    static constexpr Color Red()
    {
        return {1.0F, 0.0F, 0.0F, 1.0F};
    }
    static constexpr Color Green()
    {
        return {0.0F, 1.0F, 0.0F, 1.0F};
    }
    static constexpr Color Blue()
    {
        return {0.0F, 0.0F, 1.0F, 1.0F};
    }
    static constexpr Color Yellow()
    {
        return {1.0F, 1.0F, 0.0F, 1.0F};
    }
    static constexpr Color Cyan()
    {
        return {0.0F, 1.0F, 1.0F, 1.0F};
    }
    static constexpr Color Magenta()
    {
        return {1.0F, 0.0F, 1.0F, 1.0F};
    }
    static constexpr Color Transparent()
    {
        return {0.0F, 0.0F, 0.0F, 0.0F};
    }
    static constexpr Color Gray()
    {
        return {HALF_CHANNEL, HALF_CHANNEL, HALF_CHANNEL, 1.0F};
    }
};

[[nodiscard]] constexpr Color Lerp(const Color& from, const Color& to, float alpha)
{
    const auto lerp = [alpha](float start, float end) { return start + ((end - start) * alpha); };
    return {lerp(from.red, to.red), lerp(from.green, to.green), lerp(from.blue, to.blue), lerp(from.alpha, to.alpha)};
}

}  // namespace ui
