#pragma once

#include "ui/MathTypes.hpp"

namespace ui::scale
{
[[nodiscard]] float CurrentUiScale() noexcept;
[[nodiscard]] float Metric(float value) noexcept;
[[nodiscard]] Vec2 Metric(const Vec2& value) noexcept;
} // namespace ui::scale
