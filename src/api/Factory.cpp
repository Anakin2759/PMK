

#include "ui/api/Factory.hpp"
#include "ui/api/Scale.hpp"
#include "common/Tags.hpp"
#include "ui/Policies.hpp"
#include "common/Types.hpp"
#include "common/Events.hpp"
#include "ui/ErrorCodes.hpp"
#include "common/AppConfig.hpp"
#include "core/UiRuntimeScope.hpp"
#include "core/WindowEntityLookup.hpp"
#include "utils/Logger.hpp"
#include "utils/Registry.hpp"
#include "utils/Dispatcher.hpp"
#include "ui/api/Hierarchy.hpp"
#include "SDL3/SDL_error.h"
#include "ui/api/Utils.hpp"
#include "ui/api/Animation.hpp"
#include "core/PlatformWindow.hpp"
#include "systems/TimerSystem.hpp"
#include "entt/entity/fwd.hpp"
#include "common/components/Window.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Visual.hpp"
#include "common/components/Interaction.hpp"
#include "common/components/Data.hpp"
#include "ui/Result.hpp"
#include "core/UiRuntime.hpp"
#include "helper/Helper.hpp"
#include "entt/entity/entity.hpp"
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_surface.h>
#include <stb_image.h>

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <string_view>
#include <string>
#include <span>

namespace ui::factory
{

namespace
{
void WriteStderr(std::string_view text) noexcept
{
    if (text.empty())
    {
        return;
    }
    if (std::fwrite(text.data(), 1U, text.size(), stderr) != text.size())
    {
        std::clearerr(stderr);
    }
}

struct RuntimeServices
{
    Registry& registry;
    Dispatcher& dispatcher;
};

RuntimeServices CurrentServices()
{
    auto& runtime = UiRuntime::current();
    return {.registry = runtime.registry(), .dispatcher = runtime.dispatcher()};
}

Registry& CurrentRegistry()
{
    return CurrentServices().registry;
}

struct TitleBarDragState
{
    Vec2 dragStartMouseGlobal{0.0F, 0.0F};
    Vec2 dragStartWindowPos{0.0F, 0.0F};
    bool dragAnchorValid = false;
};

SDL_Window* CreateSdlWindowOrRollback(ui::entity entity, const char* title, int width, int height,
                                      SDL_WindowFlags flags, std::string_view entityType)
{
    auto& reg = CurrentRegistry();
    SDL_Window* sdlWindow = SDL_CreateWindow(title, width, height, flags);
    if (sdlWindow == nullptr)
    {
        UiRuntime::current().logger().error("[Factory] Failed to create SDL window for {} entity {}: {}", entityType,
                                            static_cast<uint32_t>(entity), SDL_GetError());
        reg.destroy(entity);
        return nullptr;
    }

    // 应用应用程序图标（若已通过 AppConfig 配置）
    const auto iconPath = ui::config::AppConfig::instance().appIconPath();
    if (!iconPath.empty())
    {
        int wid = 0;
        int hei = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load(std::string(iconPath).c_str(), &wid, &hei, &channels, 4);
        if (pixels != nullptr)
        {
            SDL_Surface* surface = SDL_CreateSurfaceFrom(wid, hei, SDL_PIXELFORMAT_RGBA32, pixels, wid * 4);
            if (surface != nullptr)
            {
                SDL_SetWindowIcon(sdlWindow, surface);
                SDL_DestroySurface(surface);
            }
            else
            {
                UiRuntime::current().logger().warn("[Factory] Failed to create surface for app icon: {}",
                                                   SDL_GetError());
            }
            stbi_image_free(pixels);
        }
        else
        {
            UiRuntime::current().logger().warn("[Factory] Failed to load app icon '{}': {}", iconPath,
                                               stbi_failure_reason());
        }
    }

    return sdlWindow;
}

SDL_WindowFlags DefaultWindowFlags()
{
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    if (ui::config::AppConfig::instance().platformScalingEnabled())
    {
        flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    }
    return flags;
}

bool AssignWindowIdOrRollback(ui::entity entity, components::Window& window, SDL_Window* sdlWindow,
                              std::string_view entityType)
{
    auto& reg = CurrentRegistry();
    window.windowID = SDL_GetWindowID(sdlWindow);
    if (window.windowID == 0)
    {
        UiRuntime::current().logger().error("[Factory] Failed to fetch SDL window ID for {} entity {}: {}", entityType,
                                            static_cast<uint32_t>(entity), SDL_GetError());
        SDL_DestroyWindow(sdlWindow);
        reg.destroy(entity);
        return false;
    }

    window_lookup::RememberWindowEntity(detail::ToInternal(entity));

    return true;
}

ui::entity CreateTitleBarContainer(std::string_view alias, float titleBarHeight)
{
    auto& reg = CurrentRegistry();
    auto titleBar = CreateBaseWidget(alias);
    reg.emplace<components::TitleBarTag>(titleBar);

    auto& layout = reg.emplace<components::LayoutInfo>(titleBar);
    layout.direction = policies::LayoutDirection::HORIZONTAL;
    layout.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;

    auto& titleBarSize = reg.get<components::Size>(titleBar);
    titleBarSize.size = {0.0F, scale::Metric(titleBarHeight)};
    titleBarSize.sizePolicy = policies::Size::H_FILL | policies::Size::V_FIXED;

    auto& background = reg.emplace<components::Background>(titleBar);
    background.color = {0.0F, 0.0F, 0.0F, 0.0F};
    background.enabled = policies::Feature::ENABLED;

    reg.emplace<components::Clickable>(titleBar);
    auto& draggable = reg.emplace<components::Draggable>(titleBar);
    draggable.lockX = true;
    draggable.lockY = true;

    return titleBar;
}

void ConfigureTitleBarDragging(ui::entity titleBar, ui::entity windowEntity, uint32_t windowId)
{
    auto& reg = CurrentRegistry();
    auto& draggable = reg.get<components::Draggable>(titleBar);
    auto dragState = std::make_shared<TitleBarDragState>();

    draggable.onDragStart = [windowEntity, windowId, dragState]()
    {
        auto& reg = CurrentRegistry();
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowId);
        auto* position = reg.try_get<components::Position>(windowEntity);
        if (sdlWindow == nullptr || position == nullptr)
        {
            dragState->dragAnchorValid = false;
            return;
        }

        float globalMouseX = 0.0F;
        float globalMouseY = 0.0F;
        SDL_GetGlobalMouseState(&globalMouseX, &globalMouseY);

        int windowX = 0;
        int windowY = 0;
        SDL_GetWindowPosition(sdlWindow, &windowX, &windowY);

        dragState->dragStartMouseGlobal = Vec2{globalMouseX, globalMouseY};
        dragState->dragStartWindowPos = Vec2{static_cast<float>(windowX), static_cast<float>(windowY)};
        position->value = dragState->dragStartWindowPos;
        dragState->dragAnchorValid = true;
    };

    draggable.onDragMove = [windowEntity, windowId, dragState]([[maybe_unused]] Vec2 delta)
    {
        auto& reg = CurrentRegistry();
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowId);
        if (sdlWindow == nullptr)
            return;

        auto* position = reg.try_get<components::Position>(windowEntity);
        if (position == nullptr)
            return;

        float globalMouseX = 0.0F;
        float globalMouseY = 0.0F;
        SDL_GetGlobalMouseState(&globalMouseX, &globalMouseY);

        int currentX = 0;
        int currentY = 0;
        SDL_GetWindowPosition(sdlWindow, &currentX, &currentY);

        constexpr float POSITION_EPSILON = 0.01F;
        if (!dragState->dragAnchorValid)
        {
            dragState->dragStartMouseGlobal = Vec2{globalMouseX, globalMouseY};
            dragState->dragStartWindowPos = Vec2{static_cast<float>(currentX), static_cast<float>(currentY)};
            dragState->dragAnchorValid = true;
        }
        else if (std::abs(position->value.x()) < POSITION_EPSILON && std::abs(position->value.y()) < POSITION_EPSILON)
        {
            position->value = Vec2{static_cast<float>(currentX), static_cast<float>(currentY)};
        }

        const Vec2 globalDelta = Vec2{globalMouseX, globalMouseY} - dragState->dragStartMouseGlobal;
        position->value = dragState->dragStartWindowPos + globalDelta;

        const int targetX = static_cast<int>(std::lround(position->value.x()));
        const int targetY = static_cast<int>(std::lround(position->value.y()));

        if (targetX != currentX || targetY != currentY)
        {
            SDL_SetWindowPosition(sdlWindow, targetX, targetY);
        }
    };

    draggable.onDragEnd = [dragState]() { dragState->dragAnchorValid = false; };
}

ui::entity CreateWindowControlButton(const std::string& buttonAlias, uint32_t iconCodepoint, float buttonSize,
                                     float iconSize, float iconSpacing)
{
    auto& reg = CurrentRegistry();
    auto button = CreateButton("", buttonAlias);
    auto& buttonSizeComp = reg.get<components::Size>(button);
    buttonSizeComp.size = {scale::Metric(buttonSize), scale::Metric(buttonSize)};
    buttonSizeComp.sizePolicy = policies::Size::FIXED;

    auto& buttonBackground = reg.get_or_emplace<components::Background>(button);
    buttonBackground.color = {0.0F, 0.0F, 0.0F, 0.0F};
    const float buttonRadius = scale::Metric(4.0F);
    buttonBackground.borderRadius = {buttonRadius, buttonRadius, buttonRadius, buttonRadius};
    buttonBackground.enabled = policies::Feature::ENABLED;

    auto& iconComp = reg.emplace<components::Icon>(button);
    iconComp.codepoint = iconCodepoint;
    iconComp.size = {scale::Metric(iconSize), scale::Metric(iconSize)};
    iconComp.spacing = scale::Metric(iconSpacing);
    iconComp.tintColor = {0.85F, 0.85F, 0.85F, 1.0F};

    return button;
}

