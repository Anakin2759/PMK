/**
 * ************************************************************************
 *
 * @file Result.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-19
 * @version 0.2
 * @brief 项目级 Result<T> 别名（std::expected<T, ui::Error>）
 *
 * 设计要点：
 * - 错误载体为轻量 Error 结构（错误码 + 可选上下文 + 自动捕获的源位置），
 *   替代旧 std::error_code + error_category 方案；
 * - Err() 工厂在调用点自动记录 std::source_location，传播链不丢失错误源头；
 * - 错误全部走冷路径，Error 携带 std::string 上下文的分配成本可接受。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

// 兼容旧内部包含路径；权威公共声明位于 include/ui。
#include "ui/Result.hpp"
