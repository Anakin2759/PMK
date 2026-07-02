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

#include "ErrorCodes.hpp"

#include <expected>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace ui
{

/// @brief 统一错误载体：错误码 + 可选动态上下文 + 错误产生位置。
struct Error
{
    UiErrc code = UiErrc::UNKNOWN;                                 ///< 错误码
    std::string context;                                           ///< 可选上下文（资源路径、参数值等）
    std::source_location origin = std::source_location::current(); ///< 错误产生位置（由 Err() 捕获）

    /// @brief 格式化为 "asset_not_found (fonts/x.ttf) @ File.cpp:88" 形态，供日志直接输出。
    [[nodiscard]] std::string ToString() const
    {
        std::string_view file = origin.file_name();
        if (const auto pos = file.find_last_of("/\\"); pos != std::string_view::npos)
        {
            file.remove_prefix(pos + 1);
        }
        if (context.empty())
        {
            return std::format("{} @ {}:{}", ToStringView(code), file, origin.line());
        }
        return std::format("{} ({}) @ {}:{}", ToStringView(code), context, file, origin.line());
    }

    /// @brief 与错误码枚举直接比较（C++20 自动生成反向候选）。
    friend bool operator==(const Error& error, UiErrc errorCode) noexcept { return error.code == errorCode; }
};

/// @brief 主别名：T 可为 void。
template <typename T>
using Result = std::expected<T, Error>;

/// @brief 构造失败值；调用点自动捕获源位置。
[[nodiscard]] inline std::unexpected<Error> Err(UiErrc errorCode,
                                                std::string context = {},
                                                std::source_location origin = std::source_location::current())
{
    return std::unexpected<Error>{Error{.code = errorCode, .context = std::move(context), .origin = origin}};
}

/// @brief 传播已有 Error（保留原始错误源位置与上下文）。
[[nodiscard]] inline std::unexpected<Error> Err(Error error) noexcept
{
    return std::unexpected<Error>{std::move(error)};
}

/// @brief 成功值显式构造。
template <typename T>
[[nodiscard]] inline Result<std::remove_cvref_t<T>> Ok(T&& value)
{
    return Result<std::remove_cvref_t<T>>{std::forward<T>(value)};
}

/// @brief Result<void> 的成功值显式构造。
[[nodiscard]] inline Result<void> Ok() noexcept
{
    return Result<void>{};
}

// =========================================================================
// TRY 宏 — 链式错误传播
//
// TRY 可直接在宏内声明变量：TRY(auto font, LoadFont(path));
// 失败时把错误原样向上传播（保留源头 source_location / context）。
// 注意：TRY 展开为多条语句，不能用于无花括号的 if/for 体内。
// 热路径或表达式风格代码建议改用 std::expected 的 monadic API
// （and_then / transform / or_else / transform_error）。
// =========================================================================
#define UI_TRY_CONCAT_INNER(a, b) a##b
#define UI_TRY_CONCAT(a, b) UI_TRY_CONCAT_INNER(a, b)

#define TRY(var, expr)                                                                     \
    auto UI_TRY_CONCAT(_try_result_, __LINE__) = (expr);                                   \
    if (!UI_TRY_CONCAT(_try_result_, __LINE__))                                            \
    {                                                                                      \
        return std::unexpected(std::move(UI_TRY_CONCAT(_try_result_, __LINE__)).error()); \
    }                                                                                      \
    var = *std::move(UI_TRY_CONCAT(_try_result_, __LINE__))

#define TRY_VOID(expr)                                                   \
    do                                                                   \
    {                                                                    \
        auto&& _try_void_result = (expr);                                \
        if (!_try_void_result)                                           \
        {                                                                \
            return std::unexpected(std::move(_try_void_result).error()); \
        }                                                                \
    } while (false)

} // namespace ui

/// @brief std::formatter<Error> 特化：委托 Error::ToString()。
template <>
struct std::formatter<ui::Error> : std::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const ui::Error& error, FormatContext& ctx) const
    {
        return std::formatter<std::string_view>::format(error.ToString(), ctx);
    }
};
