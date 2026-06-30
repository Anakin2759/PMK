/**
 * ************************************************************************
 *
 * @file PlatformWindowSystem.hpp
 * @brief 平台窗口事件桥接系统
 *
 * 从 InteractionSystem 中拆出平台窗口事件监听逻辑。
 *
 * ************************************************************************
 */

#pragma once

#include <SDL3/SDL.h>

#include "common/Events.hpp"
#include "interface/ISystem.hpp"
#include "singleton/Dispatcher.hpp"
#include "singleton/Registry.hpp"

namespace ui::systems
{

class PlatformWindowSystem : public ui::interface::EnableRegister<PlatformWindowSystem>
{
public:
    PlatformWindowSystem() = default;
    explicit PlatformWindowSystem(Registry& /*reg*/, Dispatcher& disp) : m_disp(&disp) {}

    void registerHandlersImpl() { SDL_AddEventWatch(&PlatformWindowSystem::platformEventWatch, this); }

    void unregisterHandlersImpl() { SDL_RemoveEventWatch(&PlatformWindowSystem::platformEventWatch, this); }

    ui::interface::SystemPhase getPhase() { return ui::interface::SystemPhase::INPUT; }

private:
    /**
     * @brief SDL 事件监听回调，桥接平台窗口事件到系统内部
     * @param userdata 用户数据指针
     * @param event SDL 事件指针
     * @return true 继续处理事件
     * @return false 停止处理事件
     */
    static bool SDLCALL platformEventWatch(void* userdata, SDL_Event* event)
    {
        (void)userdata;
        if (event == nullptr) return true;

        auto* system = static_cast<PlatformWindowSystem*>(userdata);
        if (system != nullptr)
        {
            system->handlePlatformWindowEvent(*event);
        }
        return true;
    }
    /**
     * @brief 是否相关的窗口事件类型
     * @param eventType SDL 事件类型
     * @return true 相关事件
     * @return false 不相关事件
     */
    static bool isRelevantPlatformWindowEvent(Uint32 eventType)
    {
        return eventType == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED || eventType == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
            || eventType == SDL_EVENT_WINDOW_RESIZED || eventType == SDL_EVENT_WINDOW_MOVED
            || eventType == SDL_EVENT_WINDOW_SHOWN
            || eventType == SDL_EVENT_WINDOW_HIDDEN || eventType == SDL_EVENT_WINDOW_EXPOSED;
    }

    void enqueueWindowEvent(const SDL_WindowEvent& windowEvent)
    {
        if (windowEvent.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
            || windowEvent.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED || windowEvent.type == SDL_EVENT_WINDOW_RESIZED)
        {
            auto source = ui::events::WindowMetricChangeSource::DISPLAY_SCALE_CHANGED;
            if (windowEvent.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
            {
                source = ui::events::WindowMetricChangeSource::PIXEL_SIZE_CHANGED;
            }
            else if (windowEvent.type == SDL_EVENT_WINDOW_RESIZED)
            {
                source = ui::events::WindowMetricChangeSource::RESIZED;
            }
            m_disp->enqueue<ui::events::WindowPixelSizeChanged>(
                ui::events::WindowPixelSizeChanged{windowEvent.windowID, windowEvent.data1, windowEvent.data2, source});
            return;
        }

        if (windowEvent.type == SDL_EVENT_WINDOW_MOVED)
        {
            m_disp->enqueue<ui::events::WindowMoved>(
                ui::events::WindowMoved{windowEvent.windowID, windowEvent.data1, windowEvent.data2});
            return;
        }

        if (windowEvent.type == SDL_EVENT_WINDOW_EXPOSED)
        {
            m_disp->enqueue<ui::events::WindowExposed>(ui::events::WindowExposed{windowEvent.windowID});
        }
    }

    void handlePlatformWindowEvent(const SDL_Event& event)
    {
        const auto eventType = event.type;
        if (!isRelevantPlatformWindowEvent(eventType)) return;

        enqueueWindowEvent(event.window);
    }

    Dispatcher* m_disp = nullptr;
};

} // namespace ui::systems