void AppendChild(ui::entity parent, ui::entity child)
{
    auto& reg = CurrentRegistry();
    auto& parentHierarchy = reg.get<components::Hierarchy>(parent);
    auto& childHierarchy = reg.get<components::Hierarchy>(child);
    childHierarchy.parent = detail::ToInternal(parent);
    reg.remove<components::RootTag>(child);
    parentHierarchy.children.push_back(detail::ToInternal(child));
}

void AttachTitleBarToWindow(ui::entity titleBar, ui::entity windowEntity)
{
    auto& reg = CurrentRegistry();
    auto& windowHierarchy = reg.get<components::Hierarchy>(windowEntity);
    auto& titleBarHierarchy = reg.get<components::Hierarchy>(titleBar);
    titleBarHierarchy.parent = detail::ToInternal(windowEntity);
    reg.remove<components::RootTag>(titleBar);
    windowHierarchy.children.insert(windowHierarchy.children.begin(), detail::ToInternal(titleBar));
}

void MarkAsRoot(ui::entity entity)
{
    CurrentRegistry().emplace_or_replace<components::RootTag>(entity);
}
}  // namespace

ui::Result<std::unique_ptr<Application>> CreateApplication(std::span<char*> argv)
{
    try
    {
        return std::make_unique<Application>(argv);
    }
    catch (const std::exception& e)
    {
        WriteStderr("[Factory] UI initialization failed: ");
        WriteStderr(e.what());
        WriteStderr("\n");
        return ui::Err(UiErrc::DEVICE_UNAVAILABLE, e.what());
    }
    catch (...)
    {
        WriteStderr("[Factory] Unknown UI initialization failure\n");
        return ui::Err(UiErrc::UNKNOWN);
    }
}

ui::entity CreateBaseWidget(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = detail::ToPublic(reg.create());

    auto& baseInfo = reg.emplace<components::BaseInfo>(entity);
    baseInfo.alias = std::string(alias);

    reg.emplace<components::Position>(entity);
    reg.emplace<components::Size>(entity);
    reg.emplace<components::Alpha>(entity);
    reg.emplace<components::VisibleTag>(entity);
    reg.emplace<components::Hierarchy>(entity);

    utils::MarkLayoutAndVisualChanged(entity);

    return entity;
}

ui::Result<ui::EntityHandle> CreateBaseWidget(UiRuntime& runtime, std::string_view alias)
{
    UiRuntimeScope const scope(runtime);
    const ui::entity entity = CreateBaseWidget(alias);
    if (entity == ui::null_entity)
    {
        return ui::Err(UiErrc::INVALID_ENTITY, std::string(alias));
    }
    return ui::MakeEntityHandle(runtime.token(), entity);
}

void CreateFadeInAnimation(ui::entity entity, float duration)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity))
        return;
    auto& alpha = reg.get_or_emplace<components::Alpha>(entity);
    alpha.value = 0.0F;
    animation::TweenOptions options;
    options.duration = duration;
    options.easing = policies::Easing::EASE_OUT_QUAD;
    options.mode = policies::Play::ONCE;
    options.autoCleanup = true;
    animation::StartAlphaAnimation(entity, 0.0F, 1.0F, options);
}

ui::entity CreateButton(const std::string& content, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ButtonTag>(entity);
    reg.emplace<components::FocusableTag>(entity);
    reg.emplace<components::Clickable>(entity);
    auto& text = reg.emplace<components::Text>(entity);
    text.content = content;
    text.alignment = policies::Alignment::CENTER;
    text.fontSize = 0.0F;
    reg.get<components::Size>(entity).sizePolicy = policies::Size::AUTO;
    return entity;
}

ui::Result<ui::EntityHandle> CreateButton(UiRuntime& runtime, const std::string& content, std::string_view alias)
{
    UiRuntimeScope const scope(runtime);
    const ui::entity entity = CreateButton(content, alias);
    if (entity == ui::null_entity)
    {
        return ui::Err(UiErrc::INVALID_ENTITY, std::string(alias));
    }
    return ui::MakeEntityHandle(runtime.token(), entity);
}

ui::entity CreateLabel(const std::string& content, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::LabelTag>(entity);
    auto& text = reg.emplace<components::Text>(entity);
    text.content = content;
    reg.get<components::Size>(entity).sizePolicy = policies::Size::AUTO;
    return entity;
}

ui::entity CreateTextEdit(const std::string& placeholder, bool multiline, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);

    auto& textEdit = reg.emplace<components::TextEdit>(entity);
    textEdit.placeholder = placeholder;
    textEdit.inputMode =
        multiline ? (policies::TextFlag::DEFAULT | policies::TextFlag::MULTILINE) : policies::TextFlag::DEFAULT;
    textEdit.cursorPosition = 0;
    textEdit.selectionStart = 0;
    textEdit.selectionEnd = 0;
    textEdit.hasSelection = false;

    auto& text = reg.emplace<components::Text>(entity);
    text.content = "";
    reg.emplace<components::Clickable>(entity);
    reg.get<components::Size>(entity).minSize = {scale::Metric(100.0F), scale::Metric(multiline ? 80.0F : 30.0F)};
    reg.emplace<components::TextEditTag>(entity);
    reg.emplace<components::FocusableTag>(entity);

    // Add Caret component for cursor rendering
    reg.emplace<components::Caret>(entity);

    return entity;
}

ui::entity CreateImage(void* textureId, float defaultWidth, float defaultHeight, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ImageTag>(entity);
    auto& image = reg.emplace<components::Image>(entity);
    image.textureId = textureId;
    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(defaultWidth), scale::Metric(defaultHeight)};
    return entity;
}

ui::entity CreateArrow(const Vec2& start, const Vec2& end, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ArrowTag>(entity);
    auto& arrow = reg.emplace<components::Arrow>(entity);
    arrow.startPoint = scale::Metric(start);
    arrow.endPoint = scale::Metric(end);
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;
    return entity;
}

ui::entity CreateSpacer(int stretchFactor, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = detail::ToPublic(reg.create());
    auto& baseInfo = reg.emplace<components::BaseInfo>(entity);
    baseInfo.alias = alias;
    reg.emplace<components::SpacerTag>(entity);
    reg.emplace<components::Hierarchy>(entity);
    reg.emplace<components::Position>(entity);

    auto& size = reg.emplace<components::Size>(entity);
    size.size = {0.0F, 0.0F};
    size.sizePolicy = policies::Size::AUTO;

    auto& spacer = reg.emplace<components::Spacer>(entity);
    spacer.stretchFactor = static_cast<uint8_t>(std::max(1, stretchFactor));

    utils::MarkLayoutAndVisualChanged(entity);
    return entity;
}

ui::entity CreateSpacer(float width, float height, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(width), scale::Metric(height)};
    size.sizePolicy = policies::Size::FIXED;
    return entity;
}

ui::entity CreateDialog(std::string_view title, std::string_view alias)
{
    const auto services = CurrentServices();
    auto& reg = services.registry;
    auto& disp = services.dispatcher;
    auto entity = CreateBaseWidget(alias);
    MarkAsRoot(entity);
    reg.emplace<components::DialogTag>(entity);
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::FIXED;
    auto& dialog = reg.emplace<components::Window>(entity);
    dialog.title = std::string(title);
    dialog.flags |= policies::WindowFlag::NO_TITLE_BAR;
    constexpr int DEFAULT_DIALOG_WIDTH = 400;
    constexpr int DEFAULT_DIALOG_HEIGHT = 300;
    SDL_Window* sdlWindow = CreateSdlWindowOrRollback(entity, dialog.title.c_str(), DEFAULT_DIALOG_WIDTH,
                                                      DEFAULT_DIALOG_HEIGHT, DefaultWindowFlags(), "dialog");
    if (sdlWindow == nullptr)
    {
        return ui::null_entity;
    }

    if (!AssignWindowIdOrRollback(entity, dialog, sdlWindow, "dialog"))
    {
        return ui::null_entity;
    }

    SDL_SetWindowBordered(sdlWindow, false);
    platform::EnableTransparency(sdlWindow);
    reg.remove<components::VisibleTag>(entity);
    auto& dialogLayout = reg.emplace<components::LayoutInfo>(entity);
    dialogLayout.direction = policies::LayoutDirection::VERTICAL;
    dialogLayout.alignment = policies::Alignment::CENTER;
    reg.emplace<components::Padding>(entity);
    utils::MarkLayoutAndVisualChanged(entity);
    UiRuntime::current().logger().info("[Factory] Triggering WindowGraphicsContextSetEvent for dialog entity {}",
                                       static_cast<uint32_t>(entity));
    disp.trigger<events::WindowGraphicsContextSetEvent>({detail::ToInternal(entity)});

    // 自定义 Dialog 默认无标题栏（NO_TITLE_BAR），如需自绘标题栏请显式调用 CreateTitleBar。

    return entity;
}

ui::entity CreateScrollArea(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ScrollArea>(entity);
    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    layout.alignment = policies::Alignment::TOP_LEFT;
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::FILL_PARENT;
    return entity;
}

