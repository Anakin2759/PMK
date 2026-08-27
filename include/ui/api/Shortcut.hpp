#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui::shortcut
{
enum class Mod : std::uint16_t
{
    NONE = 0x0000,
    SHIFT = 0x0003,
    CTRL = 0x00C0,
    ALT = 0x0300,
    GUI = 0x0C00,
};

[[nodiscard]] constexpr Mod operator|(Mod lhs, Mod rhs) noexcept
{
    return static_cast<Mod>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

using ShortcutId = std::uint32_t;
using Callback = std::function<void()>;
using KeyCode = std::int32_t;

ShortcutId Register(KeyCode key, Callback callback);
ShortcutId Register(KeyCode key, Mod mod, Callback callback);
void Unregister(ShortcutId shortcutId);
void ClearAll();
}  // namespace ui::shortcut

namespace ui::chains
{
inline auto OnKeyPress(ui::shortcut::KeyCode key, ui::shortcut::Callback callback)
{
    auto sharedCallback = std::make_shared<ui::shortcut::Callback>(std::move(callback));
    return Chain{[key, sharedCallback](UiRuntime& /*runtime*/, ui::entity /*entity*/) mutable
                 { ui::shortcut::Register(key, [sharedCallback] { (*sharedCallback)(); }); }};
}

inline auto OnKeyPress(ui::shortcut::KeyCode key, ui::shortcut::Mod mod, ui::shortcut::Callback callback)
{
    auto sharedCallback = std::make_shared<ui::shortcut::Callback>(std::move(callback));
    return Chain{[key, mod, sharedCallback](UiRuntime& /*runtime*/, ui::entity /*entity*/) mutable
                 { ui::shortcut::Register(key, mod, [sharedCallback] { (*sharedCallback)(); }); }};
}
}  // namespace ui::chains