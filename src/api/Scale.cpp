#include "ui/api/Scale.hpp"

#include <cmath>

#include "common/AppConfig.hpp"

namespace ui::scale
{
float CurrentUiScale() noexcept
{
    return config::AppConfig::instance().platformUiScale();
}

float Metric(float value) noexcept
{
    if (!std::isfinite(value) || value == 0.0F)
    {
        return value;
    }
    return value * CurrentUiScale();
}

Vec2 Metric(const Vec2& value) noexcept
{
    return {Metric(value.x()), Metric(value.y())};
}
} // namespace ui::scale