ui::entity CreateWindow(std::string_view title, std::string_view alias)
{
    const auto services = CurrentServices();
    auto& reg = services.registry;
    auto& disp = services.dispatcher;
    auto entity = CreateBaseWidget(alias);
    MarkAsRoot(entity);
    reg.emplace<components::WindowTag>(entity);
    auto& window = reg.emplace<components::Window>(entity);
    window.title = std::string(title);
    window.flags &= ~policies::WindowFlag::MODAL;
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::FIXED;
    auto& layoutInfo = reg.emplace<components::LayoutInfo>(entity);
    layoutInfo.direction = policies::LayoutDirection::VERTICAL;
    layoutInfo.alignment = policies::Alignment::CENTER;
    reg.emplace<components::Padding>(entity);
    ui::utils::MarkLayoutAndVisualChanged(entity);
    constexpr int DEFAULT_WINDOW_WIDTH = 800;
    constexpr int DEFAULT_WINDOW_HEIGHT = 600;
    SDL_Window* sdlWindow = CreateSdlWindowOrRollback(entity, window.title.c_str(), DEFAULT_WINDOW_WIDTH,
                                                      DEFAULT_WINDOW_HEIGHT, DefaultWindowFlags(), "window");
    if (sdlWindow == nullptr)
    {
        return ui::null_entity;
    }

    if (!AssignWindowIdOrRollback(entity, window, sdlWindow, "window"))
    {
        return ui::null_entity;
    }

    platform::InstallDarkClientAreaBackground(sdlWindow);

    UiRuntime::current().logger().info("[Factory] Triggering WindowGraphicsContextSetEvent for window entity {}",
                                       static_cast<uint32_t>(entity));
    disp.trigger<events::WindowGraphicsContextSetEvent>({detail::ToInternal(entity)});
    reg.remove<components::VisibleTag>(entity);

    return entity;
}

ui::Result<ui::WindowHandle> CreateWindow(UiRuntime& runtime, std::string_view title, std::string_view alias)
{
    UiRuntimeScope const scope(runtime);
    const ui::entity entity = CreateWindow(title, alias);
    if (entity == ui::null_entity)
    {
        return ui::Err(UiErrc::INVALID_ENTITY, std::string(alias));
    }

    const auto* window = runtime.registry().try_get<components::Window>(entity);
    const std::uint32_t windowId = window != nullptr ? window->windowID : 0U;
    return ui::MakeWindowHandle(runtime.token(), entity, windowId);
}

ui::entity CreateTitleBar(ui::entity windowEntity, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto* windowComp = reg.try_get<components::Window>(windowEntity);
    if (windowComp == nullptr)
    {
        UiRuntime::current().logger().warn("[Factory] CreateTitleBar: entity {} has no Window component",
                                           static_cast<uint32_t>(windowEntity));
        return ui::null_entity;
    }

    constexpr float TITLE_BAR_HEIGHT = 32.0F;
    constexpr float BTN_SIZE = 28.0F;
    constexpr float ICON_SIZE = 16.0F;
    constexpr float ICON_SPACING = 0.0F;
    constexpr float BTN_SPACING = 2.0F;
    constexpr uint32_t ICON_CLOSE = 0xE5CD;
    constexpr uint32_t ICON_MINIMIZE = 0xE931;
    constexpr uint32_t ICON_MAXIMIZE = 0xE930;

    auto titleBar = CreateTitleBarContainer(alias, TITLE_BAR_HEIGHT);
    uint32_t const windowID = windowComp->windowID;
    ConfigureTitleBarDragging(titleBar, windowEntity, windowID);

    auto titleLabel = CreateLabel(windowComp->title, std::string(alias) + "_title");
    auto& titleText = reg.get<components::Text>(titleLabel);
    titleText.fontSize = scale::Metric(13.0F);
    titleText.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;

    auto spacer = CreateSpacer(1, std::string(alias) + "_spacer");

    auto minimizeBtn =
        CreateWindowControlButton(std::string(alias) + "_minimize", ICON_MINIMIZE, BTN_SIZE, ICON_SIZE, ICON_SPACING);
    reg.get<components::Clickable>(minimizeBtn).onClick = [windowID]()
    {
        SDL_Window* sdlWin = SDL_GetWindowFromID(windowID);
        if (sdlWin != nullptr)
            SDL_MinimizeWindow(sdlWin);
    };

    auto maximizeBtn =
        CreateWindowControlButton(std::string(alias) + "_maximize", ICON_MAXIMIZE, BTN_SIZE, ICON_SIZE, ICON_SPACING);
    reg.get<components::Clickable>(maximizeBtn).onClick = [windowID]()
    {
        SDL_Window* sdlWin = SDL_GetWindowFromID(windowID);
        if (sdlWin == nullptr)
            return;
        if ((SDL_GetWindowFlags(sdlWin) & SDL_WINDOW_MAXIMIZED) != 0)
        {
            SDL_RestoreWindow(sdlWin);
        }
        else
        {
            SDL_MaximizeWindow(sdlWin);
        }
    };

    auto closeBtn =
        CreateWindowControlButton(std::string(alias) + "_close", ICON_CLOSE, BTN_SIZE, ICON_SIZE, ICON_SPACING);
    Registry* const regPtr = &reg;
    auto& closeBtnHover = reg.emplace<components::Hoverable>(closeBtn);
    closeBtnHover.onHover = [regPtr, closeBtn]()
    {
        auto& reg = *regPtr;
        auto* closeBg = reg.try_get<components::Background>(closeBtn);
        if (closeBg != nullptr)
            closeBg->color = {0.9F, 0.2F, 0.2F, 1.0F};
    };
    closeBtnHover.onUnhover = [regPtr, closeBtn]()
    {
        auto& reg = *regPtr;
        auto* closeBg = reg.try_get<components::Background>(closeBtn);
        if (closeBg != nullptr)
            closeBg->color = {0.0F, 0.0F, 0.0F, 0.0F};
    };
    reg.get<components::Clickable>(closeBtn).onClick = [windowEntity]() { utils::CloseWindow(windowEntity); };

    AppendChild(titleBar, titleLabel);
    AppendChild(titleBar, spacer);
    AppendChild(titleBar, minimizeBtn);
    AppendChild(titleBar, maximizeBtn);
    AppendChild(titleBar, closeBtn);
    AttachTitleBarToWindow(titleBar, windowEntity);

    utils::MarkLayoutAndVisualChanged(titleBar);
    utils::MarkLayoutAndVisualChanged(windowEntity);

    auto& padding = reg.emplace<components::Padding>(titleBar);
    padding.values = {0.0F, scale::Metric(BTN_SPACING), 0.0F, scale::Metric(8.0F)};

    auto& layoutInfo = reg.get<components::LayoutInfo>(titleBar);
    layoutInfo.spacing = scale::Metric(BTN_SPACING);

    return titleBar;
}

ui::entity CreateVBoxLayout(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    layout.alignment = policies::Alignment::TOP_LEFT;
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;
    reg.emplace<components::Padding>(entity);
    return entity;
}

ui::entity CreateHBoxLayout(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::HORIZONTAL;
    layout.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;
    reg.emplace<components::Padding>(entity);
    return entity;
}

ui::entity CreateLineEdit(std::string_view initialText, std::string_view placeholder, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateTextEdit(std::string(placeholder), false, alias);
    auto& edit = reg.get<components::TextEdit>(entity);
    edit.buffer = std::string(initialText);
    edit.cursorPosition = edit.buffer.size();  // Place cursor at end
    auto& text = reg.get<components::Text>(entity);
    text.content = edit.buffer;
    return entity;
}

ui::entity CreateTextBrowser(std::string_view initialText, std::string_view placeholder, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateTextEdit(std::string(placeholder), true, alias);
    auto& edit = reg.get<components::TextEdit>(entity);
    edit.buffer = std::string(initialText);
    edit.cursorPosition = 0;  // Start at beginning for read-only
    edit.inputMode = policies::TextFlag::READ_ONLY_MULTILINE;
    auto& text = reg.get<components::Text>(entity);
    text.content = edit.buffer;

    auto& scrollArea = reg.emplace<components::ScrollArea>(entity);
    scrollArea.scroll = policies::Scroll::VERTICAL;
    scrollArea.scrollBar = policies::ScrollBar::DRAGGABLE | policies::ScrollBar::AUTO_HIDE;
    scrollArea.anchor = policies::ScrollAnchor::SMART;

    text.alignment = policies::Alignment::TOP | policies::Alignment::LEFT;
    text.wordWrap = policies::TextWrap::WORD;

    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::FILL_PARENT;

    return entity;
}

ui::entity CreateCheckBox(const std::string& label, bool checked, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::CheckBoxTag>(entity);
    reg.emplace<components::FocusableTag>(entity);
    auto& checkBox = reg.emplace<components::CheckBox>(entity);
    checkBox.checked = checked;
    checkBox.label = label;
    auto& text = reg.emplace<components::Text>(entity);
    text.content = label;
    text.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
    auto& padding = reg.get_or_emplace<components::Padding>(entity);
    padding.values = {0.0F, 0.0F, 0.0F, scale::Metric(24.0F)};  // Top, Right, Bottom, Left
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;
    size.size = {scale::Metric(120.0F), scale::Metric(22.0F)};
    auto& clickable = reg.emplace<components::Clickable>(entity);
    Registry* const regPtr = &reg;
    clickable.onClick = [regPtr, entity]()
    {
        auto& reg = *regPtr;
        auto* checkBoxComp = reg.try_get<components::CheckBox>(entity);
        if (checkBoxComp == nullptr)
            return;
        checkBoxComp->checked = !checkBoxComp->checked;
        if (checkBoxComp->onChanged)
        {
            checkBoxComp->onChanged(checkBoxComp->checked);
        }
        ui::utils::MarkVisualChanged(entity);
    };
    return entity;
}

ui::entity CreateSwitch(bool checked, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::SwitchTag>(entity);
    reg.emplace<components::FocusableTag>(entity);
    auto& switchComp = reg.emplace<components::Switch>(entity);
    switchComp.checked = checked;
    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(40.0F), scale::Metric(22.0F)};
    size.sizePolicy = policies::Size::FIXED;
    auto& clickable = reg.emplace<components::Clickable>(entity);
    clickable.onClick = [entity]()
    {
        auto& reg = CurrentRegistry();
        auto* switchComp = reg.try_get<components::Switch>(entity);
        if (switchComp == nullptr)
            return;
        switchComp->checked = !switchComp->checked;
        if (switchComp->onChanged)
        {
            switchComp->onChanged(switchComp->checked);
        }
        ui::utils::MarkVisualChanged(entity);
    };
    return entity;
}

