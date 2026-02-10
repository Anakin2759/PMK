
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <mutex> // 引入 <mutex>
#include <source_location>
#include <string>

namespace utils
{
class Logger
{
    static constexpr size_t MAX_LOG_FILE_SIZE = static_cast<size_t>(1024 * 1024 * 5); // 5MB
    static constexpr size_t MAX_LOG_FILE_COUNT = 1;                                   // 仅保留 1 个日志文件
public:
    static std::shared_ptr<spdlog::logger>& getLogger()
    {
        static std::shared_ptr<spdlog::logger> logger;
        static std::once_flag flag; // 用于一次性初始化

        std::call_once(flag,
                       [&]()
                       {
                           // 1. 创建控制台 sink (stdout_color_mt)
                           auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                           consoleSink->set_pattern("%^[%T] [%l] %n: %v%$"); // 可选：设置控制台输出格式

                           // 2. 创建文件 sink (rotating_file_sink_mt)
                           auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                               "logs/pestmankill.log", MAX_LOG_FILE_SIZE, MAX_LOG_FILE_COUNT);
                           fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%# %!] %v"); // 可选：设置文件输出格式

                           // 3. 创建 logger
                           std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
                           logger = std::make_shared<spdlog::logger>("PestManKill", begin(sinks), end(sinks));

                           // 可选：设置日志级别
                           logger->set_level(spdlog::level::debug);
                           // 可选：当日志器满了时异步刷新
                           // spdlog::flush_every(std::chrono::seconds(3));
                       });

        return logger;
    }
    template <typename... Args>
    static void warn(spdlog::format_string_t<Args...> fmt,
                     Args&&... args,
                     const std::source_location LOC = std::source_location::current())
    {
        getLogger()->log(spdlog::source_loc{LOC.file_name(), static_cast<int>(LOC.line()), LOC.function_name()},
                         spdlog::level::warn,
                         fmt,
                         std::forward<Args>(args)...);
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    Logger() = default;
    ~Logger() = default;
};

inline std::string normalizePath(const char* path)
{
    std::string result = path ? path : "";
    for (auto& ch : result)
    {
        if (ch == '\\') ch = '/';
    }
    return result;
}

} // namespace utils

// ----------------- 宏包裹日志 -----------------
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#define LOG_INFO(fmt, ...)                                                                                             \
    utils::Logger::getLogger()->info("[{}:{} {}] " fmt,                                                                \
                                     utils::normalizePath(std::source_location::current().file_name()),                \
                                     std::source_location::current().line(),                                           \
                                     std::source_location::current().function_name(),                                  \
                                     ##__VA_ARGS__)
#ifdef LOG_WARN
#undef LOG_WARN
#endif
#define LOG_WARN(fmt, ...)                                                                                             \
    utils::Logger::getLogger()->warn("[{}:{} {}] " fmt,                                                                \
                                     utils::normalizePath(std::source_location::current().file_name()),                \
                                     std::source_location::current().line(),                                           \
                                     std::source_location::current().function_name(),                                  \
                                     ##__VA_ARGS__)
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif
#define LOG_ERROR(fmt, ...)                                                                                            \
    utils::Logger::getLogger()->error("[{}:{} {}] " fmt,                                                               \
                                      utils::normalizePath(std::source_location::current().file_name()),               \
                                      std::source_location::current().line(),                                          \
                                      std::source_location::current().function_name(),                                 \
                                      ##__VA_ARGS__)
#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#define LOG_DEBUG(fmt, ...)                                                                                            \
    utils::Logger::getLogger()->debug("[{}:{} {}] " fmt,                                                               \
                                      utils::normalizePath(std::source_location::current().file_name()),               \
                                      std::source_location::current().line(),                                          \
                                      std::source_location::current().function_name(),                                 \
                                      ##__VA_ARGS__)
