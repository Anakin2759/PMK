#include "ui/api/Theme.hpp"

#include "helper/Helper.hpp"

namespace ui::theme
{

ThemePalette DefaultDarkTheme()
{
    return bridge::DefaultDarkTheme();
}

void SetTheme(UiRuntime& runtime, const ThemePalette& palette)
{
    bridge::SetTheme(runtime, palette);
}

void UseDefaultDarkTheme(UiRuntime& runtime)
{
    SetTheme(runtime, DefaultDarkTheme());
}

void RequestThemeReapply(UiRuntime& runtime)
{
    bridge::RequestThemeReapply(runtime);
}

const ThemePalette& CurrentTheme(UiRuntime& runtime)
{
    return bridge::CurrentTheme(runtime);
}

}  // namespace ui::theme