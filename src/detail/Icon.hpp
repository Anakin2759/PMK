/**
 * ************************************************************************
 *
 * @file Icon.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-04
 * @version 0.1
 * @brief 图标组件接口
  - 提供设置和移除图标组件的功能
  - 支持纹理图标和字体图标
  - 可配置图标位置、大小和间距
  - 简化图标组件的使用流程
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <string>

#include "common/Policies.hpp"
#include "entt/entity/fwd.hpp"

namespace ui::detail::icon
{
/**
 * @brief 设置图标组件
 * @param entity  实体
 * @param textureId 纹理ID
 * @param position 图标位置
 * @param iconSize 图标大小
 * @param spacing 图标与文本间距
 */
void SetIcon(entt::entity entity,
             const std::string& textureId,
             policies::IconFlag iconflag = policies::IconFlag::DEFAULT,
             float iconSize = 16.0F,
             float spacing = 4.0F);
/**
 * @brief 设置字体图标组件
 * @param entity 实体
 * @param fontName 字体库名称
 * @param codepoint Unicode 码点
 * @param position 图标位置
 * @param iconSize 图标大小
 * @param spacing 图标与文本间距
 */
void SetIcon(entt::entity entity,
             const std::string& fontName,
             uint32_t codepoint,
             policies::IconFlag iconflag = policies::IconFlag::DEFAULT,
             float iconSize = 16.0F,
             float spacing = 4.0F);

void RemoveIcon(entt::entity entity);
} // namespace ui::detail::icon