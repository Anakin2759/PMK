/**
 * ************************************************************************
 *
 * @file Timer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-06-02
 * @version 0.1
 * @brief VMP-ui 公共定时器接口
 *
 * 提供轻量级的延迟执行与周期性回调能力，供客户端使用：
 * - SetTimeout  — 延迟若干毫秒后执行一次回调
 * - SetInterval — 每隔若干毫秒重复执行回调
 * - Clear       — 通过句柄取消一个未完成的定时任务
 *
 * 所有定时器在 UI 事件循环帧更新中驱动，不需要客户端自行推进。
 * 句柄为 uint32_t，Clear 调用后句柄失效。
 *
 * 用法示例：
 *   auto h = ui::timer::SetInterval([]{ fmt::println("tick"); }, 1000);
 *   ui::timer::SetTimeout([h]{ ui::timer::Clear(h); }, 5000);
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <functional>

namespace ui
{
class UiRuntime;
}

namespace ui::timer
{

/**
 * @brief 定时器句柄类型
 */
using Handle = std::uint32_t;

/// 无效句柄常量，Clear 后可作为安全空值使用
inline constexpr Handle NULL_HANDLE = 0;

/**
 * @brief 延迟执行回调（单次触发）
 * @param callback 回调函数（无参数，无返回值）
 * @param delayMs  延迟时间（毫秒），实际精度受帧率限制
 * @return 定时器句柄，可用于 Clear
 */
Handle SetTimeout(UiRuntime& runtime, std::function<void()> callback, std::uint32_t delayMs);

/**
 * @brief 周期性执行回调（重复触发）
 * @param callback  回调函数（无参数，无返回值）
 * @param intervalMs 间隔时间（毫秒），实际精度受帧率限制
 * @return 定时器句柄，可用于 Clear
 */
Handle SetInterval(UiRuntime& runtime, std::function<void()> callback, std::uint32_t intervalMs);

/**
 * @brief 取消一个定时任务
 * @param handle 由 SetTimeout / SetInterval 返回的句柄；无效句柄静默忽略
 */
void Clear(UiRuntime& runtime, Handle handle) noexcept;

} // namespace ui::timer
