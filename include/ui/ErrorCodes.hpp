/**
 * ************************************************************************
 *
 * @file ErrorCodes.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-19
 * @version 0.1
 * @brief 项目统一错误码 UiErrc 枚举 + 名称映射
 *
 * 错误载体 ui::Error 与 Result<T> 别名见 Result.hpp。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <format>
#include <string_view>

namespace ui
{

enum class UiErrc : std::uint16_t
{
    INVALID_ENTITY = 1,
    INVALID_ARGUMENT = 2,
    REGISTRY_UNAVAILABLE = 3,
    NOT_IMPLEMENTED = 4,

    HIERARCHY_CYCLE = 100,
    HIERARCHY_DETACHED = 101,
    ENTITY_ALREADY_EXISTS = 102,

    ASSET_NOT_FOUND = 200,
    ASSET_LOAD_FAILED = 201,
    ASSET_DECODE_FAILED = 202,
    ASSET_UPLOAD_FAILED = 203,
    ATLAS_FULL = 204,
    GLYPH_RENDER_FAILED = 205,
    FILE_OPEN_FAILED = 206,

    DEVICE_UNAVAILABLE = 300,// 设备不可用（GPU、渲染上下文、窗口等）
    PIPELINE_UNAVAILABLE = 301,
    SHADER_COMPILE_FAILED = 302,
    SWAPCHAIN_UNAVAILABLE = 303,
    BACKEND_UNAVAILABLE = 304,
    WINDOW_CLAIM_FAILED = 305,
    BUFFER_MAP_FAILED = 306,

    THEME_NOT_FOUND = 400,
    THEME_TYPE_MISMATCH = 401,

    SCRIPT_PARSE_ERROR = 500,
    SCRIPT_RUNTIME_ERROR = 501,

    UNKNOWN = 900,
};

[[nodiscard]] std::string_view ToStringView(UiErrc errorCode) noexcept;

}  // namespace ui

template <>
struct std::formatter<ui::UiErrc> : std::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(ui::UiErrc errorCode, FormatContext& ctx) const
    {
        return std::formatter<std::string_view>::format(ui::ToStringView(errorCode), ctx);
    }
};
