#pragma once

#include <Eigen/Core>

#include "ui/MathTypes.hpp"

namespace ui::detail::eigen
{

[[nodiscard]] inline Eigen::Vector2f ToEigen(const ui::Vec2& value) noexcept
{
    return {value.x(), value.y()};
}

[[nodiscard]] inline ui::Vec2 FromEigen(const Eigen::Vector2f& value) noexcept
{
    return {value.x(), value.y()};
}

} // namespace ui::detail::eigen
