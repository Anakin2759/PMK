#include "tests/support/ThemeSystemTest.hpp"

#include "src/helper/Helper.hpp"

namespace ui::tests
{

TEST_F(ThemeSystemTest, DropDownHoverActiveAndDisabledUseThemeStateColors)
{
    const auto dropDown = factory::CreateDropDown({"A", "B"}, 0, "theme_dropdown_states");

    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(dropDown);
        const auto& dropDownData = registry().get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().inputBackground.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, currentTheme().dropDownArrow.red);
    }

    registry().emplace_or_replace<components::HoveredTag>(dropDown);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(dropDown);
        const auto& dropDownData = registry().get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().inputBackgroundHover.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, currentTheme().dropDownArrowHover.red);
    }

    registry().emplace_or_replace<components::ActiveTag>(dropDown);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(dropDown);
        const auto& dropDownData = registry().get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().inputBackgroundActive.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, currentTheme().dropDownArrowActive.red);
    }

    registry().remove<components::HoveredTag>(dropDown);
    registry().remove<components::ActiveTag>(dropDown);
    registry().emplace_or_replace<components::DisabledTag>(dropDown);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(dropDown);
        const auto& border = registry().get<components::Border>(dropDown);
        const auto& text = registry().get<components::Text>(dropDown);
        const auto& dropDownData = registry().get<components::DropDown>(dropDown);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().inputBackgroundDisabled.red);
        EXPECT_FLOAT_EQ(border.color.red, currentTheme().inputBorderDisabled.red);
        EXPECT_FLOAT_EQ(text.color.red, currentTheme().textDisabled.red);
        EXPECT_FLOAT_EQ(dropDownData.arrowColor.red, currentTheme().dropDownArrowDisabled.red);
    }
}

TEST_F(ThemeSystemTest, DropDownPopupItemsUseThemeSelectedAndHoverStates)
{
    const auto owner = factory::CreateDropDown({"A", "B", "C"}, 0, "theme_dropdown_popup_owner");
    const auto popupPanelResult = factory::CreateBaseWidget("theme_dropdown_popup_panel");
    ASSERT_TRUE(popupPanelResult.has_value()) << popupPanelResult.error().ToString();
    const auto popupPanel = popupPanelResult->raw;
    registry().emplace<components::DropDownPopupPanel>(popupPanel).owner = detail::ToInternal(owner);
    registry().emplace<components::LayoutInfo>(popupPanel).direction = policies::LayoutDirection::VERTICAL;

    const auto selectedItemResult = factory::CreateBaseWidget("theme_dropdown_popup_item_selected");
    ASSERT_TRUE(selectedItemResult.has_value()) << selectedItemResult.error().ToString();
    const auto selectedItem = selectedItemResult->raw;
    registry().emplace<components::DropDownPopupItem>(selectedItem, detail::ToInternal(owner), 0);
    registry().emplace<components::Text>(selectedItem).content = "A";

    const auto hoveredItemResult = factory::CreateBaseWidget("theme_dropdown_popup_item_hovered");
    ASSERT_TRUE(hoveredItemResult.has_value()) << hoveredItemResult.error().ToString();
    const auto hoveredItem = hoveredItemResult->raw;
    registry().emplace<components::DropDownPopupItem>(hoveredItem, detail::ToInternal(owner), 1);
    registry().emplace<components::Text>(hoveredItem).content = "B";

    const auto idleItemResult = factory::CreateBaseWidget("theme_dropdown_popup_item_idle");
    ASSERT_TRUE(idleItemResult.has_value()) << idleItemResult.error().ToString();
    const auto idleItem = idleItemResult->raw;
    registry().emplace<components::DropDownPopupItem>(idleItem, detail::ToInternal(owner), 2);
    registry().emplace<components::Text>(idleItem).content = "C";

    hierarchy::AddChild(popupPanel, selectedItem);
    hierarchy::AddChild(popupPanel, hoveredItem);
    hierarchy::AddChild(popupPanel, idleItem);

    triggerThemeUpdate();

    const auto& popupBackground = registry().get<components::Background>(popupPanel);
    const auto& popupBorder = registry().get<components::Border>(popupPanel);
    EXPECT_FLOAT_EQ(popupBackground.color.red, currentTheme().popupBackground.red);
    EXPECT_FLOAT_EQ(popupBorder.color.red, currentTheme().popupBorder.red);

    const auto children = popupChildren(popupPanel);
    ASSERT_EQ(children.size(), 3U);

    {
        const entt::entity selectedChild = children.front();
        ASSERT_TRUE(registry().all_of<components::DropDownPopupItem>(selectedChild));
        const auto& background = registry().get<components::Background>(selectedChild);
        const auto& text = registry().get<components::Text>(selectedChild);
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().popupItemBackgroundSelected.red);
        EXPECT_FLOAT_EQ(text.color.red, currentTheme().popupItemTextSelected.red);
    }

    registry().emplace_or_replace<components::HoveredTag>(hoveredItem);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(children.at(1));
        const auto& text = registry().get<components::Text>(children.at(1));
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().popupItemBackgroundHover.red);
        EXPECT_FLOAT_EQ(text.color.red, currentTheme().popupItemText.red);
    }

    registry().remove<components::HoveredTag>(hoveredItem);
    registry().emplace_or_replace<components::ActiveTag>(hoveredItem);
    triggerThemeUpdate();
    {
        const auto& background = registry().get<components::Background>(children.at(1));
        const auto& text = registry().get<components::Text>(children.at(1));
        EXPECT_FLOAT_EQ(background.color.red, currentTheme().popupItemBackgroundActive.red);
        EXPECT_FLOAT_EQ(text.color.red, currentTheme().popupItemText.red);
    }

    registry().remove<components::ActiveTag>(hoveredItem);
    registry().get<components::DropDown>(owner).selectedIndex = 1;
    triggerThemeUpdate();
    {
        const auto& oldSelectedBackground = registry().get<components::Background>(children.front());
        const auto& oldSelectedText = registry().get<components::Text>(children.front());
        EXPECT_FLOAT_EQ(oldSelectedBackground.color.red, currentTheme().popupItemBackground.red);
        EXPECT_FLOAT_EQ(oldSelectedText.color.red, currentTheme().popupItemText.red);

        const auto& newSelectedBackground = registry().get<components::Background>(children.at(1));
        const auto& newSelectedText = registry().get<components::Text>(children.at(1));
        EXPECT_FLOAT_EQ(newSelectedBackground.color.red, currentTheme().popupItemBackgroundSelected.red);
        EXPECT_FLOAT_EQ(newSelectedText.color.red, currentTheme().popupItemTextSelected.red);
    }
}

