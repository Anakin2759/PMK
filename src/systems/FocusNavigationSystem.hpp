/**
 * ************************************************************************
 *
 * @file FocusNavigationSystem.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-08-13
 * @version 0.1
 * @brief 焦点导航系统 - Tab / Shift+Tab 在可聚焦控件间移动键盘焦点
 *
 * 职责：
 * - 监听 RawKeyInput，识别 Tab / Shift+Tab
 * - 按创建顺序遍历 FocusableTag 实体（跳过 Disabled / 不可见）
 * - 计算目标实体后触发 FocusChangeRequest，由 StateSystem 复用 setFocus/clearFocus
 *   完整逻辑（含 TextEdit 光标与 SDL 文本输入）
 *
 * 注意：本系统不直接读写 FocusedTag，焦点状态统一由 StateSystem 持有。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <vector>

#include <entt/entt.hpp>

#include "common/Events.hpp"
#include "core/UiRuntime.hpp"
#include "interface/ISystem.hpp"
#include "utils/Dispatcher.hpp"
#include "utils/Registry.hpp"

namespace ui::systems
{

class FocusNavigationSystem : public ui::interface::EnableRegister<FocusNavigationSystem>
{
   public:
    FocusNavigationSystem() = default;
    explicit FocusNavigationSystem(UiRuntime& runtime) : m_reg(&runtime.registry()), m_disp(&runtime.dispatcher())
    {
    }

    void registerHandlersImpl();
    void unregisterHandlersImpl();

    [[nodiscard]] ui::interface::SystemPhase getPhase()
    {
        return ui::interface::SystemPhase::LOGIC;
    }

   private:
    void onRawKeyInput(const events::RawKeyInput& event);

    /// Tab / Shift+Tab 顺序导航（限定在同一焦点作用域内循环，构成焦点陷阱）。
    void navigateSequential(bool backward);

    /// 方向键空间导航（Up/Down/Left/Right，基于实体矩形中心的轴向距离）。
    void navigateSpatial(int32_t key);

    /// 收集可聚焦实体；scope 非空时仅返回该作用域（窗口/对话框）内的实体。
    [[nodiscard]] std::vector<entt::entity> collectFocusables(entt::entity scope) const;

    /// 找到实体的焦点作用域（最近的 Window/Dialog 祖先）；无则返回 entt::null。
    [[nodiscard]] entt::entity findFocusScope(entt::entity entity) const;

    /// 判断 entity 是否位于 anchor 的子树内。
    [[nodiscard]] bool isDescendantOf(entt::entity entity, entt::entity anchor) const;

    /// 触发 FocusChangeRequest，由 StateSystem 复用 setFocus 完整逻辑。
    void requestFocus(entt::entity target);

    Registry* m_reg = nullptr;
    Dispatcher* m_disp = nullptr;
};

}  // namespace ui::systems
