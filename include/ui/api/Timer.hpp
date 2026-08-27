#pragma once

#include <cstdint>
#include <functional>

namespace ui::timer
{
using Handle = std::uint32_t;
inline constexpr Handle NULL_HANDLE = 0;
Handle SetTimeout(std::function<void()> callback, std::uint32_t delayMs);
Handle SetInterval(std::function<void()> callback, std::uint32_t intervalMs);
void Clear(Handle handle) noexcept;
}  // namespace ui::timer
