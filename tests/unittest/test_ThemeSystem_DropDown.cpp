#include "tests/support/ThemeSystemTest.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, DropDownHoverActiveAndDisabledUseThemeStateColors)
{
    const auto dropDown = factory::CreateDropDown({"A", "B"}, 0, "theme_dropdown_states");

    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(dropDown);
        const auto& dropDownData = Registry::Get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().inputBackground.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, theme::CurrentTheme().dropDownArrow.red);
    }

    Registry::EmplaceOrReplace<components::HoveredTag>(dropDown);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(dropDown);
        const auto& dropDownData = Registry::Get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().inputBackgroundHover.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, theme::CurrentTheme().dropDownArrowHover.red);
    }

    Registry::EmplaceOrReplace<components::ActiveTag>(dropDown);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(dropDown);
        const auto& dropDownData = Registry::Get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().inputBackgroundActive.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, theme::CurrentTheme().dropDownArrowActive.red);
    }

    Registry::Remove<components::HoveredTag>(dropDown);
    Registry::Remove<components::ActiveTag>(dropDown);
    Registry::EmplaceOrReplace<components::DisabledTag>(dropDown);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(dropDown);
        const auto& border = Registry::Get<components::Border>(dropDown);
        const auto& text = Registry::Get<components::Text>(dropDown);
        const auto& dropDownData = Registry::Get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().inputBackgroundDisabled.red);
        EXPECT_FLOAT_EQ(border.color.red, theme::CurrentTheme().inputBorderDisabled.red);
        EXPECT_FLOAT_EQ(text.color.red, theme::CurrentTheme().textDisabled.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, theme::CurrentTheme().dropDownArrowDisabled.red);
    }
}

TEST_F(ThemeSystemTest, DropDownPopupItemsUseThemeSelectedAndHoverStates)
{
    const auto owner = factory::CreateDropDown({"A", "B", "C"}, 0, "theme_dropdown_popup_owner");
    const auto popupPanel = factory::CreateBaseWidget("theme_dropdown_popup_panel");
    Registry::Emplace<components::DropDownPopupPanel>(popupPanel).owner = owner;
    Registry::Emplace<components::LayoutInfo>(popupPanel).direction = policies::LayoutDirection::VERTICAL;

    const auto selectedItem = factory::CreateBaseWidget("theme_dropdown_popup_item_selected");
    Registry::Emplace<components::DropDownPopupItem>(selectedItem, owner, 0);
    Registry::Emplace<components::Text>(selectedItem).content = "A";

    const auto hoveredItem = factory::CreateBaseWidget("theme_dropdown_popup_item_hovered");
    Registry::Emplace<components::DropDownPopupItem>(hoveredItem, owner, 1);
    Registry::Emplace<components::Text>(hoveredItem).content = "B";

    const auto idleItem = factory::CreateBaseWidget("theme_dropdown_popup_item_idle");
    Registry::Emplace<components::DropDownPopupItem>(idleItem, owner, 2);
    Registry::Emplace<components::Text>(idleItem).content = "C";

    hierarchy::AddChild(popupPanel, selectedItem);
    hierarchy::AddChild(popupPanel, hoveredItem);
    hierarchy::AddChild(popupPanel, idleItem);

    triggerThemeUpdate();

    const auto& popupBackground = Registry::Get<components::Background>(popupPanel);
    const auto& popupBorder = Registry::Get<components::Border>(popupPanel);
    EXPECT_FLOAT_EQ(popupBackground.color.red, theme::CurrentTheme().popupBackground.red);
    EXPECT_FLOAT_EQ(popupBorder.color.red, theme::CurrentTheme().popupBorder.red);

    const auto children = popupChildren(popupPanel);
    ASSERT_EQ(children.size(), 3U);

    {
        const entt::entity selectedChild = children.front();
        ASSERT_TRUE(Registry::AllOf<components::DropDownPopupItem>(selectedChild));
        const auto& background = Registry::Get<components::Background>(selectedChild);
        const auto& text = Registry::Get<components::Text>(selectedChild);
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().popupItemBackgroundSelected.red);
        EXPECT_FLOAT_EQ(text.color.red, theme::CurrentTheme().popupItemTextSelected.red);
    }

    Registry::EmplaceOrReplace<components::HoveredTag>(hoveredItem);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(children.at(1));
        const auto& text = Registry::Get<components::Text>(children.at(1));
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().popupItemBackgroundHover.red);
        EXPECT_FLOAT_EQ(text.color.red, theme::CurrentTheme().popupItemText.red);
    }

    Registry::Remove<components::HoveredTag>(hoveredItem);
    Registry::EmplaceOrReplace<components::ActiveTag>(hoveredItem);
    triggerThemeUpdate();
    {
        const auto& background = Registry::Get<components::Background>(children.at(1));
        const auto& text = Registry::Get<components::Text>(children.at(1));
        EXPECT_FLOAT_EQ(background.color.red, theme::CurrentTheme().popupItemBackgroundActive.red);
        EXPECT_FLOAT_EQ(text.color.red, theme::CurrentTheme().popupItemText.red);
    }

    Registry::Remove<components::ActiveTag>(hoveredItem);
    Registry::Get<components::DropDown>(owner).selectedIndex = 1;
    triggerThemeUpdate();
    {
        const auto& oldSelectedBackground = Registry::Get<components::Background>(children.front());
        const auto& oldSelectedText = Registry::Get<components::Text>(children.front());
        EXPECT_FLOAT_EQ(oldSelectedBackground.color.red, theme::CurrentTheme().popupItemBackground.red);
        EXPECT_FLOAT_EQ(oldSelectedText.color.red, theme::CurrentTheme().popupItemText.red);

        const auto& newSelectedBackground = Registry::Get<components::Background>(children.at(1));
        const auto& newSelectedText = Registry::Get<components::Text>(children.at(1));
        EXPECT_FLOAT_EQ(newSelectedBackground.color.red, theme::CurrentTheme().popupItemBackgroundSelected.red);
        EXPECT_FLOAT_EQ(newSelectedText.color.red, theme::CurrentTheme().popupItemTextSelected.red);
    }
}

