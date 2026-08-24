/**
 * ************************************************************************
 *
 * @file Factory.hpp
 * @brief UI 应用、窗口和控件的公开创建 API。
 *
 * ************************************************************************
 */
#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "ui/Application.hpp"
#include "ui/Callback.hpp"
#include "ui/MathTypes.hpp"
#include "ui/Result.hpp"
#include "ui/WindowsMacroShield.hpp"
#include "ui/api/Entity.hpp"

namespace ui
{
class UiRuntime;

struct EntityHandle
{
    entity raw{null_entity};
};

struct WindowHandle
{
    entity raw{null_entity};
    std::uint32_t windowId{0};
    std::uintptr_t token{0};
};

inline Result<EntityHandle> MakeEntityHandle(std::uintptr_t /*token*/, entity raw)
{
    return EntityHandle{raw};
}

inline Result<WindowHandle> MakeWindowHandle(std::uintptr_t token, entity raw, std::uint32_t windowId)
{
    return WindowHandle{raw, windowId, token};
}
}  // namespace ui

namespace ui::factory
{
ui::Result<std::unique_ptr<Application>> CreateApplication(std::span<char*> argv);
ui::entity CreateBaseWidget(std::string_view alias = "");
ui::Result<ui::EntityHandle> CreateBaseWidget(UiRuntime& runtime, std::string_view alias = "");
void CreateFadeInAnimation(ui::entity entity, float duration);

ui::entity CreateButton(const std::string& content, std::string_view alias = "");
ui::Result<ui::EntityHandle> CreateButton(UiRuntime& runtime, const std::string& content, std::string_view alias = "");
ui::entity CreateLabel(const std::string& content, std::string_view alias = "");
ui::entity CreateTextEdit(const std::string& placeholder = "", bool multiline = false, std::string_view alias = "");
ui::entity CreateImage(void* textureId, float defaultWidth = 50.0F, float defaultHeight = 50.0F,
                       std::string_view alias = "");
ui::entity CreateArrow(const Vec2& start, const Vec2& end, std::string_view alias = "");
ui::entity CreateSpacer(int stretchFactor = 1, std::string_view alias = "");
ui::entity CreateSpacer(float width, float height, std::string_view alias = "");
ui::entity CreateDialog(std::string_view title, std::string_view alias = "");
ui::entity CreateScrollArea(std::string_view alias = "");
ui::entity CreateWindow(std::string_view title, std::string_view alias = "");
ui::Result<ui::WindowHandle> CreateWindow(UiRuntime& runtime, std::string_view title, std::string_view alias = "");
ui::entity CreateTitleBar(ui::entity windowEntity, std::string_view alias = "");
ui::entity CreateVBoxLayout(std::string_view alias = "");
ui::entity CreateHBoxLayout(std::string_view alias = "");
ui::entity CreateLineEdit(std::string_view initialText = "", std::string_view placeholder = "",
                          std::string_view alias = "");
ui::entity CreateTextBrowser(std::string_view initialText = "", std::string_view placeholder = "",
                             std::string_view alias = "");
ui::entity CreateCheckBox(const std::string& label, bool checked = false, std::string_view alias = "");
ui::entity CreateSwitch(bool checked = false, std::string_view alias = "");
ui::entity CreateRadioGroup(const std::vector<std::string>& options, int selectedIndex = 0, std::string_view alias = "");
ui::entity CreateTabView(const std::vector<std::string>& tabTitles, std::string_view alias = "");
ui::entity GetTabContent(ui::entity tabView, int index);
ui::entity CreateDropDown(const std::vector<std::string>& options, int selectedIndex = 0, std::string_view alias = "");
void CloseDropDownPopup(ui::entity ddEntity);

/// 创建列表视图（单选/多选 + 滚动）。items 为数据源；selectedIndex 为初始单选索引（-1 无选中）。
ui::entity CreateListView(const std::vector<std::string>& items, int selectedIndex = -1, std::string_view alias = "");

/// 向列表追加一项（返回新 item 实体）。
ui::entity AddListItem(ui::entity listView, const std::string& text, std::string_view alias = "");

/// 给目标实体附加 Tooltip 悬浮提示；悬停 delayMs 毫秒后显示，移开即隐藏。
inline constexpr int kDefaultTooltipDelayMs = 500;
ui::entity SetTooltip(ui::entity target, const std::string& text, int delayMs = kDefaultTooltipDelayMs);

template <typename EntityLike>
    requires(!std::same_as<std::remove_cvref_t<EntityLike>, ui::entity> &&
             (std::is_enum_v<std::remove_cvref_t<EntityLike>> || std::is_integral_v<std::remove_cvref_t<EntityLike>>))
void CloseDropDownPopup(EntityLike ddEntity)
{
    CloseDropDownPopup(static_cast<ui::entity>(ddEntity));
}

ui::entity CreateSlider(std::string_view alias = "");
ui::entity CreateProgressBar(std::string_view alias = "");
ui::entity CreateImageFromPath(std::string_view path, float defaultWidth = 0.0F, float defaultHeight = 0.0F,
                               std::string_view alias = "");
ui::entity CreateCanvas(float width = 400.0F, float height = 300.0F, std::string_view alias = "");
ui::entity CreateTable(int columns = 3, std::string_view alias = "");

// ===================== ContextMenu（右键菜单，复用 OverlaySystem） =====================

/// 创建上下文菜单容器（VBox，固定宽度，圆角背景）。
ui::entity CreateContextMenu(std::string_view alias = "");

/// 添加菜单项；点击执行 onClick 后自动关闭菜单。返回菜单项实体。
ui::entity AddContextMenuItem(ui::entity menu, const std::string& text, ui::Callback<> onClick = {});

/// 在指定窗口内坐标打开菜单（position 为窗口内逻辑坐标）。
void ShowContextMenu(ui::entity menu, const Vec2& position, ui::entity owner = ui::null_entity);

/// 关闭并收起菜单（浮层出栈，实体保留可复用）。
void CloseContextMenu(ui::entity menu);

// ===================== ModalDialog（模态浮层，复用 OverlaySystem） =====================

/// 创建模态对话框（遮罩 + 居中内容容器），parentWindow 为其所属窗口根实体。
ui::entity CreateModalDialog(ui::entity parentWindow, std::string_view alias = "");

/// 打开模态对话框（遮罩覆盖父窗口客户区，作为浮层入栈）。
void ShowModalDialog(ui::entity dialog);

/// 关闭模态对话框（浮层出栈并销毁遮罩与内容）。
void CloseModalDialog(ui::entity dialog);
}  // namespace ui::factory
