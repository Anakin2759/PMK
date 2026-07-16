/**
 * ************************************************************************
 *
 * @file Icon.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-04
 * @version 0.1
 * @brief 图标组件接口
 *  - 提供设置和移除图标组件的功能
 *  - 支持纹理图标和字体图标
 *  - 可配置图标位置、大小和间距
 *  - 简化图标组件的使用流程
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <string>

#include "ui/Policies.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui::icon
{
inline constexpr float DEFAULT_ICON_SIZE = 16.0F;    // NOLINT(readability-magic-numbers)
inline constexpr float DEFAULT_ICON_SPACING = 4.0F; // NOLINT(readability-magic-numbers)

void SetIcon(ui::entity entity,
             const std::string& textureId,
             policies::IconFlag iconflag = policies::IconFlag::DEFAULT,
             float iconSize = DEFAULT_ICON_SIZE,
             float spacing = DEFAULT_ICON_SPACING);

void SetIcon(ui::entity entity,
             const std::string& fontName,
             uint32_t codepoint,
             policies::IconFlag iconflag = policies::IconFlag::DEFAULT,
             float iconSize = DEFAULT_ICON_SIZE,
             float spacing = DEFAULT_ICON_SPACING);

void RemoveIcon(ui::entity entity);
} // namespace ui::icon

namespace ui::actions::icon
{
inline constexpr EntityAction<static_cast<void (*)(
    ui::entity, const std::string&, uint32_t, policies::IconFlag, float, float)>(&ui::icon::SetIcon)>
    SET_FONT_ICON_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, const std::string&, policies::IconFlag, float, float)>(
    &ui::icon::SetIcon)>
    SET_TEXTURE_ICON_ACTION{};
inline constexpr EntityAction<&ui::icon::RemoveIcon> REMOVE_ICON_ACTION{};
} // namespace ui::actions::icon

namespace ui::chains
{
inline auto Icon(const std::string& fontName,
                 uint32_t codepoint,
                 policies::IconFlag iconflag = policies::IconFlag::DEFAULT,
                 float iconSize = ui::icon::DEFAULT_ICON_SIZE,
                 float spacing = ui::icon::DEFAULT_ICON_SPACING)
{
    return ui::actions::icon::SET_FONT_ICON_ACTION.bind(fontName, codepoint, iconflag, iconSize, spacing);
}

inline auto Icon(const std::string& textureId,
                 policies::IconFlag iconflag = policies::IconFlag::DEFAULT,
                 float iconSize = ui::icon::DEFAULT_ICON_SIZE,
                 float spacing = ui::icon::DEFAULT_ICON_SPACING)
{
    return ui::actions::icon::SET_TEXTURE_ICON_ACTION.bind(textureId, iconflag, iconSize, spacing);
}

inline auto RemoveIcon()
{
    return ui::actions::icon::REMOVE_ICON_ACTION.bind();
}
} // namespace ui::chains
