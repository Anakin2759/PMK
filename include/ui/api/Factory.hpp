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
ui::entity CreateDropDown(const std::vector<std::string>& options, int selectedIndex = 0, std::string_view alias = "");
void CloseDropDownPopup(ui::entity ddEntity);

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
}  // namespace ui::factory
