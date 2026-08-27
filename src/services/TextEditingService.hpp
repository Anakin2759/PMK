/**
 * ************************************************************************
 *
 * @file TextEditingService.hpp
 * @brief TextEdit 编辑命令服务，处理 SDL_TEXTINPUT/SDL_KEYDOWN 事件，更新 TextEdit 组件状态。
 *
 * ************************************************************************
 */

#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace ui
{
class Registry;
}

namespace ui::services
{

class TextEditingService
{
   public:
    static void handleTextInput(Registry& reg, const std::string& rawText); // 处理 SDL_TEXTINPUT 事件
    static void handleKeyDown(Registry& reg, SDL_Keycode key, SDL_Keymod modState);// 处理 SDL_KEYDOWN 事件
};

}  // namespace ui::services