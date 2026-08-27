#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <ui.hpp>

namespace example::ui_demo::view
{
using namespace ui::chains;

namespace detail
{
inline ui::Result<ui::entity> CreateButton(ui::UiRuntime& runtime, const std::string& text, std::string_view alias)
{
    auto result = ui::factory::CreateButton(runtime, text, alias);
    if (!result)
        return ui::Err(result.error());
    return ui::Ok(result->raw);
}

inline ui::entity MakeSectionTitle(ui::UiRuntime& runtime, const std::string& text, const std::string& alias)
{
    const auto label = ui::factory::CreateLabel(runtime, text, alias);
    WithRuntime(runtime, label,
                TextColor({1.0F, 0.85F, 0.5F, 1.0F}) | FontSize(14.0F) |
                    SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FIXED) | Size(0.0F, 22.0F) |
                    TextAlignment(ui::policies::Alignment::LEFT | ui::policies::Alignment::VCENTER));
    return label;
}

inline std::string DemoAssetPath(std::string_view fileName)
{
    return (std::filesystem::path(__FILE__).parent_path().parent_path() / "assets" / fileName).string();
}

inline ui::entity MakeImageCard(ui::UiRuntime& runtime, std::string_view title, std::string_view fileName,
                                const std::string& alias)
{
    const auto card = ui::factory::CreateVBoxLayout(runtime, alias);
    WithRuntime(runtime, card,
                FixedSize(96.0F, 124.0F) | BackgroundColor({0.12F, 0.12F, 0.16F, 0.92F}) | BorderRadius(6.0F) |
                    Padding(6.0F) | Spacing(5.0F));
    const auto image = ui::factory::CreateImageFromPath(runtime, DemoAssetPath(fileName), 84.0F, 84.0F, alias + "_image");
    const auto label = ui::factory::CreateLabel(runtime, std::string(title), alias + "_label");
    WithRuntime(runtime, image, FixedSize(84.0F, 84.0F) | BorderRadius(4.0F));
    WithRuntime(runtime, label, TextAlignment(ui::policies::Alignment::CENTER) | FontSize(11.0F));
    WithRuntime(runtime, card, AddChild(image) | AddChild(label));
    return card;
}
}  // namespace detail