ui::entity CreateRadioGroup(const std::vector<std::string>& options, int selectedIndex, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::RadioGroupTag>(entity);
    auto& group = reg.emplace<components::RadioGroup>(entity);
    group.selectedIndex = selectedIndex;
    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    layout.alignment = policies::Alignment::TOP_LEFT;
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;

    const int optionCount = static_cast<int>(options.size());
    for (int index = 0; index < optionCount; ++index)
    {
        const std::string& label = options[static_cast<std::size_t>(index)];
        const std::string optionAlias = std::string(alias) + "_option_" + std::to_string(index);
        const auto option = CreateBaseWidget(optionAlias);
        reg.emplace<components::RadioButtonTag>(option);
        reg.emplace<components::FocusableTag>(option);
        auto& radioButton = reg.emplace<components::RadioButton>(option);
        radioButton.checked = (index == selectedIndex);
        radioButton.label = label;
        radioButton.group = detail::ToInternal(entity);
        radioButton.optionIndex = index;
        auto& text = reg.emplace<components::Text>(option);
        text.content = label;
        text.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
        auto& padding = reg.get_or_emplace<components::Padding>(option);
        padding.values = {0.0F, 0.0F, 0.0F, scale::Metric(24.0F)};
        auto& optionSize = reg.get<components::Size>(option);
        optionSize.sizePolicy = policies::Size::AUTO;
        optionSize.size = {scale::Metric(120.0F), scale::Metric(22.0F)};
        auto& clickable = reg.emplace<components::Clickable>(option);
        Registry* const regPtr = &reg;
        clickable.onClick = [regPtr, option]()
        {
            auto& reg = *regPtr;
            auto* radioButton = reg.try_get<components::RadioButton>(option);
            if (radioButton == nullptr || radioButton->group == entt::null)
                return;
            auto* group = reg.try_get<components::RadioGroup>(radioButton->group);
            if (group == nullptr)
                return;

            // 点击已选中项：no-op，避免重复触发 onChanged
            if (radioButton->checked && group->selectedIndex == radioButton->optionIndex)
                return;

            const entt::entity optionInternal = detail::ToInternal(option);

            // 先统一修改状态（回调期间不持有 view 迭代器，避免迭代器失效 UB）
            entt::entity newlyChecked = entt::null;
            auto view = reg.view<components::RadioButton>();
            for (const entt::entity member : view)
            {
                auto& memberButton = view.get<components::RadioButton>(member);
                if (memberButton.group != radioButton->group)
                    continue;
                const bool shouldBeChecked = (member == optionInternal);
                if (shouldBeChecked != memberButton.checked)
                {
                    memberButton.checked = shouldBeChecked;
                    if (shouldBeChecked)
                    {
                        newlyChecked = member;
                    }
                    ui::utils::MarkVisualChanged(member);
                }
            }
            group->selectedIndex = radioButton->optionIndex;

            // 状态修改完成后统一触发回调：仅新选中项触发 onChanged(true)
            if (newlyChecked != entt::null)
            {
                if (auto* checkedButton = reg.try_get<components::RadioButton>(newlyChecked);
                    checkedButton != nullptr && checkedButton->onChanged)
                {
                    checkedButton->onChanged(true);
                }
            }
            if (group->onChanged)
            {
                group->onChanged(group->selectedIndex);
            }
        };
        hierarchy::AddChild(entity, option);
    }
    return entity;
}

namespace
{
constexpr float kTabHeaderHeight = 28.0F;
constexpr float kTabHeaderPaddingLeft = 12.0F;
constexpr float kTabHeaderPaddingRight = 12.0F;
constexpr float kTabHeaderRadius = 4.0F;

// Tab 头选中/未选中配色（与 Switch/RadioButton 的 accent 一致）
constexpr Color kTabSelectedBackground{0.31F, 0.67F, 0.98F, 1.0F};
constexpr Color kTabUnselectedBackground{0.18F, 0.19F, 0.24F, 1.0F};
constexpr Color kTabSelectedText{0.97F, 0.98F, 1.0F, 1.0F};
constexpr Color kTabUnselectedText{0.72F, 0.75F, 0.82F, 1.0F};

void ApplyTabVisualState(Registry& reg, entt::entity header, bool selected)
{
    if (auto* background = reg.try_get<components::Background>(header); background != nullptr)
    {
        background->color = selected ? kTabSelectedBackground : kTabUnselectedBackground;
        background->enabled = policies::Feature::ENABLED;
    }
    if (auto* text = reg.try_get<components::Text>(header); text != nullptr)
    {
        text->color = selected ? kTabSelectedText : kTabUnselectedText;
    }
    if (auto* tabItem = reg.try_get<components::TabItem>(header); tabItem != nullptr)
    {
        tabItem->selected = selected;
    }
    ui::utils::MarkVisualChanged(header);
}

void SelectTab(Registry& reg, entt::entity tabViewEntity, int index)
{
    auto* tabView = reg.try_get<components::TabView>(tabViewEntity);
    if (tabView == nullptr || index < 0 || index >= static_cast<int>(tabView->tabHeaders.size()))
        return;

    if (tabView->selectedIndex == index)
        return;

    tabView->selectedIndex = index;

    const std::size_t tabCount = tabView->tabHeaders.size();
    for (std::size_t i = 0; i < tabCount; ++i)
    {
        const bool isSelected = (static_cast<int>(i) == index);
        if (tabView->tabHeaders[i] != entt::null && reg.valid(tabView->tabHeaders[i]))
        {
            ApplyTabVisualState(reg, tabView->tabHeaders[i], isSelected);
        }
        // 内容面板：仅选中页可见
        if (i < tabView->contentPanels.size() && tabView->contentPanels[i] != entt::null &&
            reg.valid(tabView->contentPanels[i]))
        {
            if (isSelected)
            {
                reg.emplace_or_replace<components::VisibleTag>(tabView->contentPanels[i]);
            }
            else
            {
                reg.remove<components::VisibleTag>(tabView->contentPanels[i]);
            }
            ui::utils::MarkVisualChanged(tabView->contentPanels[i]);
        }
    }

    if (tabView->onChanged)
    {
        tabView->onChanged(tabView->selectedIndex);
    }
}

}  // namespace

ui::entity CreateTabView(const std::vector<std::string>& tabTitles, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::TabViewTag>(entity);
    auto& tabView = reg.emplace<components::TabView>(entity);
    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    layout.alignment = policies::Alignment::TOP_LEFT;
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;

    // TabBar（水平）
    const auto tabBar = CreateBaseWidget(std::string(alias) + "_tabbar");
    reg.emplace<components::LayoutInfo>(tabBar).direction = policies::LayoutDirection::HORIZONTAL;
    reg.get<components::LayoutInfo>(tabBar).alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
    reg.get<components::Size>(tabBar).sizePolicy = policies::Size::AUTO;

    const int tabCount = static_cast<int>(tabTitles.size());
    for (int index = 0; index < tabCount; ++index)
    {
        const std::string& title = tabTitles[static_cast<std::size_t>(index)];

        // Tab 头
        const auto header = CreateBaseWidget(std::string(alias) + "_tab_" + std::to_string(index));
        reg.emplace<components::TabItemTag>(header);
        reg.emplace<components::FocusableTag>(header);
        auto& tabItem = reg.emplace<components::TabItem>(header);
        tabItem.owner = detail::ToInternal(entity);
        tabItem.tabIndex = index;
        tabItem.selected = (index == 0);

        auto& text = reg.emplace<components::Text>(header);
        text.content = title;
        text.fontSize = scale::Metric(13.0F);
        text.alignment = policies::Alignment::CENTER | policies::Alignment::VCENTER;

        auto& background = reg.emplace<components::Background>(header);
        background.borderRadius = Vec4{kTabHeaderRadius, kTabHeaderRadius, kTabHeaderRadius, kTabHeaderRadius};
        background.enabled = policies::Feature::ENABLED;

        auto& headerPadding = reg.get_or_emplace<components::Padding>(header);
        headerPadding.values = {0.0F, kTabHeaderPaddingRight, 0.0F, kTabHeaderPaddingLeft};

        auto& headerSize = reg.get<components::Size>(header);
        headerSize.sizePolicy = policies::Size::AUTO;
        headerSize.minSize = {scale::Metric(48.0F), scale::Metric(kTabHeaderHeight)};

        auto& clickable = reg.emplace<components::Clickable>(header);
        Registry* const regPtr = &reg;
        clickable.onClick = [regPtr, entity, index]()
        {
            SelectTab(*regPtr, detail::ToInternal(entity), index);
        };

        tabView.tabHeaders.push_back(detail::ToInternal(header));
        hierarchy::AddChild(tabBar, header);

        // 内容面板
        const auto panel = CreateBaseWidget(std::string(alias) + "_panel_" + std::to_string(index));
        reg.emplace<components::LayoutInfo>(panel).direction = policies::LayoutDirection::VERTICAL;
        reg.get<components::LayoutInfo>(panel).alignment = policies::Alignment::TOP_LEFT;
        reg.get<components::Size>(panel).sizePolicy = policies::Size::FILL_PARENT;
        if (index != 0)
        {
            reg.remove<components::VisibleTag>(panel);
        }
        tabView.contentPanels.push_back(detail::ToInternal(panel));
        hierarchy::AddChild(entity, panel);
    }

    hierarchy::AddChild(entity, tabBar);

    // 初始化 Tab 头选中态（index=0 选中）
    if (!tabView.tabHeaders.empty())
    {
        ApplyTabVisualState(reg, tabView.tabHeaders.front(), true);
    }

    return entity;
}

