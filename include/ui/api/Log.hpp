/**
 * ************************************************************************
 *
 * @file Log.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-03-23
 * @version 0.3
 * @brief VMP-ui 公共日志接口
 *
 * 客户端可通过 ui::log::SetCallback 注入自定义输出函数，
 * 也可直接调用 ui::log::Info / Debug / Warning / Error / Critical
 * 向库内部日志系统写入。支持 std::format 格式化参数。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "ui/api/Chains.hpp"

namespace ui
{
class UiRuntime;
}

#ifdef ERROR
#undef ERROR
#endif

namespace ui::log
{
enum class Level : std::uint8_t
{
    NO_DEBUG = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

using Callback = void (*)(Level, std::string_view);

void SetLevel(UiRuntime& runtime, Level level);
void SetCallback(UiRuntime& runtime, Callback callback);
void SetFilePath(UiRuntime& runtime, std::string_view path);
void LogImpl(UiRuntime& runtime, Level level, std::string_view message);

inline void Debug(UiRuntime& runtime, std::string_view message)
{
    LogImpl(runtime, Level::DEBUG, message);
}
inline void Info(UiRuntime& runtime, std::string_view message)
{
    LogImpl(runtime, Level::INFO, message);
}
inline void Warning(UiRuntime& runtime, std::string_view message)
{
    LogImpl(runtime, Level::WARNING, message);
}
inline void Error(UiRuntime& runtime, std::string_view message)
{
    LogImpl(runtime, Level::ERROR, message);
}
inline void Critical(UiRuntime& runtime, std::string_view message)
{
    LogImpl(runtime, Level::CRITICAL, message);
}

template <typename... Args>
inline void Debug(UiRuntime& runtime, std::format_string<Args...> fmt, Args&&... args)
{
    LogImpl(runtime, Level::DEBUG, std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args>
inline void Info(UiRuntime& runtime, std::format_string<Args...> fmt, Args&&... args)
{
    LogImpl(runtime, Level::INFO, std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args>
inline void Warning(UiRuntime& runtime, std::format_string<Args...> fmt, Args&&... args)
{
    LogImpl(runtime, Level::WARNING, std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args>
inline void Error(UiRuntime& runtime, std::format_string<Args...> fmt, Args&&... args)
{
    LogImpl(runtime, Level::ERROR, std::format(fmt, std::forward<Args>(args)...));
}
template <typename... Args>
inline void Critical(UiRuntime& runtime, std::format_string<Args...> fmt, Args&&... args)
{
    LogImpl(runtime, Level::CRITICAL, std::format(fmt, std::forward<Args>(args)...));
}
}  // namespace ui::log

namespace ui::chains
{
inline auto LogDebug(std::string_view message)
{
    return Chain{[msg = std::string(message)](UiRuntime& runtime, ui::entity) { ui::log::Debug(runtime, msg); }};
}
inline auto LogInfo(std::string_view message)
{
    return Chain{[msg = std::string(message)](UiRuntime& runtime, ui::entity) { ui::log::Info(runtime, msg); }};
}
inline auto LogWarning(std::string_view message)
{
    return Chain{[msg = std::string(message)](UiRuntime& runtime, ui::entity) { ui::log::Warning(runtime, msg); }};
}
inline auto LogError(std::string_view message)
{
    return Chain{[msg = std::string(message)](UiRuntime& runtime, ui::entity) { ui::log::Error(runtime, msg); }};
}
inline auto LogCritical(std::string_view message)
{
    return Chain{[msg = std::string(message)](UiRuntime& runtime, ui::entity) { ui::log::Critical(runtime, msg); }};
}

template <typename... Args>
inline auto LogDebug(std::format_string<Args...> fmt, Args&&... args)
{
    return Chain{[msg = std::format(fmt, std::forward<Args>(args)...)](UiRuntime& runtime, ui::entity) {
        ui::log::Debug(runtime, msg);
    }};
}
template <typename... Args>
inline auto LogInfo(std::format_string<Args...> fmt, Args&&... args)
{
    return Chain{[msg = std::format(fmt, std::forward<Args>(args)...)](UiRuntime& runtime, ui::entity) {
        ui::log::Info(runtime, msg);
    }};
}
template <typename... Args>
inline auto LogWarning(std::format_string<Args...> fmt, Args&&... args)
{
    return Chain{[msg = std::format(fmt, std::forward<Args>(args)...)](UiRuntime& runtime, ui::entity) {
        ui::log::Warning(runtime, msg);
    }};
}
template <typename... Args>
inline auto LogError(std::format_string<Args...> fmt, Args&&... args)
{
    return Chain{[msg = std::format(fmt, std::forward<Args>(args)...)](UiRuntime& runtime, ui::entity) {
        ui::log::Error(runtime, msg);
    }};
}
template <typename... Args>
inline auto LogCritical(std::format_string<Args...> fmt, Args&&... args)
{
    return Chain{[msg = std::format(fmt, std::forward<Args>(args)...)](UiRuntime& runtime, ui::entity) {
        ui::log::Critical(runtime, msg);
    }};
}
}  // namespace ui::chains