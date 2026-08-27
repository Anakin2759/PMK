#include "ui/api/Theme.hpp"

#include "helper/Helper.hpp"

namespace ui::theme
{

ThemePalette DefaultDarkTheme()
{
    return bridge::DefaultDarkTheme();
}

void SetTheme(const ThemePalette& palette)
{
    bridge::SetTheme(UiRuntime::current(), palette);
}

void UseDefaultDarkTheme()
{
    SetTheme(DefaultDarkTheme());
}

void RequestThemeReapply()
{
    bridge::RequestThemeReapply(UiRuntime::current());
}

const ThemePalette& CurrentTheme()
{
    return bridge::CurrentTheme(UiRuntime::current());
}

}  // namespace ui::theme