ui::entity GetTabContent(ui::entity tabViewEntity, int index)
{
    auto& reg = CurrentRegistry();
    auto* tabView = reg.try_get<components::TabView>(tabViewEntity);
    if (tabView == nullptr || index < 0 || index >= static_cast<int>(tabView->contentPanels.size()))
        return ui::null_entity;
    return detail::ToPublic(tabView->contentPanels[static_cast<std::size_t>(index)]);
}

// ============================================================================
// ListView（列表视图：单选/多选 + 滚动，复用半成品 ListArea 组件）
// ============================================================================
namespace
{

constexpr float kListItemPaddingLeft = 10.0F;
constexpr float kListItemPaddingRight = 8.0F;
constexpr float kListViewMinWidth = 140.0F;
constexpr float kListViewMinHeight = 60.0F;
constexpr Color kListItemTextColor{0.90F, 0.91F, 0.94F, 1.0F};

/// 单选更新：统一改写所有 item 选中态（回调期间不持有 view 迭代器）。
void SelectSingleItem(Registry& reg, entt::entity listViewEntity, entt::entity newSelected)
{
    auto* listArea = reg.try_get<components::ListArea>(listViewEntity);
    if (listArea == nullptr)
        return;

    const int newIndex = [&]
    {
        if (newSelected == entt::null)
            return -1;
        auto* itemComp = reg.try_get<components::ListAreaItem>(newSelected);
        return itemComp != nullptr ? itemComp->itemIndex : -1;
    }();

    // 点已选中项：no-op（防重复回调）
    if (newIndex != -1 && newIndex == listArea->selectedIndex)
        return;

    listArea->selectedIndex = newIndex;
    for (const entt::entity item : listArea->items)
    {
        if (!reg.valid(item))
            continue;
        auto* itemComp = reg.try_get<components::ListAreaItem>(item);
        const bool isSelected = (itemComp != nullptr && itemComp->itemIndex == newIndex);
        if (auto* background = reg.try_get<components::Background>(item); background != nullptr)
        {
            if (isSelected)
            {
                background->color = listArea->selectedBackground;
                background->enabled = policies::Feature::ENABLED;
            }
            else
            {
                background->enabled = policies::Feature::DISABLED;
            }
            ui::utils::MarkVisualChanged(detail::ToPublic(item));
        }
    }

    if (listArea->onChanged)
    {
        listArea->onChanged(listArea->selectedIndex);
    }
}

/// 多选切换：维护 selectedIndices 集合并触发回调。
void ToggleMultiSelectItem(Registry& reg, entt::entity listViewEntity, entt::entity item)
{
    auto* listArea = reg.try_get<components::ListArea>(listViewEntity);
    if (listArea == nullptr)
        return;
    auto* itemComp = reg.try_get<components::ListAreaItem>(item);
    if (itemComp == nullptr)
        return;

    const int index = itemComp->itemIndex;
    const auto iter = std::ranges::find(listArea->selectedIndices, index);
    if (iter != listArea->selectedIndices.end())
    {
        listArea->selectedIndices.erase(iter);
        if (auto* background = reg.try_get<components::Background>(item); background != nullptr)
        {
            background->enabled = policies::Feature::DISABLED;
            ui::utils::MarkVisualChanged(detail::ToPublic(item));
        }
    }
    else
    {
        listArea->selectedIndices.push_back(index);
        if (auto* background = reg.try_get<components::Background>(item); background != nullptr)
        {
            background->color = listArea->selectedBackground;
            background->enabled = policies::Feature::ENABLED;
            ui::utils::MarkVisualChanged(detail::ToPublic(item));
        }
    }

    if (listArea->onMultiChanged)
    {
        listArea->onMultiChanged(listArea->selectedIndices);
    }
}

}  // namespace

ui::entity CreateListView(const std::vector<std::string>& items, int selectedIndex, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ListAreaTag>(entity);
    auto& listArea = reg.emplace<components::ListArea>(entity);
    // 注意：texts 与 items 由 AddListItem 单点维护（push_back 同步），此处不再预填，避免双写。

    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    layout.alignment = policies::Alignment::TOP_LEFT;

    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(kListViewMinWidth), scale::Metric(kListViewMinHeight)};
    size.sizePolicy = policies::Size::FIXED;

    // 滚动容器：item 全部挂到其下，LayoutSystem 自动回写 contentSize（滚动免费）
    const auto scrollArea = CreateScrollArea(std::string(alias) + "_scroll");
    hierarchy::AddChild(entity, scrollArea);

    // 初始选中
    const int initialIndex = std::clamp(selectedIndex, -1, static_cast<int>(items.size()) - 1);

    const int itemCount = static_cast<int>(items.size());
    for (int index = 0; index < itemCount; ++index)
    {
        const ui::entity itemEntity = AddListItem(entity, items[static_cast<std::size_t>(index)],
                                                 std::string(alias) + "_item_" + std::to_string(index));
        if (index == initialIndex)
        {
            auto* itemComp = reg.try_get<components::ListAreaItem>(itemEntity);
            if (itemComp != nullptr)
            {
                listArea.selectedIndex = index;
            }
        }
    }

    // 初态视觉：选中项高亮
    for (const entt::entity item : listArea.items)
    {
        if (!reg.valid(item))
            continue;
        auto* itemComp = reg.try_get<components::ListAreaItem>(item);
        if (itemComp != nullptr && itemComp->itemIndex == listArea.selectedIndex)
        {
            if (auto* background = reg.try_get<components::Background>(item); background != nullptr)
            {
                background->color = listArea.selectedBackground;
                background->enabled = policies::Feature::ENABLED;
            }
        }
    }

    ui::utils::MarkLayoutAndVisualChanged(entity);
    return entity;
}

ui::entity AddListItem(ui::entity listViewEntity, const std::string& text, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto* listArea = reg.try_get<components::ListArea>(listViewEntity);
    if (listArea == nullptr)
    {
        return ui::null_entity;
    }

    const auto item = CreateBaseWidget(alias);
    reg.emplace<components::ListAreaItemTag>(item);
    reg.emplace<components::ListAreaItem>(item, detail::ToInternal(listViewEntity),
                                          static_cast<int>(listArea->items.size()));

    auto& itemText = reg.emplace<components::Text>(item);
    itemText.content = text;
    itemText.fontSize = scale::Metric(13.0F);
    itemText.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
    itemText.color = kListItemTextColor;

    auto& itemSize = reg.get<components::Size>(item);
    itemSize.sizePolicy = policies::Size::FILL_PARENT;
    itemSize.minSize = {scale::Metric(0.0F), scale::Metric(listArea->itemHeight)};

    auto& itemPadding = reg.get_or_emplace<components::Padding>(item);
    itemPadding.values = {0.0F, kListItemPaddingRight, 0.0F, kListItemPaddingLeft};

    auto& background = reg.emplace<components::Background>(item);
    background.enabled = policies::Feature::DISABLED;  // 未选中无背景

    reg.emplace<components::Hoverable>(item);

    Registry* const regPtr = &reg;
    const entt::entity itemInternal = detail::ToInternal(item);

    // 悬停高亮
    reg.get<components::Hoverable>(item).onHover = [regPtr, itemInternal]()
    {
        auto& reg = *regPtr;
        auto* background = reg.try_get<components::Background>(itemInternal);
        auto* itemComp = reg.try_get<components::ListAreaItem>(itemInternal);
        if (background == nullptr || itemComp == nullptr)
            return;
        auto* listArea = reg.try_get<components::ListArea>(itemComp->owner);
        if (listArea == nullptr)
            return;
        const bool isSelected = (listArea->selectedIndex == itemComp->itemIndex) ||
                                std::ranges::find(listArea->selectedIndices, itemComp->itemIndex) !=
                                    listArea->selectedIndices.end();
        if (!isSelected)
        {
            background->color = listArea->hoverBackground;
            background->enabled = policies::Feature::ENABLED;
            ui::utils::MarkVisualChanged(detail::ToPublic(itemInternal));
        }
    };
    reg.get<components::Hoverable>(item).onUnhover = [regPtr, itemInternal]()
    {
        auto& reg = *regPtr;
        auto* background = reg.try_get<components::Background>(itemInternal);
        auto* itemComp = reg.try_get<components::ListAreaItem>(itemInternal);
        if (background == nullptr || itemComp == nullptr)
            return;
        auto* listArea = reg.try_get<components::ListArea>(itemComp->owner);
        if (listArea == nullptr)
            return;
        const bool isSelected = (listArea->selectedIndex == itemComp->itemIndex) ||
                                std::ranges::find(listArea->selectedIndices, itemComp->itemIndex) !=
                                    listArea->selectedIndices.end();
        if (isSelected)
        {
            background->color = listArea->selectedBackground;
            background->enabled = policies::Feature::ENABLED;
        }
        else
        {
            background->enabled = policies::Feature::DISABLED;
        }
        ui::utils::MarkVisualChanged(detail::ToPublic(itemInternal));
    };

    // 点击选择
    reg.get_or_emplace<components::Clickable>(item).onClick = [regPtr, listViewEntity, itemInternal]()
    {
        auto& reg = *regPtr;
        auto* listArea = reg.try_get<components::ListArea>(listViewEntity);
        if (listArea == nullptr)
            return;
        if (listArea->multiSelect == policies::Selection::MULTI)
        {
            ToggleMultiSelectItem(reg, detail::ToInternal(listViewEntity), itemInternal);
        }
        else
        {
            SelectSingleItem(reg, detail::ToInternal(listViewEntity), itemInternal);
        }
    };

    listArea->items.push_back(itemInternal);
    listArea->texts.push_back(text);

    // 挂到滚动容器（ScrollArea 是 ListView 的第一个子节点）
    const auto* scrollHier = reg.try_get<components::Hierarchy>(detail::ToInternal(listViewEntity));
    if (scrollHier != nullptr && !scrollHier->children.empty())
    {
        hierarchy::AddChild(detail::ToPublic(scrollHier->children.front()), item);
    }
    else
    {
        hierarchy::AddChild(listViewEntity, item);
    }

    ui::utils::MarkLayoutAndVisualChanged(listViewEntity);
    return item;
}

