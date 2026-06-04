#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, AppliesDefaultButtonThemeOnUpdate)
{
    const auto button = factory::CreateButton("Theme", "theme_button");

    triggerThemeUpdate();

    const auto* background = Registry::TryGet<components::Background>(button);
    const auto* border = Registry::TryGet<components::Border>(button);
    const auto* text = Registry::TryGet<components::Text>(button);

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
    EXPECT_TRUE(Registry::AllOf<components::ThemedTag>(button));
}

TEST_F(ThemeSystemTest, ExplicitStyleIsNotOverriddenOnFirstApply)
{
    const auto button = factory::CreateButton("Theme", "theme_button_explicit");
    visibility::SetBackgroundColor(button, Color::Red());
    text::SetTextColor(button, Color::Yellow());

    triggerThemeUpdate();

    const auto& background = Registry::Get<components::Background>(button);
    const auto& text = Registry::Get<components::Text>(button);

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

    const auto& background = Registry::Get<components::Background>(button);
    const auto& text = Registry::Get<components::Text>(button);

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

    const auto& background = Registry::Get<components::Background>(button);
    EXPECT_FLOAT_EQ(background.color.red, 1.0F);
    EXPECT_FLOAT_EQ(background.color.green, 0.0F);
    EXPECT_FLOAT_EQ(background.color.blue, 0.0F);
}

TEST_F(ThemeSystemTest, ButtonHoverAndActiveUseThemeStateColors)
{
    const auto button = factory::CreateButton("Theme", "theme_button_states");

    triggerThemeUpdate();
    EXPECT_FLOAT_EQ(Registry::Get<components::Background>(button).color.red,
                    theme::CurrentTheme().primaryButtonBackground.red);

    Registry::EmplaceOrReplace<components::HoveredTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundHover.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackgroundHover.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackgroundHover.blue);
    }

    Registry::EmplaceOrReplace<components::ActiveTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundActive.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackgroundActive.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackgroundActive.blue);
    }

    Registry::Remove<components::ActiveTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundHover.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackgroundHover.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackgroundHover.blue);
    }

    Registry::Remove<components::HoveredTag>(button);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(button);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackground.red);
        EXPECT_FLOAT_EQ(background.color.green, theme::CurrentTheme().primaryButtonBackground.green);
        EXPECT_FLOAT_EQ(background.color.blue, theme::CurrentTheme().primaryButtonBackground.blue);
    }
}

TEST_F(ThemeSystemTest, ButtonDisabledUsesDisabledThemeColors)
{
    const auto button = factory::CreateButton("Theme", "theme_button_disabled");

    triggerThemeUpdate();
    Registry::EmplaceOrReplace<components::DisabledTag>(button);
    triggerThemeUpdate();

    const auto& background = Registry::Get<components::Background>(button);
    const auto& border = Registry::Get<components::Border>(button);
    const auto& text = Registry::Get<components::Text>(button);

    EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().primaryButtonBackgroundDisabled.red);
    EXPECT_FLOAT_EQ(border.color.red, theme::CurrentTheme().disabledBorder.red);
    EXPECT_FLOAT_EQ(text.color.red, theme::CurrentTheme().textDisabled.red);
}

} // namespace ui::tests
