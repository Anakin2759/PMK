#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <spdlog/sinks/stdout_color_sinks.h>
static constexpr size_t MAX_LOG_FILE_SIZE = static_cast<size_t>(1024 * 1024 * 5); // 5MB
static constexpr size_t MAX_LOG_FILES = 1;
inline std::shared_ptr<spdlog::logger> CreateRollingLogger()
{
    std::filesystem::create_directories("logs");

    auto logger = spdlog::rotating_logger_mt("game_logger",
                                             "logs/debug.log",
                                             MAX_LOG_FILE_SIZE, // 5MB
                                             MAX_LOG_FILES      // 保留1个文件
    );

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::info);
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    sink->set_level(spdlog::level::debug);
    logger->sinks().push_back(sink);
    return logger;
}