namespace
{

entt::entity FindWindowRoot(Registry& reg, entt::entity entity)
{
    entt::entity current = entity;
    while (current != entt::null && reg.valid(current))
    {
        if (reg.any_of<components::WindowTag>(current))
        {
            return current;
        }
        const auto* hier = reg.try_get<components::Hierarchy>(current);
        current = (hier != nullptr) ? hier->parent : entt::null;
    }
    return entt::null;
}

}  // namespace

void CloseDropDownPopup(ui::entity ddEntity)
{
    auto& runtime = UiRuntime::current();
    auto& reg = CurrentRegistry();
    auto* dropDown = reg.try_get<components::DropDown>(ddEntity);
    if (dropDown == nullptr)
        return;
    if (dropDown->popupEntity == entt::null || !reg.valid(dropDown->popupEntity))
    {
        dropDown->open = false;
        return;
    }

    const entt::entity popupEntityInternal = dropDown->popupEntity;
    const ui::entity popupToDestroy = detail::ToPublic(popupEntityInternal);

    // 先置空 popupEntity 打断重入：OverlayCloseRequest → StateSystem 桥接 → DropDownCloseRequested
    // → 本函数，重入时 popupEntity 已为 null 会提前返回，避免无限递归。
    dropDown->popupEntity = entt::null;
    dropDown->open = false;
    ui::utils::MarkVisualChanged(ddEntity);

    // 通知 OverlaySystem 出栈并恢复焦点（幂等：已出栈时静默忽略）
    runtime.dispatcher().trigger<events::OverlayCloseRequest>(events::OverlayCloseRequest{popupEntityInternal});

    auto timerSystem = systems::TimerSystem{runtime};
    timerSystem.addTask(
        0,
        [regPtr = &reg, popupToDestroy]()
        {
            auto& reg = *regPtr;
            if (!reg.valid(popupToDestroy))
                return;

            const auto* popupHier = reg.try_get<components::Hierarchy>(popupToDestroy);
            if (popupHier != nullptr && popupHier->parent != entt::null)
            {
                hierarchy::RemoveChild(detail::ToPublic(popupHier->parent), popupToDestroy);
            }

            std::vector<ui::entity> toDestroy;
            std::vector<ui::entity> stack{popupToDestroy};
            while (!stack.empty())
            {
                const ui::entity cur = stack.back();
                stack.pop_back();
                if (!reg.valid(cur))
                    continue;
                toDestroy.push_back(cur);
                if (const auto* hier = reg.try_get<components::Hierarchy>(cur))
                {
                    for (const entt::entity child : hier->children)
                    {
                        stack.push_back(detail::ToPublic(child));
                    }
                }
            }
            for (ui::entity ent : std::ranges::reverse_view(toDestroy))
            {
                if (reg.valid(ent))
                {
                    reg.destroy(ent);
                }
            }
        },
        true);
}

void CloseDropDownPopup(entt::entity ddEntity)
{
    CloseDropDownPopup(detail::ToPublic(ddEntity));
}

namespace
{

/// Tooltip 浮层相对目标的垂直偏移（像素）。
constexpr float kTooltipOffsetY = 6.0F;
constexpr float kTooltipPaddingTop = 6.0F;
constexpr float kTooltipPaddingRight = 8.0F;
constexpr float kTooltipPaddingBottom = 6.0F;
constexpr float kTooltipPaddingLeft = 8.0F;
constexpr float kTooltipFontSize = 12.0F;
constexpr float kTooltipRadius = 4.0F;

void ShowTooltipPopup(ui::entity target)
{
    auto& reg = CurrentRegistry();
    auto& runtime = UiRuntime::current();
    auto* tooltip = reg.try_get<components::Tooltip>(target);
    if (tooltip == nullptr || !tooltip->hovered || tooltip->text.empty())
        return;
    if (tooltip->popupEntity != entt::null && reg.valid(tooltip->popupEntity))
        return;

    const Rect targetRect = ui::utils::GetEntityRect(target);
    const entt::entity windowRoot = FindWindowRoot(reg, detail::ToInternal(target));
    if (windowRoot == entt::null)
        return;

    const auto popup = CreateBaseWidget("__tooltip__");
    auto& popupPos = reg.get<components::Position>(popup);
    popupPos.value = {targetRect.x(), targetRect.y() + targetRect.height() + kTooltipOffsetY};
    popupPos.positionPolicy = policies::Position::ABSOLUTE_POS;

    auto& popupSize = reg.get<components::Size>(popup);
    popupSize.sizePolicy = policies::Size::AUTO;

    auto& textComp = reg.emplace<components::Text>(popup);
    textComp.content = tooltip->text;
    textComp.fontSize = scale::Metric(kTooltipFontSize);

    auto& background = reg.emplace<components::Background>(popup);
    background.color = Color{0.10F, 0.10F, 0.12F, 0.96F};
    background.borderRadius = Vec4{kTooltipRadius, kTooltipRadius, kTooltipRadius, kTooltipRadius};
    background.enabled = policies::Feature::ENABLED;

    auto& padding = reg.get_or_emplace<components::Padding>(popup);
    padding.values = {kTooltipPaddingTop, kTooltipPaddingRight, kTooltipPaddingBottom, kTooltipPaddingLeft};

    hierarchy::AddChild(detail::ToPublic(windowRoot), popup);
    tooltip->popupEntity = detail::ToInternal(popup);

    runtime.dispatcher().trigger<events::OverlayOpenRequest>(
        events::OverlayOpenRequest{detail::ToInternal(popup), detail::ToInternal(target)});

    ui::utils::MarkLayoutAndVisualChanged(detail::ToPublic(windowRoot));
}

void HideTooltipPopup(ui::entity target)
{
    auto& reg = CurrentRegistry();
    auto& runtime = UiRuntime::current();
    auto* tooltip = reg.try_get<components::Tooltip>(target);
    if (tooltip == nullptr)
        return;

    if (tooltip->pendingTask != 0)
    {
        systems::TimerSystem{runtime}.cancelTask(tooltip->pendingTask);
        tooltip->pendingTask = 0;
    }

    if (tooltip->popupEntity != entt::null && reg.valid(tooltip->popupEntity))
    {
        const entt::entity popup = tooltip->popupEntity;
        tooltip->popupEntity = entt::null;

        runtime.dispatcher().trigger<events::OverlayCloseRequest>(events::OverlayCloseRequest{popup});

        const auto* popupHierarchy = reg.try_get<components::Hierarchy>(popup);
        if (popupHierarchy != nullptr && popupHierarchy->parent != entt::null)
        {
            hierarchy::RemoveChild(detail::ToPublic(popupHierarchy->parent), detail::ToPublic(popup));
        }
        reg.destroy(popup);
    }
}

}  // namespace

ui::entity SetTooltip(ui::entity target, const std::string& text, int delayMs)
{
    auto& reg = CurrentRegistry();
    auto& tooltip = reg.get_or_emplace<components::Tooltip>(target);
    tooltip.text = text;
    tooltip.delayMs = delayMs;
    reg.emplace_or_replace<components::TooltipTag>(target);

    auto& hoverable = reg.get_or_emplace<components::Hoverable>(target);
    Registry* const regPtr = &reg;
    hoverable.onHover = [regPtr, target]()
    {
        auto& reg = *regPtr;
        auto* tooltip = reg.try_get<components::Tooltip>(target);
        if (tooltip == nullptr || tooltip->text.empty())
            return;
        if (tooltip->popupEntity != entt::null && reg.valid(tooltip->popupEntity))
            return;

        tooltip->hovered = true;
        auto& runtime = UiRuntime::current();
        tooltip->pendingTask = systems::TimerSystem{runtime}.addTask(
            static_cast<uint32_t>(tooltip->delayMs), [target]() { ShowTooltipPopup(target); }, true);
    };
    hoverable.onUnhover = [regPtr, target]()
    {
        auto& reg = *regPtr;
        if (auto* tooltip = reg.try_get<components::Tooltip>(target); tooltip != nullptr)
        {
            tooltip->hovered = false;
        }
        HideTooltipPopup(target);
    };

    return target;
}

