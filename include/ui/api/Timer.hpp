#pragma once

#include <cstdint>
#include <functional>

namespace ui
{
class UiRuntime;
}

namespace ui::timer
{
using Handle = std::uint32_t;
inline constexpr Handle NULL_HANDLE = 0;
Handle SetTimeout(UiRuntime& runtime, std::function<void()> callback, std::uint32_t delayMs);
Handle SetInterval(UiRuntime& runtime, std::function<void()> callback, std::uint32_t intervalMs);
void Clear(UiRuntime& runtime, Handle handle) noexcept;
} // namespace ui::timer