TEST_F(ThemeSystemTest, DropDownPopupGeometryReappliesThemeOwnedRadii)
{
    const auto owner = factory::CreateDropDown({"A", "B"}, 0, "theme_dropdown_popup_geometry_owner");
    const auto popupPanelResult = factory::CreateBaseWidget("theme_dropdown_popup_geometry_panel");
    ASSERT_TRUE(popupPanelResult.has_value()) << popupPanelResult.error().ToString();
    const auto popupPanel = popupPanelResult->raw;
    registry().emplace<components::DropDownPopupPanel>(popupPanel).owner = detail::ToInternal(owner);

    const auto popupItemResult = factory::CreateBaseWidget("theme_dropdown_popup_geometry_item");
    ASSERT_TRUE(popupItemResult.has_value()) << popupItemResult.error().ToString();
    const auto popupItem = popupItemResult->raw;
    registry().emplace<components::DropDownPopupItem>(popupItem, detail::ToInternal(owner), 0);
    registry().emplace<components::Text>(popupItem).content = "A";
    hierarchy::AddChild(popupPanel, popupItem);

    triggerThemeUpdate();

    {
        const auto& panelBackground = registry().get<components::Background>(popupPanel);
        const auto& panelBorder = registry().get<components::Border>(popupPanel);
        const auto& itemBackground = registry().get<components::Background>(popupItem);
        EXPECT_FLOAT_EQ(panelBackground.borderRadius.x(), currentTheme().popupPanelRadius.x());
        EXPECT_FLOAT_EQ(panelBorder.borderRadius.x(), currentTheme().popupPanelRadius.x());
        EXPECT_FLOAT_EQ(panelBorder.thickness, currentTheme().popupBorderThickness);
        EXPECT_FLOAT_EQ(itemBackground.borderRadius.x(), currentTheme().popupItemRadius.x());
    }

    auto newTheme = theme::DefaultDarkTheme();
    newTheme.popupPanelRadius = Vec4{10.0F, 10.0F, 10.0F, 10.0F};
    newTheme.popupBorderThickness = 2.5F;
    newTheme.popupItemRadius = Vec4{4.0F, 4.0F, 4.0F, 4.0F};
    setTheme(newTheme);

    triggerThemeUpdate();

    {
        const auto& panelBackground = registry().get<components::Background>(popupPanel);
        const auto& panelBorder = registry().get<components::Border>(popupPanel);
        const auto& itemBackground = registry().get<components::Background>(popupItem);
        EXPECT_FLOAT_EQ(panelBackground.borderRadius.x(), 10.0F);
        EXPECT_FLOAT_EQ(panelBorder.borderRadius.x(), 10.0F);
        EXPECT_FLOAT_EQ(panelBorder.thickness, 2.5F);
        EXPECT_FLOAT_EQ(itemBackground.borderRadius.x(), 4.0F);
    }
}

}  // namespace ui::tests
