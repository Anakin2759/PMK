/**
 * ************************************************************************
 *
 * @file Timer.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-06-02
 * @version 0.1
 * @brief VMP-ui 公共定时器接口实现
 *
 * 直接委托给 systems::TimerSystem 的静态任务管理。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include "ui/api/Timer.hpp"

#include "common/GlobalContext.hpp"
#include "core/UiRuntime.hpp"
#include "systems/TimerSystem.hpp"

namespace ui::timer
{
namespace
{
[[nodiscard]] systems::TimerSystem CurrentTimerSystem()
{
    auto& runtime = UiRuntime::current();
    runtime.registry().getOrEmplaceInCtx<globalcontext::FrameContext>();
    return systems::TimerSystem{runtime};
}
}  // namespace

Handle SetTimeout(std::function<void()> callback, std::uint32_t delayMs)
{
    if (callback == nullptr || delayMs == 0)
    {
        return NULL_HANDLE;
    }
    auto timerSystem = CurrentTimerSystem();
    return timerSystem.addTask(delayMs, std::move(callback), true);
}

Handle SetInterval(std::function<void()> callback, std::uint32_t intervalMs)
{
    if (callback == nullptr || intervalMs == 0)
    {
        return NULL_HANDLE;
    }
    auto timerSystem = CurrentTimerSystem();
    return timerSystem.addTask(intervalMs, std::move(callback), false);
}

void Clear(Handle handle) noexcept
{
    if (handle != NULL_HANDLE)
    {
        auto timerSystem = CurrentTimerSystem();
        timerSystem.cancelTask(handle);
    }
}

}  // namespace ui::timer
