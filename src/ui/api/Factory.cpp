/**
 * Implementation for UI factory API
 */

#include "Factory.hpp"

#include "../common/Components.hpp"
#include "../common/Tags.hpp"
#include "../common/Policies.hpp"
#include "../common/Types.hpp"
#include "../common/Events.hpp"
#include "../singleton/Registry.hpp"
#include "../singleton/Dispatcher.hpp"
#include <SDL3/SDL_video.h>

#include "Icon.hpp"

namespace ui::factory
{

Application CreateApplication(int argc, char* argv[])
{
    return Application(argc, argv);
}

entt::entity CreateBaseWidget(std::string_view alias)
{
    auto entity = Registry::Create();

    auto& baseInfo = Registry::Emplace<components::BaseInfo>(entity);
    baseInfo.alias = std::string(alias);

    Registry::Emplace<components::Position>(entity);
    Registry::Emplace<components::Size>(entity);
    Registry::Emplace<components::Alpha>(entity);
    Registry::Emplace<components::VisibleTag>(entity);
    Registry::Emplace<components::Hierarchy>(entity);
    Registry::Emplace<components::RootTag>(entity); // 默认标记为根节点

    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);

    return entity;
}

void CreateFadeInAnimation(entt::entity entity, float duration)
{
    if (!Registry::Valid(entity)) return;
    auto& alpha = Registry::GetOrEmplace<components::Alpha>(entity);
    alpha.value = 0.0F;
    auto& time = Registry::GetOrEmplace<components::AnimationTime>(entity);
    time.duration = duration;
    time.elapsed = 0.0F;
    auto& alphaAnim = Registry::GetOrEmplace<components::AnimationAlpha>(entity);
    alphaAnim.from = 0.0F;
    alphaAnim.to = 1.0F;
    Registry::EmplaceOrReplace<components::AnimatingTag>(entity);
}

entt::entity CreateButton(const std::string& content, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::ButtonTag>(entity);
    Registry::Emplace<components::Clickable>(entity);
    auto& text = Registry::Emplace<components::Text>(entity);
    text.content = content;
    text.alignment = ui::policies::Alignment::CENTER;
    text.fontSize = 0.0F;
    Registry::Get<components::Size>(entity).sizePolicy = ui::policies::Size::Auto;
    return entity;
}

entt::entity CreateLabel(const std::string& content, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::LabelTag>(entity);
    auto& text = Registry::Emplace<components::Text>(entity);
    text.content = content;
    Registry::Get<components::Size>(entity).sizePolicy = ui::policies::Size::Auto;
    return entity;
}

entt::entity CreateTextEdit(const std::string& placeholder, bool multiline, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);

    auto& textEdit = Registry::Emplace<components::TextEdit>(entity);
    textEdit.placeholder = placeholder;
    textEdit.inputMode = multiline ? (ui::policies::TextFlag::Default | ui::policies::TextFlag::Multiline)
                                   : ui::policies::TextFlag::Default;
    textEdit.cursorPosition = 0;
    textEdit.selectionStart = 0;
    textEdit.selectionEnd = 0;
    textEdit.hasSelection = false;

    auto& text = Registry::Emplace<components::Text>(entity);
    text.content = "";
    Registry::Emplace<components::Clickable>(entity);
    Registry::Get<components::Size>(entity).minSize = {100.0F, multiline ? 80.0F : 30.0F};
    Registry::Emplace<components::TextEditTag>(entity);

    // Add Caret component for cursor rendering
    Registry::Emplace<components::Caret>(entity);

    return entity;
}

entt::entity CreateImage(void* textureId, float defaultWidth, float defaultHeight, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::ImageTag>(entity);
    auto& image = Registry::Emplace<components::Image>(entity);
    image.textureId = textureId;
    auto& size = Registry::Get<components::Size>(entity);
    size.size = {defaultWidth, defaultHeight};
    return entity;
}

entt::entity CreateArrow(const Vec2& start, const Vec2& end, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::ArrowTag>(entity);
    auto& arrow = Registry::Emplace<components::Arrow>(entity);
    arrow.startPoint = start;
    arrow.endPoint = end;
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = ui::policies::Size::Auto;
    return entity;
}

entt::entity CreateSpacer(int stretchFactor, std::string_view alias)
{
    auto entity = Registry::Create();
    auto& baseInfo = Registry::Emplace<components::BaseInfo>(entity);
    baseInfo.alias = alias;
    Registry::Emplace<components::SpacerTag>(entity);
    Registry::Emplace<components::Hierarchy>(entity);
    Registry::Emplace<components::Position>(entity);

    // 添加 Size 组件，初始值为 0，避免布局不稳定
    auto& size = Registry::Emplace<components::Size>(entity);
    size.size = {0.0F, 0.0F};
    size.sizePolicy = ui::policies::Size::Auto;

    auto& spacer = Registry::Emplace<components::Spacer>(entity);
    spacer.stretchFactor = static_cast<uint8_t>(std::max(1, stretchFactor));

    Registry::Emplace<components::RootTag>(entity); // 默认标记为根节点
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
    return entity;
}

entt::entity CreateSpacer(float width, float height, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    auto& size = Registry::Get<components::Size>(entity);
    size.size = {width, height};
    size.sizePolicy = ui::policies::Size::Fixed;
    return entity;
}

