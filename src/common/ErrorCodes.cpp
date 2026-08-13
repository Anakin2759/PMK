/**
 * ************************************************************************
 *
 * @file ErrorCodes.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-19
 * @version 0.1
 *
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include "ui/ErrorCodes.hpp"
#include <string_view>

namespace ui
{

std::string_view ToStringView(UiErrc errorCode) noexcept
{
    switch (errorCode)
    {
        case UiErrc::INVALID_ENTITY:
            return "invalid_entity";
        case UiErrc::INVALID_ARGUMENT:
            return "invalid_argument";
        case UiErrc::REGISTRY_UNAVAILABLE:
            return "registry_unavailable";
        case UiErrc::NOT_IMPLEMENTED:
            return "not_implemented";

        case UiErrc::HIERARCHY_CYCLE:
            return "hierarchy_cycle";
        case UiErrc::HIERARCHY_DETACHED:
            return "hierarchy_detached";
        case UiErrc::ENTITY_ALREADY_EXISTS:
            return "entity_already_exists";

        case UiErrc::ASSET_NOT_FOUND:
            return "asset_not_found";
        case UiErrc::ASSET_LOAD_FAILED:
            return "asset_load_failed";
        case UiErrc::ASSET_DECODE_FAILED:
            return "asset_decode_failed";
        case UiErrc::ASSET_UPLOAD_FAILED:
            return "asset_upload_failed";
        case UiErrc::ATLAS_FULL:
            return "atlas_full";
        case UiErrc::GLYPH_RENDER_FAILED:
            return "glyph_render_failed";
        case UiErrc::FILE_OPEN_FAILED:
            return "file_open_failed";

        case UiErrc::DEVICE_UNAVAILABLE:
            return "device_unavailable";
        case UiErrc::PIPELINE_UNAVAILABLE:
            return "pipeline_unavailable";
        case UiErrc::SHADER_COMPILE_FAILED:
            return "shader_compile_failed";
        case UiErrc::SWAPCHAIN_UNAVAILABLE:
            return "swapchain_unavailable";
        case UiErrc::BACKEND_UNAVAILABLE:
            return "backend_unavailable";
        case UiErrc::WINDOW_CLAIM_FAILED:
            return "window_claim_failed";
        case UiErrc::BUFFER_MAP_FAILED:
            return "buffer_map_failed";

        case UiErrc::THEME_NOT_FOUND:
            return "theme_not_found";
        case UiErrc::THEME_TYPE_MISMATCH:
            return "theme_type_mismatch";

        case UiErrc::SCRIPT_PARSE_ERROR:
            return "script_parse_error";
        case UiErrc::SCRIPT_RUNTIME_ERROR:
            return "script_runtime_error";

        case UiErrc::UNKNOWN:
            return "unknown";
    }
    return "unrecognized";
}

}  // namespace ui
