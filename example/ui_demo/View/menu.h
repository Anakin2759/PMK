#pragma once

#include <ui.hpp>

#include "Mainwindow.h"

namespace example::ui_demo::view
{
using namespace ui::chains;

/**
 * @brief 创建初始菜单对话框
 */
inline void CreateMenuDialog(ui::UiRuntime& runtime)
{
    auto existing = ui::query::FindByAlias(runtime, "menuDialog");
    if (existing.has_value())
    {
        ui::log::Info(runtime, "Menu dialog already exists, skipping creation.");
        return;
    }

    if (existing.error() != ui::UiErrc::INVALID_ENTITY)
    {
        ui::log::Error(runtime, "Failed to check for existing menu dialog: {}", existing.error().ToString());
        return;
    }

    // 创建菜单对话框
    auto menuDialog = ui::factory::CreateDialog(runtime, "VMP-ui Menu", "menuDialog");

    WithRuntime(runtime, menuDialog,
                Size(200.0F, 300.0F) | BackgroundColor({0.15F, 0.15F, 0.15F, 0.95F}) | BorderRadius(12.0F) |
                    LayoutDirection(ui::policies::LayoutDirection::VERTICAL) | Spacing(15.0F) | Padding(20.0F));

    // 创建标题标签
    auto titleLabel = ui::factory::CreateLabel(runtime, "欢迎来到 害虫杀", "titleLabel");

    WithRuntime(runtime, titleLabel,
                TextAlignment(ui::policies::Alignment::CENTER) | TextColor({1.0F, 0.9F, 0.3F, 1.0F}) |
                    FontSize(18.0F));  // 金黄色

    WithRuntime(runtime, menuDialog, AddChild(titleLabel));

    // 创建分隔间距
    WithRuntime(runtime, menuDialog, AddChild(ui::factory::CreateSpacer(runtime, 1, "spacer1")));

    // 定义通用按钮样式
    auto buttonStyle = FixedSize(150.0F, 40.0F) | TextAlignment(ui::policies::Alignment::CENTER) | BorderRadius(5.0F) |
                       BorderThickness(2.0F);

    // 创建开始按钮
    auto startBtnResult = ui::factory::CreateButton(runtime, "开始", "startBtn");
    if (!startBtnResult)
    {
        ui::log::Error(runtime, "Failed to create start button: {}", startBtnResult.error().ToString());
        return;
    }
    const auto startBtn = startBtnResult->raw;

    WithRuntime(runtime, startBtn,
                buttonStyle | BackgroundColor({0.2F, 0.4F, 0.8F, 1.0F}) | BorderColor({0.4F, 0.6F, 1.0F, 1.0F}) |
                    FontSize(14.0F) | BorderRadius(10.0F) |
                    OnClick(
                        [&runtime, menuDialog]()
                        {
                            CreateMainWindow(runtime);
                            ui::utils::CloseWindow(runtime, menuDialog);
                        }));

    WithRuntime(runtime, menuDialog, AddChild(startBtn));

    // 创建设置按钮
    auto settingsBtnResult = ui::factory::CreateButton(runtime, "设置", "settingsBtn");
    if (!settingsBtnResult)
    {
        ui::log::Error(runtime, "Failed to create settings button: {}", settingsBtnResult.error().ToString());
        return;
    }
    const auto settingsBtn = settingsBtnResult->raw;

    WithRuntime(runtime, settingsBtn,
                buttonStyle | TextColor({1.0F, 1.0F, 1.0F, 1.0F}) | BackgroundColor({0.3F, 0.3F, 0.3F, 1.0F}) |
                    BorderColor({0.5F, 0.5F, 0.5F, 1.0F}) | FontSize(14.0F));

    WithRuntime(runtime, menuDialog, AddChild(settingsBtn));

    // 创建退出按钮
    auto exitBtnResult = ui::factory::CreateButton(runtime, "退出", "exitBtn");
    if (!exitBtnResult)
    {
        ui::log::Error(runtime, "Failed to create exit button: {}", exitBtnResult.error().ToString());
        return;
    }
    const auto exitBtn = exitBtnResult->raw;

    WithRuntime(runtime, exitBtn,
                buttonStyle | BackgroundColor({0.6F, 0.2F, 0.2F, 1.0F}) | BorderColor({0.8F, 0.3F, 0.3F, 1.0F}) |
                    FontSize(14.0F) |
                    OnClick(
                        [&runtime]()
                        {
                            ui::log::Info(runtime, "退出menu.");
                            ui::utils::QuitUiEventLoop(runtime);
                        }));

    WithRuntime(runtime, menuDialog, AddChild(exitBtn));

    // 创建底部间距
    WithRuntime(runtime, menuDialog, AddChild(ui::factory::CreateSpacer(runtime, 1, "spacer2")));

    // 创建版本信息标签
    auto versionLabel = ui::factory::CreateLabel(runtime, "v0.1.0 - 2026", "versionLabel");

    WithRuntime(runtime, versionLabel,
                TextAlignment(ui::policies::Alignment::CENTER) | TextColor({0.6F, 0.6F, 0.6F, 1.0F}) |
                    FontSize(12.0F));

    WithRuntime(runtime, menuDialog, AddChild(versionLabel));

    // 显示菜单对话框
    ui::log::Info(runtime, "Showing menu dialog...");
    WithRuntime(runtime, menuDialog, Show());
    ui::log::Info(runtime, "CreateMenuDialog completed.");
}

}  // namespace example::ui_demo::view
