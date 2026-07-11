/**
 * ************************************************************************
 *
 * @file Result.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-19
 * @version 0.2
 * @brief 项目级 Result<T> 别名（std::expected<T, ui::Error>）
 *
 * ************************************************************************
 */
#pragma once

#include "ui/ErrorCodes.hpp"

#include <expected>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace ui
{

struct Error
{
    UiErrc code = UiErrc::UNKNOWN;
    std::string context;
    std::source_location origin = std::source_location::current();

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

    friend bool operator==(const Error& error, UiErrc errorCode) noexcept { return error.code == errorCode; }
};

template <typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> Err(UiErrc errorCode,
                                                std::string context = {},
                                                std::source_location origin = std::source_location::current())
{
    return std::unexpected<Error>{Error{.code = errorCode, .context = std::move(context), .origin = origin}};
}

[[nodiscard]] inline std::unexpected<Error> Err(Error error) noexcept
{
    return std::unexpected<Error>{std::move(error)};
}

template <typename T>
[[nodiscard]] inline Result<std::remove_cvref_t<T>> Ok(T&& value)
{
    return Result<std::remove_cvref_t<T>>{std::forward<T>(value)};
}

[[nodiscard]] inline Result<void> Ok() noexcept
{
    return Result<void>{};
}

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

template <>
struct std::formatter<ui::Error> : std::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const ui::Error& error, FormatContext& ctx) const
    {
        return std::formatter<std::string_view>::format(error.ToString(), ctx);
    }
};
