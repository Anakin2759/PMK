/**
 * ************************************************************************
 *
 * @file InteractionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-28 (Refactored)
 * @version 0.3
 * @brief 交互处理系统 - SDL事件捕获与分发层
 *
 * ## 职责（本系统实际执行的代码）
 *
 * 1. 捕获 SDL 原始事件（鼠标、键盘、滚轮、窗口）— pollSdlEvents()
 * 2. 将事件转换为内部 ECS 事件（MouseMove/MouseButton/Scroll/Key 等）并 enqueue 到事件总线
 *
 * @note 本系统**不直接调用** HitTestSystem / StateSystem / ActionSystem。
 *       这些系统同为独立的 ECS 系统，通过订阅本系统发布的事件异步响应：
 *
 * ## 逻辑事件流（跨系统协作，非本系统代码路径）
 *
 * SDL 事件捕获（InteractionSystem）
 *   ├─→ 鼠标/滚轮事件 enqueue → HitTestSystem（LOGIC 阶段）碰撞检测 → 触发 Hover/Press/Release
 *   │                                              ↓
 *   │                                   StateSystem 状态管理（Hover/Active/Focus）
 *   │                                              ↓
 *   │                                   ActionSystem 执行回调
 *   │
 *   ├─→ 键盘事件 enqueue → TextInputSystem / ShortcutSystem 处理
 *   │
 *   └─→ 窗口事件 enqueue → PlatformWindowSystem / StateSystem 窗口同步 → RenderSystem 渲染更新

 * 键盘长按处理：
    * - 记录按下时间戳
    * - 定时检查是否达到重复输入条件
    * - 触发重复按键事件
    * - 500ms初始延迟，之后每33s重复一次

 *鼠标长按和拖动处理：
    * - 在HitTestSystem中处理
    * - 记录按下位置和时间
    * - 超过阈值触发拖动开始事件
    * - 鼠标移动时持续触发拖动事件
    * - 鼠标释放时触发拖动结束事件

    * - 400ms 不超过4个像素算长按 超过是拖动
    * - 支持拖动的控件优先拖动处理
    * - 不支持点击的控件按照拖动处理
    * - 不支持拖动的控件忽略拖动事件

 *
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <string>
#include <SDL3/SDL.h>
#include "common/Events.hpp"
#include "common/components/Window.hpp"
#include "utils/Registry.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Dispatcher.hpp"
#include "interface/ISystem.hpp"
#include "common/Types.hpp"

namespace ui::systems
{

class InteractionSystem : public ui::interface::EnableRegister<InteractionSystem>
{
   public:
    InteractionSystem() = default;
    explicit InteractionSystem(UiRuntime& runtime) : m_reg(&runtime.registry()), m_disp(&runtime.dispatcher())
    {
    }

    void registerHandlersImpl()
    {
    }

    void unregisterHandlersImpl()
    {
    }

    /// OP-21: 由 SystemManager::pollInput() 调用，替代原来在 TaskChain 中的静态调用
    void pollInput()
    {
        pollSdlEvents();
    }

    /// OP-22: Input 阶段
    ui::interface::SystemPhase getPhase()
    {
        return ui::interface::SystemPhase::INPUT;
    }

    /**
     * @brief 处理 SDL每tick事件
     *
     * - 负责 SDL_PollEvent 事件
     * - 识别 Quit / Window Resized，并通过回调交由上层处理
     * - 直接从 SDL 事件中追踪鼠标状态
     */
    void pollSdlEvents()
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event))
        {
            dispatchPolledEvent(event);
        }
    }

   private:
    /**
     * @brief 分发 SDL 轮询事件到相应处理函数
     * @param event SDL 事件对象
     */
    void dispatchPolledEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                m_disp->enqueue<ui::events::QuitRequested>();
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                enqueueCloseWindowRequest(event.window.windowID);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                enqueueRawPointerMove(event.motion);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                enqueueRawPointerButton(event.button, true);
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                enqueueRawPointerButton(event.button, false);
                break;

            case SDL_EVENT_TEXT_INPUT:
                dispatchRawTextInput(event.text.text);
                break;

            case SDL_EVENT_KEY_DOWN:
                dispatchRawKeyInput(event.key, true);
                break;

            case SDL_EVENT_KEY_UP:
                dispatchRawKeyInput(event.key, false);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                enqueueRawPointerWheel(event.wheel);
                break;

            default:
                break;
        }
    }
    /**
     * @brief 根据 SDL 窗口ID查找对应实体并入队关闭事件
     * @param windowId SDL窗口ID
     */
    void enqueueCloseWindowRequest(uint32_t windowId)
    {
        entt::entity targetWindow = entt::null;
        auto view = m_reg->view<components::Window>();
        for (const entt::entity entity : view)
        {
            if (view.get<components::Window>(entity).windowID == windowId)
            {
                targetWindow = entity;
                break;
            }
        }
        if (!m_reg->valid(targetWindow))
            return;

        m_disp->enqueue<ui::events::CloseWindow>(ui::events::CloseWindow{targetWindow});
    }
    /**
     * @brief 分发文本输入事件
     * @param text 输入的文本
     */
    void dispatchRawTextInput(const char* text)
    {
        std::string input = text != nullptr ? std::string(text) : std::string();
        if (input.empty())
            return;

        m_disp->trigger<ui::events::RawTextInput>(ui::events::RawTextInput{std::move(input)});
    }

    void enqueueRawPointerMove(const SDL_MouseMotionEvent& motionEvent)
    {
        const auto pointerX = motionEvent.x;
        const auto pointerY = motionEvent.y;
        const auto deltaX = motionEvent.xrel;
        const auto deltaY = motionEvent.yrel;
        m_disp->enqueue<ui::events::RawPointerMove>(
            ui::events::RawPointerMove{Vec2(pointerX, pointerY), Vec2(deltaX, deltaY), motionEvent.windowID});
    }

    void enqueueRawPointerButton(const SDL_MouseButtonEvent& buttonEvent, bool pressed)
    {
        const auto pointerX = buttonEvent.x;
        const auto pointerY = buttonEvent.y;
        m_disp->enqueue<ui::events::RawPointerButton>(
            ui::events::RawPointerButton{Vec2(pointerX, pointerY), buttonEvent.windowID, pressed, buttonEvent.button});
    }

    void enqueueRawPointerWheel(const SDL_MouseWheelEvent& wheelEvent)
    {
        float pointerX = 0.0F;
        float pointerY = 0.0F;
        SDL_GetMouseState(&pointerX, &pointerY);

        m_disp->enqueue<ui::events::RawPointerWheel>(ui::events::RawPointerWheel{
            Vec2(pointerX, pointerY), Vec2(wheelEvent.x, wheelEvent.y), wheelEvent.windowID});
    }

    void dispatchRawKeyInput(const SDL_KeyboardEvent& keyEvent, bool pressed)
    {
        m_disp->trigger<ui::events::RawKeyInput>(ui::events::RawKeyInput{
            static_cast<int32_t>(keyEvent.key), pressed, keyEvent.repeat, static_cast<uint16_t>(keyEvent.mod)});
    }

    Registry* m_reg = nullptr;
    Dispatcher* m_disp = nullptr;
};

}  // namespace ui::systems