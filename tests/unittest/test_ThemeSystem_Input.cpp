#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, TextEditFocusedUsesThemeFocusBorderColor)
{
    const auto textEdit = factory::CreateTextEdit(runtime(), "placeholder", false, "theme_text_edit_focus");

    triggerThemeUpdate();
    {
        const auto& border = registry().get<components::Border>(textEdit);
        EXPECT_FLOAT_EQ(border.color.red, currentTheme().inputBorder.red);
        EXPECT_FLOAT_EQ(border.color.green, currentTheme().inputBorder.green);
        EXPECT_FLOAT_EQ(border.color.blue, currentTheme().inputBorder.blue);
    }

    registry().emplace_or_replace<components::FocusedTag>(textEdit);
    triggerThemeUpdate();
    {
        const auto& border = registry().get<components::Border>(textEdit);
        EXPECT_FLOAT_EQ(border.color.red, currentTheme().focusBorderColor.red);
        EXPECT_FLOAT_EQ(border.color.green, currentTheme().focusBorderColor.green);
        EXPECT_FLOAT_EQ(border.color.blue, currentTheme().focusBorderColor.blue);
    }

    registry().remove<components::FocusedTag>(textEdit);
    triggerThemeUpdate();
    {
        const auto& border = registry().get<components::Border>(textEdit);
        EXPECT_FLOAT_EQ(border.color.red, currentTheme().inputBorder.red);
        EXPECT_FLOAT_EQ(border.color.green, currentTheme().inputBorder.green);
        EXPECT_FLOAT_EQ(border.color.blue, currentTheme().inputBorder.blue);
    }
}

TEST_F(ThemeSystemTest, SetThemeReappliesThemeOwnedControlRadii)
{
    const auto buttonResult = factory::CreateButton(runtime(), "Theme", "theme_button_radius_reapply");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;
    const auto textEdit = factory::CreateTextEdit(runtime(), "placeholder", false, "theme_text_edit_radius_reapply");
    const auto dropDown = factory::CreateDropDown(runtime(), {"A", "B"}, 0, "theme_dropdown_radius_reapply");

    triggerThemeUpdate();

    {
        const auto& buttonBackground = registry().get<components::Background>(button);
        const auto& buttonBorder = registry().get<components::Border>(button);
        const auto& textEditBorder = registry().get<components::Border>(textEdit);
        const auto& dropDownBackground = registry().get<components::Background>(dropDown);
        const auto& dropDownBorder = registry().get<components::Border>(dropDown);
        EXPECT_FLOAT_EQ(buttonBackground.borderRadius.x(), currentTheme().primaryButtonRadius.x());
        EXPECT_FLOAT_EQ(buttonBorder.thickness, currentTheme().primaryButtonBorderThickness);
        EXPECT_FLOAT_EQ(textEditBorder.borderRadius.x(), currentTheme().inputControlRadius.x());
        EXPECT_FLOAT_EQ(textEditBorder.thickness, currentTheme().inputBorderThickness);
        EXPECT_FLOAT_EQ(dropDownBackground.borderRadius.x(), currentTheme().inputControlRadius.x());
        EXPECT_FLOAT_EQ(dropDownBorder.thickness, currentTheme().inputBorderThickness);
    }

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.primaryButtonRadius = Vec4{12.0F, 12.0F, 12.0F, 12.0F};
    newTheme.primaryButtonBorderThickness = 1.75F;
    newTheme.inputControlRadius = Vec4{9.0F, 9.0F, 9.0F, 9.0F};
    newTheme.inputBorderThickness = 2.25F;
    setTheme(newTheme);

    triggerThemeUpdate();

    {
        const auto& buttonBackground = registry().get<components::Background>(button);
        const auto& buttonBorder = registry().get<components::Border>(button);
        const auto& textEditBackground = registry().get<components::Background>(textEdit);
        const auto& textEditBorder = registry().get<components::Border>(textEdit);
        const auto& dropDownBackground = registry().get<components::Background>(dropDown);
        const auto& dropDownBorder = registry().get<components::Border>(dropDown);
        EXPECT_FLOAT_EQ(buttonBackground.borderRadius.x(), 12.0F);
        EXPECT_FLOAT_EQ(buttonBorder.borderRadius.x(), 12.0F);
        EXPECT_FLOAT_EQ(buttonBorder.thickness, 1.75F);
        EXPECT_FLOAT_EQ(textEditBackground.borderRadius.x(), 9.0F);
        EXPECT_FLOAT_EQ(textEditBorder.borderRadius.x(), 9.0F);
        EXPECT_FLOAT_EQ(textEditBorder.thickness, 2.25F);
        EXPECT_FLOAT_EQ(dropDownBackground.borderRadius.x(), 9.0F);
        EXPECT_FLOAT_EQ(dropDownBorder.borderRadius.x(), 9.0F);
        EXPECT_FLOAT_EQ(dropDownBorder.thickness, 2.25F);
    }
}

TEST_F(ThemeSystemTest, CheckBoxHoverActiveAndDisabledUseThemeStateColors)
{
    const auto checkBox = factory::CreateCheckBox(runtime(), "Theme", true, "theme_checkbox_states");

    triggerThemeUpdate();
    {
        const auto& checkBoxData = registry().get<components::CheckBox>(checkBox);
        EXPECT_FLOAT_EQ(checkBoxData.boxColor.red, currentTheme().surfaceBackground.red);
        EXPECT_FLOAT_EQ(checkBoxData.checkColor.red, currentTheme().accent.red);
    }

    registry().emplace_or_replace<components::HoveredTag>(checkBox);
    triggerThemeUpdate();
    {
        const auto& checkBoxData = registry().get<components::CheckBox>(checkBox);
        EXPECT_FLOAT_EQ(checkBoxData.boxColor.red, currentTheme().checkBoxBoxHover.red);
    }

    registry().emplace_or_replace<components::ActiveTag>(checkBox);
    triggerThemeUpdate();
    {
        const auto& checkBoxData = registry().get<components::CheckBox>(checkBox);
        EXPECT_FLOAT_EQ(checkBoxData.boxColor.red, currentTheme().checkBoxBoxActive.red);
    }

    registry().remove<components::HoveredTag>(checkBox);
    registry().remove<components::ActiveTag>(checkBox);
    registry().emplace_or_replace<components::DisabledTag>(checkBox);
    triggerThemeUpdate();
    {
        const auto& checkBoxData = registry().get<components::CheckBox>(checkBox);
        const auto& text = registry().get<components::Text>(checkBox);
        EXPECT_FLOAT_EQ(checkBoxData.boxColor.red, currentTheme().checkBoxBoxDisabled.red);
        EXPECT_FLOAT_EQ(checkBoxData.checkColor.red, currentTheme().accentDisabled.red);
        EXPECT_FLOAT_EQ(text.color.red, currentTheme().textDisabled.red);
    }
}

}  // namespace ui::tests