namespace
{
void OpenDropDownPopup(ui::entity ddEntity)
{
    auto& reg = CurrentRegistry();
    auto& disp = UiRuntime::current().dispatcher();
    auto* dropDown = reg.try_get<components::DropDown>(ddEntity);
    if (dropDown == nullptr || dropDown->options.empty())
        return;

    const entt::entity windowRoot = FindWindowRoot(reg, detail::ToInternal(ddEntity));
    if (windowRoot == entt::null)
        return;
    const ui::entity publicWindowRoot = detail::ToPublic(windowRoot);
    // 计算下拉菜单弹出位置和大小
    const Rect ddRect = ui::utils::GetEntityRect(ddEntity);

    constexpr float ITEM_H = 26.0F;
    constexpr float ITEM_PAD = 6.0F;
    const float popupW = ddRect.width();
    const float popupH = ITEM_H * static_cast<float>(dropDown->options.size());

    const auto popup = CreateBaseWidget("__dd_popup__");
    auto& popupPos = reg.get<components::Position>(popup);
    popupPos.value = {ddRect.x(), ddRect.y() + ddRect.height()};
    popupPos.positionPolicy = policies::Position::ABSOLUTE_POS;

    auto& popupSize = reg.get<components::Size>(popup);
    popupSize.sizePolicy = policies::Size::FIXED;
    popupSize.size = {scale::Metric(popupW), scale::Metric(popupH)};

    reg.emplace<components::DropDownPopupPanel>(popup).owner = detail::ToInternal(ddEntity);

    // 统一浮层栈：由 OverlaySystem 分配 z-order 并压栈（替代硬编码 ZOrderIndex=1000）
    disp.trigger<events::OverlayOpenRequest>(
        events::OverlayOpenRequest{detail::ToInternal(popup), detail::ToInternal(ddEntity)});

    auto& popupLayout = reg.emplace<components::LayoutInfo>(popup);
    popupLayout.direction = policies::LayoutDirection::VERTICAL;
    popupLayout.alignment = policies::Alignment::TOP_LEFT;

    const int optCount = static_cast<int>(dropDown->options.size());
    for (int idx = 0; idx < optCount; ++idx)
    {
        const std::string& optText = dropDown->options.at(static_cast<std::size_t>(idx));

        const auto optBtn = CreateBaseWidget("__dd_option__");
        reg.emplace<components::Clickable>(optBtn);
        auto& popupItem = reg.emplace<components::DropDownPopupItem>(optBtn);
        popupItem.owner = detail::ToInternal(ddEntity);
        popupItem.optionIndex = idx;

        auto& btnText = reg.emplace<components::Text>(optBtn);
        btnText.content = optText;
        btnText.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;

        auto& btnSize = reg.get<components::Size>(optBtn);
        btnSize.sizePolicy = policies::Size::FIXED;
        btnSize.size = {scale::Metric(popupW), scale::Metric(ITEM_H)};

        auto& btnPad = reg.get_or_emplace<components::Padding>(optBtn);
        btnPad.values = {0.0F, 0.0F, 0.0F, ITEM_PAD};

        reg.emplace<components::Hoverable>(optBtn);

        Registry* const regPtr = &reg;
        reg.get<components::Clickable>(optBtn).onClick = [regPtr, ddEntity, idx]()
        {
            auto& reg = *regPtr;
            auto* ddComp = reg.try_get<components::DropDown>(ddEntity);
            if (ddComp == nullptr)
                return;
            ddComp->selectedIndex = idx;
            if (auto* textComp = reg.try_get<components::Text>(ddEntity))
            {
                textComp->content = ddComp->selectedText();
            }
            if (ddComp->onChanged)
            {
                ddComp->onChanged(idx);
            }
            ui::utils::MarkVisualChanged(ddEntity);
            CloseDropDownPopup(ddEntity);
        };
        hierarchy::AddChild(popup, optBtn);
    }

    hierarchy::AddChild(publicWindowRoot, popup);
    dropDown->popupEntity = detail::ToInternal(popup);
    dropDown->open = true;
    ui::utils::MarkLayoutAndVisualChanged(publicWindowRoot);
}

}  // namespace

ui::entity CreateDropDown(const std::vector<std::string>& options, int selectedIndex, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::DropDownTag>(entity);
    reg.emplace<components::FocusableTag>(entity);
    auto& dropDown = reg.emplace<components::DropDown>(entity);
    dropDown.options = options;
    dropDown.selectedIndex = selectedIndex;
    auto& text = reg.emplace<components::Text>(entity);
    text.content = dropDown.selectedText();
    text.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
    auto& padding = reg.get_or_emplace<components::Padding>(entity);
    padding.values = {0.0F, scale::Metric(20.0F), 0.0F, scale::Metric(6.0F)};
    auto& clickable = reg.emplace<components::Clickable>(entity);
    clickable.onClick = [entity]()
    {
        auto* ddComp = CurrentRegistry().try_get<components::DropDown>(entity);
        if (ddComp == nullptr)
            return;
        if (ddComp->open)
        {
            CloseDropDownPopup(entity);
        }
        else
        {
            OpenDropDownPopup(entity);
        }
    };
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::AUTO;
    size.size = {scale::Metric(140.0F), scale::Metric(26.0F)};
    return entity;
}

ui::entity CreateSlider(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::SliderInfo>(entity);
    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(200.0F), scale::Metric(28.0F)};
    size.sizePolicy = policies::Size::FIXED;
    reg.emplace<components::LayoutInfo>(entity);
    utils::MarkLayoutAndVisualChanged(entity);
    reg.emplace<components::FocusableTag>(entity);
    reg.emplace<components::SliderTag>(entity);
    return entity;
}

ui::entity CreateProgressBar(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ProgressBar>(entity);
    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(200.0F), scale::Metric(14.0F)};
    size.sizePolicy = policies::Size::FIXED;
    reg.emplace<components::LayoutInfo>(entity);
    utils::MarkLayoutAndVisualChanged(entity);
    reg.emplace<components::ProgressBarTag>(entity);
    return entity;
}

ui::entity CreateImageFromPath(std::string_view path, float defaultWidth, float defaultHeight, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ImageTag>(entity);
    reg.emplace<components::Image>(entity);
    reg.emplace<components::ImageSource>(entity, std::string(path));
    auto& size = reg.get<components::Size>(entity);
    if (defaultWidth > 0.0F || defaultHeight > 0.0F)
    {
        size.size = {scale::Metric(defaultWidth), scale::Metric(defaultHeight)};
        size.sizePolicy = policies::Size::FIXED;
    }
    reg.emplace<components::LayoutInfo>(entity);
    utils::MarkLayoutAndVisualChanged(entity);
    return entity;
}

ui::entity CreateCanvas(float width, float height, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::CanvasTag>(entity);
    reg.emplace<components::CanvasDrawList>(entity);
    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(width), scale::Metric(height)};
    size.sizePolicy = policies::Size::FIXED;
    reg.emplace<components::LayoutInfo>(entity);
    utils::MarkLayoutAndVisualChanged(entity);
    return entity;
}

ui::entity CreateTable(int columns, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::TableTag>(entity);
    auto& info = reg.emplace<components::TableInfo>(entity);
    info.columnCount = columns;
    info.rowHeight = scale::Metric(info.rowHeight);
    info.headerHeight = scale::Metric(info.headerHeight);
    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::FILL_PARENT;

    auto& scrollArea = reg.emplace<components::ScrollArea>(entity);
    scrollArea.scroll = policies::Scroll::VERTICAL;
    scrollArea.scrollBar = policies::ScrollBar::DRAGGABLE | policies::ScrollBar::AUTO_HIDE;

    auto& padding = reg.emplace<components::Padding>(entity);
    padding.values.x() = info.headerHeight;

    reg.emplace<components::LayoutInfo>(entity);
    utils::MarkLayoutAndVisualChanged(entity);
    return entity;
}

// ============================================================================
// ContextMenu（右键菜单，复用 OverlaySystem 统一浮层栈）
// ============================================================================
namespace
{

constexpr float kContextMenuWidth = 160.0F;
constexpr float kContextMenuItemHeight = 26.0F;
constexpr float kContextMenuItemPaddingLeft = 10.0F;
constexpr float kContextMenuPaddingV = 4.0F;
constexpr float kContextMenuPaddingH = 4.0F;
constexpr float kContextMenuRadius = 6.0F;
constexpr Color kContextMenuBackground{0.14F, 0.14F, 0.17F, 0.97F};
constexpr Color kContextMenuItemHover{0.25F, 0.27F, 0.33F, 1.0F};
constexpr Color kContextMenuItemText{0.90F, 0.91F, 0.94F, 1.0F};

}  // namespace

ui::entity CreateContextMenu(std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ContextMenuTag>(entity);
    reg.emplace<components::ContextMenu>(entity);

    auto& layout = reg.emplace<components::LayoutInfo>(entity);
    layout.direction = policies::LayoutDirection::VERTICAL;
    layout.alignment = policies::Alignment::TOP_LEFT;
    layout.spacing = scale::Metric(0.0F);

    auto& size = reg.get<components::Size>(entity);
    size.size = {scale::Metric(kContextMenuWidth), scale::Metric(0.0F)};
    size.sizePolicy = policies::Size::AUTO;

    auto& background = reg.emplace<components::Background>(entity);
    background.color = kContextMenuBackground;
    background.borderRadius = Vec4{kContextMenuRadius, kContextMenuRadius, kContextMenuRadius, kContextMenuRadius};
    background.enabled = policies::Feature::ENABLED;

    auto& padding = reg.get_or_emplace<components::Padding>(entity);
    padding.values = {kContextMenuPaddingV, kContextMenuPaddingH, kContextMenuPaddingV, kContextMenuPaddingH};

    reg.remove<components::VisibleTag>(entity);
    return entity;
}

