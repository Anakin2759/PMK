#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, WindowGeometryReappliesThemeOwnedRadius)
{
    const auto window = factory::CreateBaseWidget("theme_window_radius_reapply");
    registry().emplace<components::WindowTag>(window);

    const auto dialog = factory::CreateBaseWidget("theme_dialog_radius_reapply");
    registry().emplace<components::DialogTag>(dialog);

    triggerThemeUpdate();

    {
        const auto& windowBackground = registry().get<components::Background>(window);
        const auto& windowBorder = registry().get<components::Border>(window);
        const auto& dialogBorder = registry().get<components::Border>(dialog);
        EXPECT_FLOAT_EQ(windowBackground.borderRadius.x(), theme::CurrentTheme().windowPanelRadius.x());
        EXPECT_FLOAT_EQ(windowBorder.thickness, theme::CurrentTheme().windowBorderThickness);
        EXPECT_FLOAT_EQ(dialogBorder.borderRadius.x(), theme::CurrentTheme().windowPanelRadius.x());
        EXPECT_FLOAT_EQ(dialogBorder.thickness, theme::CurrentTheme().windowBorderThickness);
    }

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.windowPanelRadius = Vec4{14.0F, 14.0F, 14.0F, 14.0F};
    newTheme.windowBorderThickness = 3.0F;
    theme::SetTheme(newTheme);

    triggerThemeUpdate();

    {
        const auto& windowBackground = registry().get<components::Background>(window);
        const auto& windowBorder = registry().get<components::Border>(window);
        const auto& dialogBackground = registry().get<components::Background>(dialog);
        const auto& dialogBorder = registry().get<components::Border>(dialog);
        EXPECT_FLOAT_EQ(windowBackground.borderRadius.x(), 14.0F);
        EXPECT_FLOAT_EQ(windowBorder.borderRadius.x(), 14.0F);
        EXPECT_FLOAT_EQ(windowBorder.thickness, 3.0F);
        EXPECT_FLOAT_EQ(dialogBackground.borderRadius.x(), 14.0F);
        EXPECT_FLOAT_EQ(dialogBorder.borderRadius.x(), 14.0F);
        EXPECT_FLOAT_EQ(dialogBorder.thickness, 3.0F);
    }
}

}  // namespace ui::tests
