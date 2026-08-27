#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, AppliesDefaultButtonThemeOnUpdate)
{
    const auto buttonResult = factory::CreateButton("Theme", "theme_button");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;

    triggerThemeUpdate();

    const auto* background = registry().try_get<components::Background>(button);
    const auto* border = registry().try_get<components::Border>(button);
    const auto* text = registry().try_get<components::Text>(button);

    ASSERT_NE(background, nullptr);
    ASSERT_NE(border, nullptr);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(background->enabled, policies::Feature::ENABLED);
    EXPECT_EQ(border->enabled, policies::Feature::ENABLED);
    EXPECT_FLOAT_EQ(background->color.red, currentTheme().primaryButtonBackground.red);
    EXPECT_FLOAT_EQ(background->color.green, currentTheme().primaryButtonBackground.green);
    EXPECT_FLOAT_EQ(background->color.blue, currentTheme().primaryButtonBackground.blue);
    EXPECT_FLOAT_EQ(text->color.red, currentTheme().primaryButtonText.red);
    EXPECT_FLOAT_EQ(text->color.green, currentTheme().primaryButtonText.green);
    EXPECT_FLOAT_EQ(text->color.blue, currentTheme().primaryButtonText.blue);
    EXPECT_TRUE(registry().all_of<components::ThemedTag>(button));
}

TEST_F(ThemeSystemTest, ExplicitStyleIsNotOverriddenOnFirstApply)
{
    const auto buttonResult = factory::CreateButton("Theme", "theme_button_explicit");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;
    visibility::SetBackgroundColor(button, Color::Red());
    text::SetTextColor(button, Color::Yellow());

    triggerThemeUpdate();

    const auto& background = registry().get<components::Background>(button);
    const auto& text = registry().get<components::Text>(button);

    EXPECT_FLOAT_EQ(background.color.red, 1.0F);
    EXPECT_FLOAT_EQ(background.color.green, 0.0F);
    EXPECT_FLOAT_EQ(background.color.blue, 0.0F);
    EXPECT_FLOAT_EQ(text.color.red, 1.0F);
    EXPECT_FLOAT_EQ(text.color.green, 1.0F);
    EXPECT_FLOAT_EQ(text.color.blue, 0.0F);
}

TEST_F(ThemeSystemTest, SetThemeReappliesThemeOwnedValues)
{
    const auto buttonResult = factory::CreateButton("Theme", "theme_button_reapply");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;

    triggerThemeUpdate();

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.primaryButtonBackground = Color::Green();
    newTheme.primaryButtonText = Color::Black();
    setTheme(newTheme);

    triggerThemeUpdate();

    const auto& background = registry().get<components::Background>(button);
    const auto& text = registry().get<components::Text>(button);

    EXPECT_FLOAT_EQ(background.color.red, 0.0F);
    EXPECT_FLOAT_EQ(background.color.green, 1.0F);
    EXPECT_FLOAT_EQ(background.color.blue, 0.0F);
    EXPECT_FLOAT_EQ(text.color.red, 0.0F);
    EXPECT_FLOAT_EQ(text.color.green, 0.0F);
    EXPECT_FLOAT_EQ(text.color.blue, 0.0F);
}

TEST_F(ThemeSystemTest, SetThemeDoesNotOverwriteExplicitOverrideAfterFirstApply)
{
    const auto buttonResult = factory::CreateButton("Theme", "theme_button_override");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;

    triggerThemeUpdate();
    visibility::SetBackgroundColor(button, Color::Red());

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.primaryButtonBackground = Color::Green();
    setTheme(newTheme);

    triggerThemeUpdate();

    const auto& background = registry().get<components::Background>(button);
    EXPECT_FLOAT_EQ(background.color.red, 1.0F);
    EXPECT_FLOAT_EQ(background.color.green, 0.0F);
    EXPECT_FLOAT_EQ(background.color.blue, 0.0F);
}

TEST_F(ThemeSystemTest, ButtonHoverAndActiveUseThemeStateColors)
{
    const auto buttonResult = factory::CreateButton("Theme", "theme_button_states");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;

    triggerThemeUpdate();
    EXPECT_FLOAT_EQ(registry().get<components::Background>(button).color.red,
                    currentTheme().primaryButtonBackground.red);

    registry().emplace_or_replace<components::HoveredTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().primaryButtonBackgroundHover.red);
        EXPECT_FLOAT_EQ(background.color.green, currentTheme().primaryButtonBackgroundHover.green);
        EXPECT_FLOAT_EQ(background.color.blue, currentTheme().primaryButtonBackgroundHover.blue);
    }

    registry().emplace_or_replace<components::ActiveTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().primaryButtonBackgroundActive.red);
        EXPECT_FLOAT_EQ(background.color.green, currentTheme().primaryButtonBackgroundActive.green);
        EXPECT_FLOAT_EQ(background.color.blue, currentTheme().primaryButtonBackgroundActive.blue);
    }

    registry().remove<components::ActiveTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().primaryButtonBackgroundHover.red);
        EXPECT_FLOAT_EQ(background.color.green, currentTheme().primaryButtonBackgroundHover.green);
        EXPECT_FLOAT_EQ(background.color.blue, currentTheme().primaryButtonBackgroundHover.blue);
    }

    registry().remove<components::HoveredTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().primaryButtonBackground.red);
        EXPECT_FLOAT_EQ(background.color.green, currentTheme().primaryButtonBackground.green);
        EXPECT_FLOAT_EQ(background.color.blue, currentTheme().primaryButtonBackground.blue);
    }
}

TEST_F(ThemeSystemTest, ButtonDisabledUsesDisabledThemeColors)
{
    const auto buttonResult = factory::CreateButton("Theme", "theme_button_disabled");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;

    triggerThemeUpdate();
    registry().emplace_or_replace<components::DisabledTag>(button);
    triggerThemeUpdate();

    const auto& background = registry().get<components::Background>(button);
    const auto& border = registry().get<components::Border>(button);
    const auto& text = registry().get<components::Text>(button);

    EXPECT_FLOAT_EQ(background.color.red, currentTheme().primaryButtonBackgroundDisabled.red);
    EXPECT_FLOAT_EQ(border.color.red, currentTheme().disabledBorder.red);
    EXPECT_FLOAT_EQ(text.color.red, currentTheme().textDisabled.red);
}

}  // namespace ui::tests