inline void CreateMainWindow(ui::UiRuntime& runtime)  // NOLINT
{
    auto windowResult = ui::factory::CreateWindow(runtime, "UI Controls Demo", "gameWindow");
    if (!windowResult)
    {
        ui::log::Error(runtime, "Failed to create main window: {}", windowResult.error().ToString());
        return;
    }
    const auto gameWindow = windowResult->raw;
    WithRuntime(runtime, gameWindow,
                WindowFlag(ui::policies::WindowFlag::DEFAULT) | Size(1200.0F, 960.0F) |
                    BackgroundColor({0.10F, 0.10F, 0.12F, 1.0F}) |
                    LayoutDirection(ui::policies::LayoutDirection::VERTICAL) | Spacing(8.0F) | Padding(8.0F));

    auto panelStyle = BackgroundColor({0.06F, 0.06F, 0.09F, 0.85F}) | BorderRadius(6.0F) | Padding(8.0F) | Spacing(5.0F);
    const auto row = ui::factory::CreateHBoxLayout(runtime, "controlsRow");
    WithRuntime(runtime, row, SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FILL) | Spacing(8.0F));
    WithRuntime(runtime, gameWindow, AddChild(row));

    const auto controls = ui::factory::CreateVBoxLayout(runtime, "inputPanel");
    WithRuntime(runtime, controls, panelStyle | FixedSize(420.0F, 420.0F));
    WithRuntime(runtime, row, AddChild(controls));
    WithRuntime(runtime, controls, AddChild(detail::MakeSectionTitle(runtime, "Input Controls", "inputTitle")));

    auto primaryResult = detail::CreateButton(runtime, "Primary", "primaryBtn");
    if (!primaryResult)
    {
        ui::log::Error(runtime, "Failed to create primary button: {}", primaryResult.error().ToString());
        return;
    }
    const auto primary = *primaryResult;
    WithRuntime(runtime, primary,
                FixedSize(120.0F, 36.0F) | BackgroundColor({0.20F, 0.50F, 0.85F, 1.0F}) | BorderRadius(5.0F) |
                    OnClick([&runtime]() { ui::log::Info(runtime, "Primary button"); }));
    WithRuntime(runtime, controls, AddChild(primary));

    const auto lineEdit = ui::factory::CreateLineEdit(runtime, "", "Enter text...", "demoLineEdit");
    WithRuntime(runtime, lineEdit,
                SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FIXED) | Size(0.0F, 32.0F) |
                    BackgroundColor({0.15F, 0.15F, 0.18F, 0.95F}) | Padding(6.0F) |
                    OnTextChanged([&runtime](const std::string&) { ui::log::Info(runtime, "LineEdit changed"); }));
    WithRuntime(runtime, controls, AddChild(lineEdit));

    const auto progress = ui::factory::CreateProgressBar(runtime, "demoProgress");
    WithRuntime(runtime, progress,
                SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FIXED) | Size(0.0F, 18.0F) |
                    ProgressValue(0.40F) | ProgressFillColor({0.20F, 0.75F, 0.45F, 1.0F}));
    const auto slider = ui::factory::CreateSlider(runtime, "demoSlider");
    WithRuntime(runtime, slider,
                SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FIXED) | Size(0.0F, 24.0F) |
                    SliderRange(0.0F, 100.0F) | SliderValue(40.0F) |
                    OnSliderValueChanged([&runtime, progress](float value) {
                        WithRuntime(runtime, progress, ProgressValue(value / 100.0F));
                    }));
    WithRuntime(runtime, controls, AddChild(progress) | AddChild(slider));

    const auto canvasPanel = ui::factory::CreateVBoxLayout(runtime, "canvasPanel");
    WithRuntime(runtime, canvasPanel, panelStyle | SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FILL));
    WithRuntime(runtime, row, AddChild(canvasPanel));
    WithRuntime(runtime, canvasPanel, AddChild(detail::MakeSectionTitle(runtime, "Canvas Drawing", "canvasTitle")));
    const auto canvas = ui::factory::CreateCanvas(runtime, 600.0F, 300.0F, "demoCanvas");
    WithRuntime(runtime, canvas,
                SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FILL) |
                    BackgroundColor({0.08F, 0.08F, 0.11F, 1.0F}) |
                    CanvasDrawLine({10.0F, 10.0F}, {200.0F, 80.0F}, {0.30F, 0.80F, 0.40F, 1.0F}, 2.0F) |
                    CanvasDrawRect({220.0F, 10.0F}, {390.0F, 90.0F}, {0.30F, 0.60F, 1.00F, 1.0F}, 2.0F));
    ui::canvas::DrawFilledCircle(runtime, canvas, {230.0F, 180.0F}, 55.0F, {0.40F, 0.30F, 0.80F, 0.85F});
    WithRuntime(runtime, canvasPanel, AddChild(canvas));

    const auto imageRow = ui::factory::CreateHBoxLayout(runtime, "imageRow");
    WithRuntime(runtime, imageRow, SizePolicy(ui::policies::Size::H_FILL | ui::policies::Size::V_FIXED) | Spacing(8.0F));
    WithRuntime(runtime, imageRow,
                AddChild(detail::MakeImageCard(runtime, "PNG", "sample.png", "pngCard")) |
                    AddChild(detail::MakeImageCard(runtime, "JPEG", "sample.jpg", "jpegCard")) |
                    AddChild(detail::MakeImageCard(runtime, "BMP", "sample.bmp", "bmpCard")));
    WithRuntime(runtime, gameWindow, AddChild(imageRow));

    WithRuntime(runtime, gameWindow, Show());
    ui::log::Info(runtime, "Main window created: Runtime-aware controls demo");
}

}  // namespace example::ui_demo::view
