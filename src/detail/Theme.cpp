#include "ThemeBridge.hpp"

#include "core/UiRuntime.hpp"

namespace ui::theme::bridge
{

ThemePalette DefaultDarkTheme()
{
    return ThemePalette{};
}

void SetTheme(const ThemePalette& palette)
{
    auto& context = UiRuntime::current().ensureContext<ThemeContext>();
    context.previousPalette = context.palette;
    context.palette = palette;
    ++context.version;
    context.reapplyRequested = true;
}

void UseDefaultDarkTheme()
{
    SetTheme(DefaultDarkTheme());
}

void RequestThemeReapply()
{
    auto& context = UiRuntime::current().ensureContext<ThemeContext>();
    context.previousPalette = context.palette;
    context.reapplyRequested = true;
}

const ThemePalette& CurrentTheme()
{
    return UiRuntime::current().ensureContext<ThemeContext>().palette;
}

} // namespace ui::theme::bridge