TEST_F(ThemeSystemTest, DropDownPopupGeometryReappliesThemeOwnedRadii)
{
    const auto owner = factory::CreateDropDown({"A", "B"}, 0, "theme_dropdown_popup_geometry_owner");
    const auto popupPanel = factory::CreateBaseWidget("theme_dropdown_popup_geometry_panel");
    Registry::Emplace<components::DropDownPopupPanel>(popupPanel).owner = owner;

    const auto popupItem = factory::CreateBaseWidget("theme_dropdown_popup_geometry_item");
    Registry::Emplace<components::DropDownPopupItem>(popupItem, owner, 0);
    Registry::Emplace<components::Text>(popupItem).content = "A";
    hierarchy::AddChild(popupPanel, popupItem);

    triggerThemeUpdate();

    {
        const auto& panelBackground = Registry::Get<components::Background>(popupPanel);
        const auto& panelBorder = Registry::Get<components::Border>(popupPanel);
        const auto& itemBackground = Registry::Get<components::Background>(popupItem);
        EXPECT_FLOAT_EQ(panelBackground.borderRadius.x(), theme::CurrentTheme().popupPanelRadius.x());
        EXPECT_FLOAT_EQ(panelBorder.borderRadius.x(), theme::CurrentTheme().popupPanelRadius.x());
        EXPECT_FLOAT_EQ(panelBorder.thickness, theme::CurrentTheme().popupBorderThickness);
        EXPECT_FLOAT_EQ(itemBackground.borderRadius.x(), theme::CurrentTheme().popupItemRadius.x());
    }

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.popupPanelRadius = Vec4{10.0F, 10.0F, 10.0F, 10.0F};
    newTheme.popupBorderThickness = 2.5F;
    newTheme.popupItemRadius = Vec4{4.0F, 4.0F, 4.0F, 4.0F};
    theme::SetTheme(newTheme);

    triggerThemeUpdate();

    {
        const auto& panelBackground = Registry::Get<components::Background>(popupPanel);
        const auto& panelBorder = Registry::Get<components::Border>(popupPanel);
        const auto& itemBackground = Registry::Get<components::Background>(popupItem);
        EXPECT_FLOAT_EQ(panelBackground.borderRadius.x(), 10.0F);
        EXPECT_FLOAT_EQ(panelBorder.borderRadius.x(), 10.0F);
        EXPECT_FLOAT_EQ(panelBorder.thickness, 2.5F);
        EXPECT_FLOAT_EQ(itemBackground.borderRadius.x(), 4.0F);
    }
}

} // namespace ui::tests
