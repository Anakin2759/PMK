#pragma once

#include "common/Theme.hpp"

namespace ui::theme::bridge
{

[[nodiscard]] ThemePalette DefaultDarkTheme();
void SetTheme(const ThemePalette& palette);
void UseDefaultDarkTheme();
void RequestThemeReapply();
[[nodiscard]] const ThemePalette& CurrentTheme();

} // namespace ui::theme::bridge