ui::entity AddContextMenuItem(ui::entity menu, const std::string& text, ui::Callback<> onClick)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget("__context_menu_item__");

    reg.emplace<components::Clickable>(entity);
    reg.emplace<components::Hoverable>(entity);

    auto& itemText = reg.emplace<components::Text>(entity);
    itemText.content = text;
    itemText.fontSize = scale::Metric(13.0F);
    itemText.alignment = policies::Alignment::LEFT | policies::Alignment::VCENTER;
    itemText.color = kContextMenuItemText;

    auto& size = reg.get<components::Size>(entity);
    size.sizePolicy = policies::Size::FILL_PARENT;
    size.minSize = {scale::Metric(0.0F), scale::Metric(kContextMenuItemHeight)};

    auto& padding = reg.get_or_emplace<components::Padding>(entity);
    padding.values = {0.0F, 0.0F, 0.0F, kContextMenuItemPaddingLeft};

    Registry* const regPtr = &reg;
    reg.get<components::Hoverable>(entity).onHover = [regPtr, entity]()
    {
        auto& reg = *regPtr;
        if (auto* bg = reg.try_get<components::Background>(entity); bg != nullptr)
        {
            bg->color = kContextMenuItemHover;
            bg->enabled = policies::Feature::ENABLED;
        }
        ui::utils::MarkVisualChanged(entity);
    };
    reg.get<components::Hoverable>(entity).onUnhover = [regPtr, entity]()
    {
        auto& reg = *regPtr;
        if (auto* bg = reg.try_get<components::Background>(entity); bg != nullptr)
        {
            bg->enabled = policies::Feature::DISABLED;
        }
        ui::utils::MarkVisualChanged(entity);
    };

    reg.get<components::Clickable>(entity).onClick = [regPtr, menu, onClick = std::move(onClick)]() mutable
    {
        auto& reg = *regPtr;
        // 先关闭菜单（即使无回调也收起）
        auto* menuComp = reg.try_get<components::ContextMenu>(menu);
        if (menuComp != nullptr && menuComp->open)
        {
            CloseContextMenu(menu);
        }
        // 执行用户回调
        if (onClick)
        {
            onClick();
        }
    };

    hierarchy::AddChild(menu, entity);
    ui::utils::MarkLayoutAndVisualChanged(menu);
    return entity;
}

void ShowContextMenu(ui::entity menu, const Vec2& position, ui::entity owner)
{
    auto& reg = CurrentRegistry();
    auto& runtime = UiRuntime::current();
    auto* menuComp = reg.try_get<components::ContextMenu>(menu);
    if (menuComp == nullptr)
        return;

    // 已打开则先关闭
    if (menuComp->open)
    {
        CloseContextMenu(menu);
    }

    // 定位所属窗口：优先用 owner（触发者）向上找窗口根；菜单尚未挂到窗口时自动挂载
    const entt::entity ownerInternal = detail::ToInternal(owner);
    const entt::entity anchor = ownerInternal != entt::null ? ownerInternal : detail::ToInternal(menu);
    const entt::entity windowRoot = FindWindowRoot(reg, anchor);
    if (windowRoot == entt::null)
        return;

    // 菜单未挂到窗口树时，挂载到窗口根（保持 RootTag 语义：浮层子节点可复用）
    const auto* menuHier = reg.try_get<components::Hierarchy>(menu);
    if (menuHier == nullptr || menuHier->parent == entt::null)
    {
        hierarchy::AddChild(detail::ToPublic(windowRoot), menu);
    }

    // 定位并显示
    auto& pos = reg.get<components::Position>(menu);
    pos.value = {position.x(), position.y()};
    pos.positionPolicy = policies::Position::ABSOLUTE_POS;
    reg.emplace_or_replace<components::VisibleTag>(menu);
    ui::utils::MarkLayoutAndVisualChanged(detail::ToPublic(windowRoot));

    // 入浮层栈
    runtime.dispatcher().trigger<events::OverlayOpenRequest>(
        events::OverlayOpenRequest{detail::ToInternal(menu), detail::ToInternal(owner)});

    menuComp->open = true;
    menuComp->owner = detail::ToInternal(owner);
}

void CloseContextMenu(ui::entity menu)
{
    auto& reg = CurrentRegistry();
    auto* menuComp = reg.try_get<components::ContextMenu>(menu);
    if (menuComp == nullptr)
        return;
    if (!menuComp->open)
        return;

    menuComp->open = false;
    menuComp->owner = entt::null;
    reg.remove<components::VisibleTag>(menu);
    ui::utils::MarkVisualChanged(menu);

    UiRuntime::current().dispatcher().trigger<events::OverlayCloseRequest>(
        events::OverlayCloseRequest{detail::ToInternal(menu)});
}

// ============================================================================
// ModalDialog（模态浮层：遮罩 + 居中内容容器，复用 OverlaySystem）
// ============================================================================
namespace
{

constexpr Color kModalMaskColor{0.0F, 0.0F, 0.0F, 0.45F};
constexpr float kModalDialogWidth = 320.0F;
constexpr float kModalDialogMinHeight = 160.0F;
constexpr float kModalDialogRadius = 10.0F;
constexpr float kModalDialogPadding = 16.0F;
constexpr Color kModalDialogBackground{0.13F, 0.13F, 0.16F, 1.0F};

/// 遮罩覆盖到父窗口尺寸；内容容器居中。
void LayoutModalOverlay(Registry& reg, entt::entity overlayRoot, entt::entity parentWindow)
{
    const Rect windowRect = ui::utils::GetEntityRect(detail::ToPublic(parentWindow));
    auto& maskSize = reg.get<components::Size>(overlayRoot);
    maskSize.size = {scale::Metric(windowRect.width()), scale::Metric(windowRect.height())};
    maskSize.sizePolicy = policies::Size::FIXED;
    ui::utils::MarkLayoutAndVisualChanged(detail::ToPublic(parentWindow));
}

}  // namespace

ui::entity CreateModalDialog(ui::entity parentWindow, std::string_view alias)
{
    auto& reg = CurrentRegistry();
    auto entity = CreateBaseWidget(alias);
    reg.emplace<components::ModalDialogTag>(entity);
    auto& dialog = reg.emplace<components::ModalDialog>(entity);

    // 遮罩容器：半透明黑，覆盖父窗口客户区，点击遮罩关闭
    const auto overlayRoot = CreateBaseWidget("__modal_mask__");
    reg.emplace<components::Clickable>(overlayRoot);
    auto& maskBackground = reg.emplace<components::Background>(overlayRoot);
    maskBackground.color = kModalMaskColor;
    maskBackground.enabled = policies::Feature::ENABLED;

    auto& maskPos = reg.get<components::Position>(overlayRoot);
    maskPos.value = {0.0F, 0.0F};
    maskPos.positionPolicy = policies::Position::ABSOLUTE_POS;

    // 遮罩拦截点击 → 关闭对话框
    Registry* const regPtr = &reg;
    reg.get<components::Clickable>(overlayRoot).onClick = [regPtr, entity]()
    {
        auto& reg = *regPtr;
        auto* dialogComp = reg.try_get<components::ModalDialog>(entity);
        if (dialogComp != nullptr && dialogComp->open)
        {
            CloseModalDialog(entity);
        }
    };

    // 内容容器：居中、圆角背景
    const auto contentRoot = CreateBaseWidget("__modal_content__");
    auto& contentLayout = reg.emplace<components::LayoutInfo>(contentRoot);
    contentLayout.direction = policies::LayoutDirection::VERTICAL;
    contentLayout.alignment = policies::Alignment::TOP_LEFT;
    contentLayout.spacing = scale::Metric(8.0F);

    auto& contentSize = reg.get<components::Size>(contentRoot);
    contentSize.size = {scale::Metric(kModalDialogWidth), scale::Metric(kModalDialogMinHeight)};
    contentSize.sizePolicy = policies::Size::FIXED;

    auto& contentBackground = reg.emplace<components::Background>(contentRoot);
    contentBackground.color = kModalDialogBackground;
    contentBackground.borderRadius =
        Vec4{kModalDialogRadius, kModalDialogRadius, kModalDialogRadius, kModalDialogRadius};
    contentBackground.enabled = policies::Feature::ENABLED;

    auto& contentPadding = reg.get_or_emplace<components::Padding>(contentRoot);
    contentPadding.values = {kModalDialogPadding, kModalDialogPadding, kModalDialogPadding, kModalDialogPadding};

    hierarchy::AddChild(overlayRoot, contentRoot);
    hierarchy::AddChild(entity, overlayRoot);
    dialog.popupEntity = detail::ToInternal(overlayRoot);
    reg.remove<components::VisibleTag>(entity);

    // 记录父窗口（供 Show 时定位遮罩）
    if (auto* hierarchyComp = reg.try_get<components::Hierarchy>(entity); hierarchyComp != nullptr)
    {
        hierarchyComp->parent = detail::ToInternal(parentWindow);
    }
    return entity;
}

void ShowModalDialog(ui::entity dialog)
{
    auto& reg = CurrentRegistry();
    auto& runtime = UiRuntime::current();
    auto* dialogComp = reg.try_get<components::ModalDialog>(dialog);
    if (dialogComp == nullptr)
        return;
    if (dialogComp->open)
        return;

    const auto* hierarchyComp = reg.try_get<components::Hierarchy>(dialog);
    if (hierarchyComp == nullptr || hierarchyComp->parent == entt::null)
        return;
    const entt::entity windowRoot = FindWindowRoot(reg, hierarchyComp->parent);
    if (windowRoot == entt::null)
        return;

    // 遮罩覆盖父窗口
    const entt::entity overlayRoot = dialogComp->popupEntity;
    if (overlayRoot == entt::null || !reg.valid(overlayRoot))
        return;
    LayoutModalOverlay(reg, overlayRoot, hierarchyComp->parent);

    reg.emplace_or_replace<components::VisibleTag>(dialog);
    ui::utils::MarkLayoutAndVisualChanged(detail::ToPublic(windowRoot));

    runtime.dispatcher().trigger<events::OverlayOpenRequest>(
        events::OverlayOpenRequest{detail::ToInternal(dialog), hierarchyComp->parent});

    dialogComp->open = true;
}

void CloseModalDialog(ui::entity dialog)
{
    auto& reg = CurrentRegistry();
    auto& runtime = UiRuntime::current();
    auto* dialogComp = reg.try_get<components::ModalDialog>(dialog);
    if (dialogComp == nullptr)
        return;
    if (!dialogComp->open)
        return;

    dialogComp->open = false;
    reg.remove<components::VisibleTag>(dialog);
    ui::utils::MarkVisualChanged(dialog);

    runtime.dispatcher().trigger<events::OverlayCloseRequest>(
        events::OverlayCloseRequest{detail::ToInternal(dialog)});
}

}  // namespace ui::factory
