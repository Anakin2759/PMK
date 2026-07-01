/**
 * ************************************************************************
 *
 * @file WindowSync.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-30
 * @version 0.2
 * @brief 处理窗口相关组件与 SDL_Window 之间的同步逻辑
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>

#include "PlatformWindow.hpp"
#include "UiRuntime.hpp"
#include "common/AppConfig.hpp"
#include "common/Policies.hpp"
#include "common/Tags.hpp"
#include "common/Types.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Visual.hpp"
#include "common/components/Window.hpp"
#include "utils/Logger.hpp"

namespace ui::window_sync
{

inline bool SyncWindowDisplayMetrics(components::Window& windowComp, SDL_Window* sdlWindow)
{
    if (sdlWindow == nullptr) return false;

    int logicalWidth = 0;
    int logicalHeight = 0;
    SDL_GetWindowSize(sdlWindow, &logicalWidth, &logicalHeight);

    int pixelWidth = 0;
    int pixelHeight = 0;
    SDL_GetWindowSizeInPixels(sdlWindow, &pixelWidth, &pixelHeight);

    if (logicalWidth <= 0 || logicalHeight <= 0)
    {
        logicalWidth = pixelWidth > 0 ? pixelWidth : 1;
        logicalHeight = pixelHeight > 0 ? pixelHeight : 1;
    }
    if (pixelWidth <= 0 || pixelHeight <= 0)
    {
        pixelWidth = logicalWidth;
        pixelHeight = logicalHeight;
    }

    float displayScale = 1.0F;
    float uiScale = 1.0F;
    const auto& appConfig = config::AppConfig::instance();
    if (appConfig.platformScalingEnabled())
    {
        displayScale = platform::GetWindowFramebufferScale(sdlWindow);
        uiScale = appConfig.forcedPlatformScale() > 0.0F ? appConfig.forcedPlatformScale()
                                                         : platform::GetWindowUiScale(sdlWindow);
    }
    if (!std::isfinite(displayScale) || displayScale <= 0.0F)
    {
        displayScale = 1.0F;
    }
    if (!std::isfinite(uiScale) || uiScale <= 0.0F)
    {
        uiScale = 1.0F;
    }

    constexpr float METRIC_EPSILON = 0.01F;
    const bool changed = std::abs(windowComp.displayScale - displayScale) > METRIC_EPSILON
                      || std::abs(windowComp.uiScale - uiScale) > METRIC_EPSILON
                      || std::abs(windowComp.logicalSize.x() - static_cast<float>(logicalWidth)) > METRIC_EPSILON
                      || std::abs(windowComp.logicalSize.y() - static_cast<float>(logicalHeight)) > METRIC_EPSILON
                      || std::abs(windowComp.pixelSize.x() - static_cast<float>(pixelWidth)) > METRIC_EPSILON
                      || std::abs(windowComp.pixelSize.y() - static_cast<float>(pixelHeight)) > METRIC_EPSILON;

    windowComp.displayScale = displayScale;
    windowComp.uiScale = uiScale;
    windowComp.logicalSize = Vec2{static_cast<float>(logicalWidth), static_cast<float>(logicalHeight)};
    windowComp.pixelSize = Vec2{static_cast<float>(pixelWidth), static_cast<float>(pixelHeight)};

    if (changed && appConfig.debugScaling())
    {
        const float derivedScaleX = logicalWidth > 0 ? static_cast<float>(pixelWidth) / static_cast<float>(logicalWidth) : 0.0F;
        const float derivedScaleY =
            logicalHeight > 0 ? static_cast<float>(pixelHeight) / static_cast<float>(logicalHeight) : 0.0F;
        ui::UiRuntime::current().logger().info("[Scaling][WindowSync] windowId={} logical=({}, {}) pixel=({}, {}) displayScale={:.3f} "
                     "uiScale={:.3f} derivedScale=({:.3f}, {:.3f})",
                     windowComp.windowID,
                     logicalWidth,
                     logicalHeight,
                     pixelWidth,
                     pixelHeight,
                     displayScale,
                     uiScale,
                     derivedScaleX,
                     derivedScaleY);
    }

    return changed;
}

inline void SyncWindowTitle(entt::entity entity, const components::Window& windowComp, SDL_Window* sdlWindow)
{
    std::string newTitle;

    const auto& registry = ui::UiRuntime::current().registry();
    const auto* titleComp = registry.try_get<components::Title>(entity);
    if (titleComp != nullptr && !titleComp->text.empty())
    {
        newTitle = titleComp->text;
    }
    else if (!windowComp.title.empty())
    {
        newTitle = windowComp.title;
    }

    if (!newTitle.empty())
    {
        const char* currentTitle = SDL_GetWindowTitle(sdlWindow);
        if (currentTitle == nullptr || newTitle != currentTitle)
        {
            SDL_SetWindowTitle(sdlWindow, newTitle.c_str());
        }
    }
}

inline void SyncWindowPosition(entt::entity entity, SDL_Window* sdlWindow)
{
    auto& registry = ui::UiRuntime::current().registry();
    auto* posComp = registry.try_get<components::Position>(entity);
    if (posComp == nullptr) return;

    int currentX = 0;
    int currentY = 0;
    SDL_GetWindowPosition(sdlWindow, &currentX, &currentY);

    constexpr float POSITION_EPSILON = 0.01F;
    if (std::abs(posComp->value.x()) < POSITION_EPSILON && std::abs(posComp->value.y()) < POSITION_EPSILON)
    {
        posComp->value = Eigen::Vector2f{static_cast<float>(currentX), static_cast<float>(currentY)};
        return;
    }

    const int targetX = static_cast<int>(posComp->value.x());
    const int targetY = static_cast<int>(posComp->value.y());
    if (std::abs(currentX - targetX) > 1 || std::abs(currentY - targetY) > 1)
    {
        SDL_SetWindowPosition(sdlWindow, targetX, targetY);
    }
}

inline bool TryGetWindowSizeTarget(const components::Size& sizeComp, int& width, int& height)
{
    const float widthValue = sizeComp.size.x();
    const float heightValue = sizeComp.size.y();
    if (!std::isfinite(widthValue) || !std::isfinite(heightValue) || widthValue <= 0.0F || heightValue <= 0.0F)
    {
        return false;
    }

    width = static_cast<int>(std::round(widthValue));
    height = static_cast<int>(std::round(heightValue));
    return width > 0 && height > 0;
}

inline void SyncWindowSize(entt::entity entity, SDL_Window* sdlWindow)
{
    auto& registry = ui::UiRuntime::current().registry();
    auto* sizeComp = registry.try_get<components::Size>(entity);
    if (sizeComp == nullptr) return;

    int currentWidth = 0;
    int currentHeight = 0;
    SDL_GetWindowSize(sdlWindow, &currentWidth, &currentHeight);

    int targetWidth = 0;
    int targetHeight = 0;
    if (!TryGetWindowSizeTarget(*sizeComp, targetWidth, targetHeight))
    {
        if (currentWidth > 0 && currentHeight > 0)
        {
            sizeComp->size = Eigen::Vector2f{static_cast<float>(currentWidth), static_cast<float>(currentHeight)};
        }
        return;
    }

    if (currentWidth != targetWidth || currentHeight != targetHeight)
    {
        SDL_SetWindowSize(sdlWindow, targetWidth, targetHeight);
    }
}

inline void SyncWindowSizeConstraints(entt::entity entity, const components::Window& windowComp, SDL_Window* sdlWindow)
{
    const auto& registry = ui::UiRuntime::current().registry();
    const auto* sizeComp = registry.try_get<components::Size>(entity);

    int currentMinW = 0;
    int currentMinH = 0;
    int currentMaxW = 0;
    int currentMaxH = 0;
    SDL_GetWindowMinimumSize(sdlWindow, &currentMinW, &currentMinH);
    SDL_GetWindowMaximumSize(sdlWindow, &currentMaxW, &currentMaxH);

    int newMinW = static_cast<int>(windowComp.minSize.x());
    int newMinH = static_cast<int>(windowComp.minSize.y());
    if (sizeComp != nullptr)
    {
        if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::H_FIXED) && sizeComp->size.x() > 0.0F)
        {
            newMinW = std::min(newMinW, static_cast<int>(std::round(sizeComp->size.x())));
        }
        if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::V_FIXED) && sizeComp->size.y() > 0.0F)
        {
            newMinH = std::min(newMinH, static_cast<int>(std::round(sizeComp->size.y())));
        }
    }
    const int newMaxW = (windowComp.maxSize.x() < FLT_MAX) ? static_cast<int>(windowComp.maxSize.x()) : 0;
    const int newMaxH = (windowComp.maxSize.y() < FLT_MAX) ? static_cast<int>(windowComp.maxSize.y()) : 0;

    if (newMinW != currentMinW || newMinH != currentMinH)
    {
        SDL_SetWindowMinimumSize(sdlWindow, newMinW, newMinH);
    }

    if (newMaxW != currentMaxW || newMaxH != currentMaxH)
    {
        SDL_SetWindowMaximumSize(sdlWindow, newMaxW, newMaxH);
    }
}

inline void SyncWindowFrameless(const components::Window& windowComp, SDL_Window* sdlWindow)
{
    const SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
    const bool currentlyBordered = (flags & SDL_WINDOW_BORDERLESS) == 0;
    const bool shouldBeBordered = !policies::HasFlag(windowComp.flags, policies::WindowFlag::NO_TITLE_BAR);

    if (currentlyBordered != shouldBeBordered)
    {
        SDL_SetWindowBordered(sdlWindow, shouldBeBordered);
    }
}

inline void SyncWindowResizable(const components::Window& windowComp, SDL_Window* sdlWindow)
{
    const SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
    const bool currentlyResizable = (flags & SDL_WINDOW_RESIZABLE) != 0;
    const bool shouldBeResizable = !policies::HasFlag(windowComp.flags, policies::WindowFlag::NO_RESIZE);

    if (currentlyResizable != shouldBeResizable)
    {
        SDL_SetWindowResizable(sdlWindow, shouldBeResizable);
    }
}

inline void SyncWindowOpacity(entt::entity entity, SDL_Window* sdlWindow)
{
    const auto& registry = ui::UiRuntime::current().registry();
    const auto* alphaComp = registry.try_get<components::Alpha>(entity);
    if (alphaComp == nullptr) return;

    const float currentOpacity = SDL_GetWindowOpacity(sdlWindow);
    constexpr float OPACITY_THRESHOLD = 0.01F;
    if (std::abs(currentOpacity - alphaComp->value) > OPACITY_THRESHOLD)
    {
        SDL_SetWindowOpacity(sdlWindow, alphaComp->value);
    }
}

inline void SyncWindowVisibility(entt::entity entity, SDL_Window* sdlWindow)
{
    const auto& registry = ui::UiRuntime::current().registry();
    const bool shouldBeVisible = registry.any_of<components::VisibleTag>(entity);
    const SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
    const bool currentlyVisible = (flags & SDL_WINDOW_HIDDEN) == 0;

    if (shouldBeVisible && !currentlyVisible)
    {
        SDL_ShowWindow(sdlWindow);
    }
    else if (!shouldBeVisible && currentlyVisible)
    {
        SDL_HideWindow(sdlWindow);
    }
}

inline void SyncWindowModal(entt::entity entity, const components::Window& windowComp, SDL_Window* sdlWindow)
{
    const auto& registry = ui::UiRuntime::current().registry();
    if (!registry.any_of<components::DialogTag>(entity)) return;

    const SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
    const bool currentlyModal = (flags & SDL_WINDOW_MODAL) != 0;
    const bool isModal = policies::HasFlag(windowComp.flags, policies::WindowFlag::MODAL);

    if (isModal && !currentlyModal)
    {
        SDL_SetWindowModal(sdlWindow, true);
    }
    else if (!isModal && currentlyModal)
    {
        SDL_SetWindowModal(sdlWindow, false);
    }
}

inline void SyncWindowProperties(entt::entity entity, components::Window& windowComp, SDL_Window* sdlWindow)
{
    if (sdlWindow == nullptr) return;

    SyncWindowDisplayMetrics(windowComp, sdlWindow);
    SyncWindowTitle(entity, windowComp, sdlWindow);
    SyncWindowPosition(entity, sdlWindow);
    SyncWindowSizeConstraints(entity, windowComp, sdlWindow);
    SyncWindowSize(entity, sdlWindow);
    SyncWindowResizable(windowComp, sdlWindow);
    SyncWindowFrameless(windowComp, sdlWindow);
    SyncWindowOpacity(entity, sdlWindow);
    SyncWindowVisibility(entity, sdlWindow);
    SyncWindowModal(entity, windowComp, sdlWindow);
    SyncWindowDisplayMetrics(windowComp, sdlWindow);
}

} // namespace ui::window_sync