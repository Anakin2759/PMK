#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, WindowGeometryReappliesThemeOwnedRadius)
{
    const auto windowResult = factory::CreateBaseWidget("theme_window_radius_reapply");
    ASSERT_TRUE(windowResult.has_value()) << windowResult.error().ToString();
    const auto window = windowResult->raw;
    registry().emplace<components::WindowTag>(window);

    const auto dialogResult = factory::CreateBaseWidget("theme_dialog_radius_reapply");
    ASSERT_TRUE(dialogResult.has_value()) << dialogResult.error().ToString();
    const auto dialog = dialogResult->raw;
    registry().emplace<components::DialogTag>(dialog);

    triggerThemeUpdate();

    {
        const auto& windowBackground = registry().get<components::Background>(window);
        const auto& windowBorder = registry().get<components::Border>(window);
        const auto& dialogBorder = registry().get<components::Border>(dialog);
        EXPECT_FLOAT_EQ(windowBackground.borderRadius.x(), currentTheme().windowPanelRadius.x());
        EXPECT_FLOAT_EQ(windowBorder.thickness, currentTheme().windowBorderThickness);
        EXPECT_FLOAT_EQ(dialogBorder.borderRadius.x(), currentTheme().windowPanelRadius.x());
        EXPECT_FLOAT_EQ(dialogBorder.thickness, currentTheme().windowBorderThickness);
    }

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.windowPanelRadius = Vec4{14.0F, 14.0F, 14.0F, 14.0F};
    newTheme.windowBorderThickness = 3.0F;
    setTheme(newTheme);

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
