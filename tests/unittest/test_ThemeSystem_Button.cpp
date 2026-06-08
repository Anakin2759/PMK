#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, AppliesDefaultButtonThemeOnUpdate)
{
    const auto button = factory::CreateButton("Theme", "theme_button");

    triggerThemeUpdate();

    const auto* background = registry().try_get<components::Background>(button);
    const auto* border = registry().try_get<components::Border>(button);
    const auto* text = registry().try_get<components::Text>(button);

    ASSERT_NE(background, nullptr);
    ASSERT_NE(border, nullptr);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(background->enabled, policies::Feature::ENABLED);
    EXPECT_EQ(border->enabled, policies::Feature::ENABLED);
    EXPECT_FLOAT_EQ(background->color.red, theme::CurrentTheme().primaryButtonBackground.red);
    EXPECT_FLOAT_EQ(background->color.green, theme::CurrentTheme().primaryButtonBackground.green);
    EXPECT_FLOAT_EQ(background->color.blue, theme::CurrentTheme().primaryButtonBackground.blue);
    EXPECT_FLOAT_EQ(text->color.red, theme::CurrentTheme().primaryButtonText.red);
    EXPECT_FLOAT_EQ(text->color.green, theme::CurrentTheme().primaryButtonText.green);
    EXPECT_FLOAT_EQ(text->color.blue, theme::CurrentTheme().primaryButtonText.blue);
    EXPECT_TRUE(registry().all_of<components::ThemedTag>(button));
}

TEST_F(ThemeSystemTest, ExplicitStyleIsNotOverriddenOnFirstApply)
{
    const auto button = factory::CreateButton("Theme", "theme_button_explicit");
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
    const auto button = factory::CreateButton("Theme", "theme_button_reapply");

    triggerThemeUpdate();

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.primaryButtonBackground = Color::Green();
    newTheme.primaryButtonText = Color::Black();
    theme::SetTheme(newTheme);

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
    const auto button = factory::CreateButton("Theme", "theme_button_override");

    triggerThemeUpdate();
    visibility::SetBackgroundColor(button, Color::Red());

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.primaryButtonBackground = Color::Green();
    theme::SetTheme(newTheme);

    triggerThemeUpdate();

    const auto& background = registry().get<components::Background>(button);
    EXPECT_FLOAT_EQ(background.color.red, 1.0F);
    EXPECT_FLOAT_EQ(background.color.green, 0.0F);
    EXPECT_FLOAT_EQ(background.color.blue, 0.0F);
}

TEST_F(ThemeSystemTest, ButtonHoverAndActiveUseThemeStateColors)
{
    const auto button = factory::CreateButton("Theme", "theme_button_states");

    triggerThemeUpdate();
    EXPECT_FLOAT_EQ(registry().get<components::Background>(button).color.red,
                    theme::CurrentTheme().primaryButtonBackground.red);

    registry().emplace_or_replace<components::HoveredTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundHover.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackgroundHover.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackgroundHover.blue);
    }

    registry().emplace_or_replace<components::ActiveTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundActive.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackgroundActive.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackgroundActive.blue);
    }

    registry().remove<components::ActiveTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundHover.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackgroundHover.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackgroundHover.blue);
    }

    registry().remove<components::HoveredTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackground.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackground.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackground.blue);
    }
}

TEST_F(ThemeSystemTest, ButtonDisabledUsesDisabledThemeColors)
{
    const auto button = factory::CreateButton("Theme", "theme_button_disabled");

    triggerThemeUpdate();
    registry().emplace_or_replace<components::DisabledTag>(button);
    triggerThemeUpdate();

    const auto& background = registry().get<components::Background>(button);
    const auto& border = registry().get<components::Border>(button);
    const auto& text = registry().get<components::Text>(button);

    EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundDisabled.red);
    EXPECT_FLOAT_EQ(border.color.red, theme::CurrentTheme().disabledBorder.red);
    EXPECT_FLOAT_EQ(text.color.red, theme::CurrentTheme().textDisabled.red);
}

} // namespace ui::tests
