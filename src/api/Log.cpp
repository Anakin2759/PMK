/**
 * ************************************************************************
 *
 * @file Log.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-03-23
 * @version 0.3
 * @brief VMP-ui 公共日志接口实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include "ui/api/Log.hpp"

#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"

#include <atomic>
#include <string_view>

// Log.hpp 虽已 #undef ERROR，但其后的 UiRuntime.hpp/Logger.hpp 会经 spdlog 重新引入
// Windows wingdi.h 的 ERROR 宏，导致下方 `case Level::ERROR:` 被展开为 `case Level::0:`。
// 因此在对所有头完成包含之后再次消除该宏。
#ifdef ERROR
#undef ERROR
#endif

namespace
{

std::atomic<ui::log::Callback>& CallbackStorage()
{
    static std::atomic<ui::log::Callback> callback{nullptr};
    return callback;
}

std::atomic<ui::log::Level>& LevelStorage()
{
    static std::atomic<ui::log::Level> level{ui::log::Level::DEBUG};
    return level;
}

}  // namespace

namespace ui::log
{

void SetLevel(UiRuntime& /*runtime*/, Level level)
{
    LevelStorage().store(level, std::memory_order_relaxed);
}

void SetCallback(UiRuntime& /*runtime*/, Callback callback)
{
    CallbackStorage().store(callback, std::memory_order_relaxed);
}

void SetFilePath(UiRuntime& runtime, std::string_view path)
{
    runtime.logger().reconfigure(path);
}

void LogImpl(UiRuntime& runtime, Level level, std::string_view message)
{
    if (level < LevelStorage().load(std::memory_order_relaxed))
        return;

    // 转发到内部 spdlog Logger
    switch (level)
    {
        case Level::DEBUG:
            runtime.logger().debug("{}", message);
            break;
        case Level::INFO:
            runtime.logger().info("{}", message);
            break;
        case Level::WARNING:
            runtime.logger().warn("{}", message);
            break;
        case Level::ERROR:
            runtime.logger().error("{}", message);
            break;
        case Level::CRITICAL:
            runtime.logger().error("[CRITICAL] {}", message);
            break;
        default:
            break;
    }

    // 如果有外部回调也一并通知
    if (auto* callback = CallbackStorage().load(std::memory_order_relaxed); callback != nullptr)
    {
        callback(level, message);
    }
}

}  // namespace ui::log
