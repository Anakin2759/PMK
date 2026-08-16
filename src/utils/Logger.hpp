/**
 * ************************************************************************
 *
 * @file Logger.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-26
 * @version 0.1
 * @brief ui 模块日志系统封装
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace ui::utils
{

inline constexpr size_t kMaxLogFileSize = static_cast<size_t>(1024) * 1024 * 5;  // 5MB
inline constexpr size_t kMaxLogFileCount = 1;

/**
 * @brief 辅助结构体：用于在调用点自动捕获位置和格式化字符串
 */
struct LogLocation
{
    spdlog::string_view_t fmt;
    std::source_location loc;

    template <typename T>
        requires std::convertible_to<T, spdlog::string_view_t>
    // NOLINTNEXTLINE(google-explicit-constructor) 保留 Logger::info("...") 调用点位置捕获语义。
    constexpr LogLocation(const T& format, std::source_location location = std::source_location::current())
        : fmt(format), loc(location)
    {
    }
};

/**
 * @brief 常规 Logger 类（非单例），每个实例持有独立的 spdlog::logger
 */
class Logger
{
   public:
    /**
     * @brief 使用默认配置构造 Logger（控制台 + 文件双 sink）
     */
    Logger() : m_logger(CreateDefaultLogger())
    {
    }

    /**
     * @brief 使用外部 spdlog::logger 构造
     */
    explicit Logger(std::shared_ptr<spdlog::logger> logger) : m_logger(std::move(logger))
    {
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = default;
    Logger& operator=(Logger&&) = default;
    ~Logger() = default;

    /**
     * @brief 警告日志
     */
    template <typename... Args>
    void warn(LogLocation msg, Args&&... args)
    {
        log(spdlog::level::warn, msg, std::forward<Args>(args)...);
    }

    /**
     * @brief 信息日志
     */
    template <typename... Args>
    void info(LogLocation msg, Args&&... args)
    {
        log(spdlog::level::info, msg, std::forward<Args>(args)...);
    }

    /**
     * @brief 错误日志
     */
    template <typename... Args>
    void error(LogLocation msg, Args&&... args)
    {
        log(spdlog::level::err, msg, std::forward<Args>(args)...);
    }

    /**
     * @brief 调试日志
     */
    template <typename... Args>
    void debug(LogLocation msg, Args&&... args)
    {
        log(spdlog::level::debug, msg, std::forward<Args>(args)...);
    }

    /**
     * @brief 重新配置日志文件路径（替换文件 sink，控制台 sink 保持不变）
     * @param filePath 新的日志文件路径
     */
    void reconfigure(std::string_view filePath)
    {
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(std::string(filePath), kMaxLogFileSize,
                                                                               kMaxLogFileCount);
        fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%# %!] %v");

        auto& sinks = m_logger->sinks();
        if (sinks.size() >= 2)
        {
            sinks[1] = std::move(fileSink);
        }
        else
        {
            sinks.push_back(std::move(fileSink));
        }
    }

    /**
     * @brief 获取底层 spdlog::logger
     */
    [[nodiscard]] std::shared_ptr<spdlog::logger> getLogger() const
    {
        return m_logger;
    }

   private:
    std::shared_ptr<spdlog::logger> m_logger;

    [[nodiscard]] static std::shared_ptr<spdlog::logger> CreateDefaultLogger()
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%T] [%l] %n: %v%$");

        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/vmpui.log", kMaxLogFileSize,
                                                                               kMaxLogFileCount);
        fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%# %!] %v");

        std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
        auto logger = std::make_shared<spdlog::logger>("VMP-ui", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::warn);
        return logger;
    }

    template <typename... Args>
    void log(spdlog::level::level_enum level, const LogLocation& msg, Args&&... args)
    {
        m_logger->log(
            spdlog::source_loc{msg.loc.file_name(), static_cast<int>(msg.loc.line()), msg.loc.function_name()}, level,
            fmt::runtime(msg.fmt), std::forward<Args>(args)...);
    }
};

// 辅助工具：路径规范化
inline std::string NormalizePath(const char* path)
{
    std::string result = path == nullptr ? "" : path;
    for (auto& cha : result)
    {
        if (cha == '\\')
            cha = '/';
    }
    return result;
}

}  // namespace ui::utils
