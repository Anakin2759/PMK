/**
 * ************************************************************************
 *
 * @file test_MainWindow.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-03-20
 * @version 0.3
 *
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <memory>
#include <algorithm>
#include <string>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

#include <ui.hpp>

#include "common/Tags.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Data.hpp"
#include "common/components/Visual.hpp"
#include <entt/entt.hpp>
#include "common/components/Interaction.hpp"

namespace ui::tests
{
namespace
{

Registry& ActiveRegistry()
{
    return UiRuntime::current().registry();
}

class UiIntegrationTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
    }
    void TearDown() override
    {
        m_scope.reset();
    }

    [[nodiscard]] UiRuntime& runtime() noexcept
    {
        return m_runtime;
    }

   private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

bool ContainsChild(const components::Hierarchy& hierarchy, entt::entity child)
{
    return std::ranges::find(hierarchy.children, child) != hierarchy.children.end();
}

void ConfigureTextBrowser(UiRuntime& runtime, ui::entity browser, bool& submitCalled, std::string& changedText)
{
    using namespace ui::chains;

    WithRuntime(runtime, browser,
                TextEditContent("ok") | TextColor(Color::Red()) | PasswordMode(policies::TextFlag::PASSWORD) |
                    OnSubmit([&submitCalled]() { submitCalled = true; }) |
                    OnTextChanged([&changedText](const std::string& value) { changedText = value; }));
}

void ExpectTextBrowserCallbacks(components::TextEdit& textEdit, bool& submitCalled, std::string& changedText)
{
    ASSERT_TRUE(static_cast<bool>(textEdit.onSubmit));
    ASSERT_TRUE(static_cast<bool>(textEdit.onTextChanged));

    textEdit.onSubmit();
    textEdit.onTextChanged(textEdit.buffer);

    EXPECT_TRUE(submitCalled);
    EXPECT_EQ(changedText, "ok");
}

void ExpectTextBrowserBufferState(const components::TextEdit& textEdit)
{
    EXPECT_EQ(textEdit.buffer, "ok");
    EXPECT_EQ(textEdit.cursorPosition, 0U);
    EXPECT_FALSE(textEdit.hasSelection);
    EXPECT_EQ(textEdit.selectionStart, 0U);
    EXPECT_EQ(textEdit.selectionEnd, 0U);
}

void ExpectTextBrowserColorAndMode(const components::TextEdit& textEdit)
{
    EXPECT_FLOAT_EQ(textEdit.textColor.red, 1.0F);
    EXPECT_FLOAT_EQ(textEdit.textColor.green, 0.0F);
    EXPECT_FLOAT_EQ(textEdit.textColor.blue, 0.0F);
    EXPECT_NE(textEdit.inputMode, policies::TextFlag::READ_ONLY_MULTILINE);
    EXPECT_NE(textEdit.inputMode, policies::TextFlag::DEFAULT);
}

void ExpectTextBrowserLayoutState(ui::entity browser)
{
    auto& registry = ActiveRegistry();
    const auto& text = registry.get<components::Text>(browser);
    const auto& scrollArea = registry.get<components::ScrollArea>(browser);
    const auto& size = registry.get<components::Size>(browser);

    EXPECT_EQ(text.alignment, policies::Alignment::TOP | policies::Alignment::LEFT);
    EXPECT_EQ(text.wordWrap, policies::TextWrap::WORD);
    EXPECT_EQ(scrollArea.scroll, policies::Scroll::VERTICAL);
    EXPECT_EQ(size.sizePolicy, policies::Size::FILL_PARENT);
}

}  // namespace

TEST_F(UiIntegrationTest, DslBuildsWidgetTreeWithinUiModule)
{
    using namespace ui::chains;

    const auto root = factory::CreateVBoxLayout(runtime(), "root_layout");
    const auto buttonResult = factory::CreateButton(runtime(), "Start", "start_button");
    ASSERT_TRUE(buttonResult.has_value()) << buttonResult.error().ToString();
    const auto button = buttonResult->raw;
    const auto editor = factory::CreateLineEdit(runtime(), "seed", "placeholder", "name_input");

    WithRuntime(runtime(), root, Spacing(12.0F) | Padding(8.0F) | AddChild(button) | AddChild(editor));
    WithRuntime(runtime(), button,
                FixedSize(180.0F, 44.0F) | BackgroundColor(Color::Blue()) | BorderRadius(6.0F) |
                    BorderColor(Color::White()) | BorderThickness(2.0F) | Text("Ready") | FontSize(18.0F) |
                    TextAlignment(policies::Alignment::CENTER) | Show());

    auto& registry = ActiveRegistry();
    const auto& rootHierarchy = registry.get<components::Hierarchy>(root);
    const auto& rootLayout = registry.get<components::LayoutInfo>(root);
    const auto& rootPadding = registry.get<components::Padding>(root);
    const auto& buttonHierarchy = registry.get<components::Hierarchy>(button);
    const auto& buttonSize = registry.get<components::Size>(button);
    const auto& buttonText = registry.get<components::Text>(button);
    const auto& buttonBackground = registry.get<components::Background>(button);
    const auto& buttonBorder = registry.get<components::Border>(button);

    ASSERT_EQ(rootHierarchy.children.size(), 2U);
    EXPECT_EQ(rootHierarchy.children.at(0), static_cast<entt::entity>(button));
    EXPECT_EQ(rootHierarchy.children.at(1), static_cast<entt::entity>(editor));
    EXPECT_EQ(buttonHierarchy.parent, static_cast<entt::entity>(root));
    EXPECT_EQ(registry.get<components::BaseInfo>(root).alias, "root_layout");
    EXPECT_EQ(registry.get<components::BaseInfo>(button).alias, "start_button");
    EXPECT_EQ(rootLayout.direction, policies::LayoutDirection::VERTICAL);
    EXPECT_FLOAT_EQ(rootLayout.spacing, 12.0F);
    EXPECT_FLOAT_EQ(rootPadding.values.x(), 8.0F);
    EXPECT_FLOAT_EQ(rootPadding.values.y(), 8.0F);
    EXPECT_FLOAT_EQ(rootPadding.values.z(), 8.0F);
    EXPECT_FLOAT_EQ(rootPadding.values.w(), 8.0F);
    EXPECT_EQ(buttonSize.sizePolicy, policies::Size::FIXED);
    EXPECT_FLOAT_EQ(buttonSize.size.x(), 180.0F);
    EXPECT_FLOAT_EQ(buttonSize.size.y(), 44.0F);
    EXPECT_EQ(buttonText.content, "Ready");
    EXPECT_FLOAT_EQ(buttonText.fontSize, 18.0F);
    EXPECT_EQ(buttonText.alignment, policies::Alignment::CENTER);
    EXPECT_EQ(buttonBackground.enabled, policies::Feature::ENABLED);
    EXPECT_FLOAT_EQ(buttonBackground.color.blue, 1.0F);
    EXPECT_FLOAT_EQ(buttonBackground.borderRadius.x(), 6.0F);
    EXPECT_EQ(buttonBorder.enabled, policies::Feature::ENABLED);
    EXPECT_FLOAT_EQ(buttonBorder.thickness, 2.0F);
    EXPECT_TRUE(registry.all_of<components::VisibleTag>(button));
    EXPECT_FALSE(registry.all_of<components::RootTag>(button));
    EXPECT_FALSE(registry.all_of<components::RootTag>(editor));
    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(root));
}

TEST_F(UiIntegrationTest, ReparentAndRemoveChildKeepHierarchyConsistent)
{
    const auto firstParent = factory::CreateHBoxLayout(runtime(), "first_parent");
    const auto secondParent = factory::CreateVBoxLayout(runtime(), "second_parent");
    const auto child = factory::CreateLabel(runtime(), "Status", "status_label");

    hierarchy::AddChild(runtime(), firstParent, child);

    auto& registry = ActiveRegistry();
    auto& firstHierarchy = registry.get<components::Hierarchy>(firstParent);
    ASSERT_EQ(firstHierarchy.children.size(), 1U);
    EXPECT_TRUE(ContainsChild(firstHierarchy, static_cast<entt::entity>(child)));
    EXPECT_EQ(registry.get<components::Hierarchy>(child).parent, static_cast<entt::entity>(firstParent));

    hierarchy::AddChild(runtime(), secondParent, child);

    const auto& secondHierarchy = registry.get<components::Hierarchy>(secondParent);
    EXPECT_TRUE(firstHierarchy.children.empty());
    ASSERT_EQ(secondHierarchy.children.size(), 1U);
    EXPECT_TRUE(ContainsChild(secondHierarchy, static_cast<entt::entity>(child)));
    EXPECT_EQ(registry.get<components::Hierarchy>(child).parent, static_cast<entt::entity>(secondParent));
    EXPECT_FALSE(registry.all_of<components::RootTag>(child));
    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(firstParent));
    EXPECT_TRUE(registry.all_of<components::LayoutDirtyTag>(secondParent));

    hierarchy::RemoveChild(runtime(), secondParent, child);

    EXPECT_TRUE(registry.get<components::Hierarchy>(secondParent).children.empty());
    EXPECT_TRUE(registry.get<components::Hierarchy>(child).parent == entt::null);
    EXPECT_TRUE(registry.all_of<components::RootTag>(child));
}

TEST_F(UiIntegrationTest, TextBrowserFactoryCombinesScrollableReadOnlyEditorState)
{
    const auto browser = factory::CreateTextBrowser(runtime(), "hello world", "unused", "log_browser");

    bool submitCalled = false;
    std::string changedText;
    ConfigureTextBrowser(runtime(), browser, submitCalled, changedText);

    auto& textEdit = ActiveRegistry().get<components::TextEdit>(browser);
    ExpectTextBrowserCallbacks(textEdit, submitCalled, changedText);
    ExpectTextBrowserBufferState(textEdit);
    ExpectTextBrowserColorAndMode(textEdit);
    ExpectTextBrowserLayoutState(browser);
}

TEST_F(UiIntegrationTest, ContainerFactoriesUseSensibleDefaultAlignment)
{
    const auto vbox = factory::CreateVBoxLayout(runtime(), "vbox_layout");
    const auto hbox = factory::CreateHBoxLayout(runtime(), "hbox_layout");
    const auto scrollArea = factory::CreateScrollArea(runtime(), "scroll_area");

    auto& registry = ActiveRegistry();
    const auto& vboxLayout = registry.get<components::LayoutInfo>(vbox);
    const auto& hboxLayout = registry.get<components::LayoutInfo>(hbox);
    const auto& scrollLayout = registry.get<components::LayoutInfo>(scrollArea);

    EXPECT_EQ(vboxLayout.direction, policies::LayoutDirection::VERTICAL);
    EXPECT_EQ(vboxLayout.alignment, policies::Alignment::TOP_LEFT);

    EXPECT_EQ(hboxLayout.direction, policies::LayoutDirection::HORIZONTAL);
    EXPECT_EQ(hboxLayout.alignment, policies::Alignment::LEFT | policies::Alignment::VCENTER);

    EXPECT_EQ(scrollLayout.direction, policies::LayoutDirection::VERTICAL);
    EXPECT_EQ(scrollLayout.alignment, policies::Alignment::TOP_LEFT);
}

}  // namespace ui::tests