entt::entity CreateDialog(std::string_view title, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::DialogTag>(entity);
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = ui::policies::Size::Fixed;
    auto& dialog = Registry::Emplace<components::Window>(entity);
    dialog.title = std::string(title);
    dialog.flags |= policies::WindowFlag::NoTitleBar;
    constexpr int DEFAULT_DIALOG_WIDTH = 400;
    constexpr int DEFAULT_DIALOG_HEIGHT = 300;
    SDL_Window* sdlWindow = SDL_CreateWindow(
        dialog.title.c_str(), DEFAULT_DIALOG_WIDTH, DEFAULT_DIALOG_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    dialog.windowID = SDL_GetWindowID(sdlWindow);
    Registry::Remove<components::VisibleTag>(entity);
    Registry::Emplace<components::LayoutInfo>(entity);
    Registry::Emplace<components::Padding>(entity);
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
    Logger::info("[Factory] Enqueuing WindowGraphicsContextSetEvent for dialog entity {}",
                 static_cast<uint32_t>(entity));
    Dispatcher::Trigger<events::WindowGraphicsContextSetEvent>({entity});
    return entity;
}

entt::entity CreateScrollArea(std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::ScrollArea>(entity);
    auto& layout = Registry::Emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = policies::Size::FillParent;
    return entity;
}

entt::entity CreateWindow(std::string_view title, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    Registry::Emplace<components::WindowTag>(entity);
    auto& window = Registry::Emplace<components::Window>(entity);
    window.title = std::string(title);
    window.flags &= ~policies::WindowFlag::Modal;
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = ui::policies::Size::Fixed;
    Registry::Emplace<components::LayoutInfo>(entity);
    Registry::Emplace<components::Padding>(entity);
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
    constexpr int DEFAULT_WINDOW_WIDTH = 800;
    constexpr int DEFAULT_WINDOW_HEIGHT = 600;
    SDL_Window* sdlWindow = SDL_CreateWindow(
        window.title.c_str(), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    window.windowID = SDL_GetWindowID(sdlWindow);
    Logger::info("[Factory] Enqueuing WindowGraphicsContextSetEvent for window entity {}",
                 static_cast<uint32_t>(entity));
    Dispatcher::Trigger<events::WindowGraphicsContextSetEvent>({entity});
    Registry::Remove<components::VisibleTag>(entity);
    return entity;
}

entt::entity CreateVBoxLayout(std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    auto& layout = Registry::Emplace<components::LayoutInfo>(entity);
    layout.direction = ui::policies::LayoutDirection::VERTICAL;
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = ui::policies::Size::Auto;
    Registry::Emplace<components::Padding>(entity);
    return entity;
}

entt::entity CreateHBoxLayout(std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    auto& layout = Registry::Emplace<components::LayoutInfo>(entity);
    layout.direction = ui::policies::LayoutDirection::HORIZONTAL;
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = ui::policies::Size::Auto;
    Registry::Emplace<components::Padding>(entity);
    return entity;
}

entt::entity CreateLineEdit(std::string_view initialText, std::string_view placeholder, std::string_view alias)
{
    auto entity = CreateTextEdit(std::string(placeholder), false, alias);
    auto& edit = Registry::Get<components::TextEdit>(entity);
    edit.buffer = std::string(initialText);
    edit.cursorPosition = edit.buffer.size(); // Place cursor at end
    auto& text = Registry::Get<components::Text>(entity);
    text.content = edit.buffer;
    return entity;
}

entt::entity CreateTextBrowser(std::string_view initialText, std::string_view placeholder, std::string_view alias)
{
    auto entity = CreateTextEdit(std::string(placeholder), true, alias);
    auto& edit = Registry::Get<components::TextEdit>(entity);
    edit.buffer = std::string(initialText);
    edit.cursorPosition = 0; // Start at beginning for read-only
    edit.inputMode = policies::TextFlag::ReadOnly | policies::TextFlag::Multiline;
    auto& text = Registry::Get<components::Text>(entity);
    text.content = edit.buffer;

    // 添加 ScrollArea 组件以支持滚动
    auto& scrollArea = Registry::Emplace<components::ScrollArea>(entity);
    scrollArea.scroll = policies::Scroll::Vertical;
    scrollArea.scrollBar = policies::ScrollBar::Draggable | policies::ScrollBar::AutoHide;
    scrollArea.anchor = policies::ScrollAnchor::Smart; // 设置为智能模式

    text.alignment = policies::Alignment::TOP | policies::Alignment::LEFT;
    text.wordWrap = policies::TextWrap::Word; // 自动换行

    // 确保尺寸策略允许填充父容器
    auto& size = Registry::Get<components::Size>(entity);
    size.sizePolicy = policies::Size::FillParent;

    return entity;
}

entt::entity CreateCheckBox(const std::string& label, bool checked, std::string_view alias)
{
    auto entity = CreateBaseWidget(alias);
    // TODO: 实现 CheckBoxTag 和 CheckBox 组件
    // Registry::Emplace<components::CheckBoxTag>(entity);
    // auto& checkBox = Registry::Emplace<components::CheckBox>(entity);
    // checkBox.checked = checked;
    auto& text = Registry::Emplace<components::Text>(entity);
    text.content = label;
    text.alignment = ui::policies::Alignment::LEFT | ui::policies::Alignment::VCENTER;
    Registry::Get<components::Size>(entity).sizePolicy = ui::policies::Size::Auto;
    return entity;
}

} // namespace ui::factory
