#pragma once

#include <cstdint>

#include "common/Types.hpp"

namespace ui::shortcut
{
/**
 * @brief 修饰键掩码
 *
 * 数值与 SDL_Keymod 完全兼容（无需包含 SDL 头文件）：
 *   SHIFT = SDL_KMOD_SHIFT = 0x0003
 *   CTRL  = SDL_KMOD_CTRL  = 0x00C0
 *   ALT   = SDL_KMOD_ALT   = 0x0300
 *   GUI   = SDL_KMOD_GUI   = 0x0C00
 */
enum class Mod : uint16_t
{
    NONE = 0x0000,
    SHIFT = 0x0003,
    CTRL = 0x00C0,
    ALT = 0x0300,
    GUI = 0x0C00,
};

inline Mod operator|(Mod lhs, Mod rhs)
{
    return static_cast<Mod>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}

using ShortcutId = uint32_t;
using Callback = ::ui::VoidCallback;
/// int32_t 与 SDL_Keycode (Sint32) 完全兼容，可直接传入 SDLK_* 常量
using KeyCode = int32_t;
} // namespace ui::shortcut