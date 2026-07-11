#include "Theme.hpp"

#include "helper/Helper.hpp"

namespace ui::theme
{

ThemePalette DefaultDarkTheme()
{
    return bridge::DefaultDarkTheme();
}

void SetTheme(const ThemePalette& palette)
{
    bridge::SetTheme(palette);
}

void UseDefaultDarkTheme()
{
    SetTheme(DefaultDarkTheme());
}

void RequestThemeReapply()
{
    bridge::RequestThemeReapply();
}

const ThemePalette& CurrentTheme()
{
    return bridge::CurrentTheme();
}

} // namespace ui::theme