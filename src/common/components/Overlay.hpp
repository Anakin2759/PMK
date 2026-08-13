/**
 * ************************************************************************
 *
 * @file Overlay.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-08-13
 * @version 0.1
 * @brief 浮层（Overlay/Popup）组件 - 统一浮层栈的基础标记
 *
 * 用于标记一个实体是浮层（DropDown 弹出列表、未来的 Tooltip/Menu/Popover/Modal），
 * 由 OverlaySystem 统一分配 z-order、维护浮层栈、处理外部点击关闭与焦点恢复。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <entt/entt.hpp>

namespace ui::components
{

/**
 * @brief 浮层标记组件
 *
 * 挂载在浮层根实体上，记录：
 * - owner：触发该浮层的控件（关闭时焦点恢复到它）
 * - zLevel：浮层栈深度序号（0 = 最底层浮层），由 OverlaySystem 分配
 */
struct OverlayLayer
{
    using is_component_tag = void;
    entt::entity owner = entt::null;  // 触发者；关闭浮层时焦点恢复到它
    int zLevel = 0;                   // 浮层栈深度序号
};

}  // namespace ui::components
