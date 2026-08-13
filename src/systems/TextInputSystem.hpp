/**
 * ************************************************************************
 *
 * @file TextInputSystem.hpp
 * @brief 文本输入与键盘编辑系统
 *
 * 从 InteractionSystem 中拆出 TextEdit 相关编辑行为和键盘重复输入策略。
 *
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include <SDL3/SDL.h>

#include "common/Events.hpp"
#include "interface/ISystem.hpp"
#include "services/TextEditingService.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Dispatcher.hpp"
#include "utils/Registry.hpp"

namespace ui::systems
{

class TextInputSystem : public ui::interface::EnableRegister<TextInputSystem>
{
   public:
    TextInputSystem() = default;
    explicit TextInputSystem(UiRuntime& runtime) : m_reg(&runtime.registry()), m_disp(&runtime.dispatcher())
    {
    }

    void registerHandlersImpl()
    {
        m_disp->sink<events::TickKeyRepeat>().connect<&TextInputSystem::doProcessKeyRepeat>(*this);
        m_disp->sink<events::RawTextInput>().connect<&TextInputSystem::onRawTextInput>(*this);
        m_disp->sink<events::RawKeyInput>().connect<&TextInputSystem::onRawKeyInput>(*this);
    }

    void unregisterHandlersImpl()
    {
        m_disp->sink<events::TickKeyRepeat>().disconnect<&TextInputSystem::doProcessKeyRepeat>(*this);
        m_disp->sink<events::RawTextInput>().disconnect<&TextInputSystem::onRawTextInput>(*this);
        m_disp->sink<events::RawKeyInput>().disconnect<&TextInputSystem::onRawKeyInput>(*this);
    }

    ui::interface::SystemPhase getPhase()
    {
        return ui::interface::SystemPhase::INPUT;
    }

   private:
    void onRawTextInput(const events::RawTextInput& event)
    {
        services::TextEditingService::handleTextInput(*m_reg, event.text);
    }

    void onRawKeyInput(const events::RawKeyInput& event)
    {
        const auto key = static_cast<SDL_Keycode>(event.key);
        if (event.pressed)
        {
            if (event.repeat)
                return;
            beginKeyRepeat(key);
            services::TextEditingService::handleKeyDown(*m_reg, key, static_cast<SDL_Keymod>(event.modifiers));
            return;
        }

        handleKeyUp(key);
    }

    void doProcessKeyRepeat()
    {
        if (m_heldKey == SDLK_UNKNOWN)
            return;

        uint64_t now = SDL_GetTicks();
        if (now < m_keyPressTime + KEY_REPEAT_DELAY)
            return;
        if (now < m_lastRepeatTime + KEY_REPEAT_INTERVAL)
            return;

        m_lastRepeatTime = now;
        services::TextEditingService::handleKeyDown(*m_reg, m_heldKey, SDL_GetModState());
    }

    void beginKeyRepeat(SDL_Keycode key)
    {
        m_heldKey = key;
        m_keyPressTime = SDL_GetTicks();
        m_lastRepeatTime = m_keyPressTime;
    }

    void handleKeyUp(SDL_Keycode key)
    {
        if (key != m_heldKey)
            return;

        m_heldKey = SDLK_UNKNOWN;
        m_keyPressTime = 0;
        m_lastRepeatTime = 0;
    }

    SDL_Keycode m_heldKey = SDLK_UNKNOWN;
    uint64_t m_keyPressTime = 0;
    uint64_t m_lastRepeatTime = 0;
    static constexpr uint64_t KEY_REPEAT_DELAY = 500;
    static constexpr uint64_t KEY_REPEAT_INTERVAL = 50;
    Registry* m_reg = nullptr;
    Dispatcher* m_disp = nullptr;
};

}  // namespace ui::systems