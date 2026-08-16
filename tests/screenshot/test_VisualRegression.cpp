/**
 * ************************************************************************
 *
 * @file test_VisualRegression.cpp
 * @brief P1-5 截图回归：offscreen + software 渲染确定性图案并与 golden 比较
 *
 * 流程：
 *   1. offscreen 驱动 + software renderer（由 CMake ENVIRONMENT 保证）
 *   2. 绘制确定性场景（清屏 + 彩色矩形 + 对角线）
 *   3. SDL_RenderPresent → SDL_RenderReadPixels 读回帧
 *   4. 写实际帧 PNG 到 UI_SCREENSHOT_OUTPUT_DIR
 *   5. 与 UI_GOLDEN_DIR 下基准图比较（容差内通过；无基准则 SKIP）
 *
 * 基线管理：
 *   - 首次运行（无 golden）→ SKIP，并把实际帧输出到 output 目录
 *   - 开发者确认输出正确后，复制到 tests/golden/ 并入库
 *   - 之后 CI/本地都会比较
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

#include <SDL3/SDL.h>
#include <stb_image.h>

#include "ScreenshotCapture.hpp"

#ifndef UI_GOLDEN_DIR
#define UI_GOLDEN_DIR "tests/golden"
#endif

#ifndef UI_SCREENSHOT_OUTPUT_DIR
#define UI_SCREENSHOT_OUTPUT_DIR "screenshot-output"
#endif

namespace ui::tests::screenshot
{
namespace
{

/// 当前场景的确定性绘制：深灰背景 + 中央蓝色矩形 + 红色对角线。
bool DrawDeterministicScene(SDL_Renderer* renderer)
{
    // 清屏（深灰）
    if (!SDL_SetRenderDrawColorFloat(renderer, 0.15F, 0.15F, 0.17F, 1.0F))
    {
        return false;
    }
    if (!SDL_RenderClear(renderer))
    {
        return false;
    }

    // 中央蓝色矩形
    if (!SDL_SetRenderDrawColorFloat(renderer, 0.1F, 0.4F, 0.9F, 1.0F))
    {
        return false;
    }
    const SDL_FRect centerRect{50.0F, 30.0F, 100.0F, 60.0F};
    if (!SDL_RenderFillRect(renderer, &centerRect))
    {
        return false;
    }

    // 红色对角线
    if (!SDL_SetRenderDrawColorFloat(renderer, 0.9F, 0.1F, 0.1F, 1.0F))
    {
        return false;
    }
    if (!SDL_RenderLine(renderer, 10.0F, 10.0F, 190.0F, 110.0F))
    {
        return false;
    }

    return true;
}

/// 创建窗口 + software renderer，绘制并读回一帧。
SDL_Surface* RenderDeterministicFrame()
{
    constexpr int kWidth = 200;
    constexpr int kHeight = 120;

    SDL_Window* window = SDL_CreateWindow("screenshot-regression", kWidth, kHeight, SDL_WINDOW_HIDDEN);
    if (window == nullptr)
    {
        return nullptr;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, "software");
    if (renderer == nullptr)
    {
        SDL_DestroyWindow(window);
        return nullptr;
    }

    bool ok = DrawDeterministicScene(renderer);
    if (ok)
    {
        ok = SDL_RenderPresent(renderer);
    }

    SDL_Surface* frame = nullptr;
    if (ok)
    {
        frame = SDL_RenderReadPixels(renderer, nullptr);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return frame;
}

}  // namespace

TEST(VisualRegressionTest, DeterministicSceneMatchesGoldenWithinTolerance)
{
    // offscreen 环境由 CMake ENVIRONMENT 保证
    ASSERT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
    ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_VIDEO)) << SDL_GetError();
    ASSERT_STREQ(SDL_GetCurrentVideoDriver(), "offscreen");

    SDL_Surface* frame = RenderDeterministicFrame();
    ASSERT_NE(frame, nullptr) << "渲染读回失败: " << SDL_GetError();

    // 输出目录存放实际帧
    const auto outputDir = std::filesystem::path(UI_SCREENSHOT_OUTPUT_DIR);
    std::filesystem::create_directories(outputDir);
    const std::string actualPath = (outputDir / "deterministic_scene.png").string();
    std::string error;
    ASSERT_TRUE(WriteSurfaceToPng(frame, actualPath, error)) << error;

    // 基准图路径
    const std::string goldenPath = (std::filesystem::path(UI_GOLDEN_DIR) / "deterministic_scene.png").string();

    if (!std::filesystem::exists(goldenPath))
    {
        SDL_DestroySurface(frame);
        GTEST_SKIP() << "未找到 golden 基准: " << goldenPath
                     << "；已输出实际帧到 " << actualPath
                     << "。确认无误后请复制到 tests/golden/ 并入库，再重新运行。";
    }

    // golden 是 PNG：用 stbi_load 加载为 RGBA（字节序 R,G,B,A）
    int goldenWidth = 0;
    int goldenHeight = 0;
    int goldenChannels = 0;
    unsigned char* goldenPixels = stbi_load(goldenPath.c_str(), &goldenWidth, &goldenHeight, &goldenChannels, 4);
    ASSERT_NE(goldenPixels, nullptr) << "golden 加载失败: " << stbi_failure_reason();

    // 包装为 SDL_Surface 视图（借用像素，比较后分别释放）
    SDL_Surface* golden = SDL_CreateSurfaceFrom(goldenWidth, goldenHeight, SDL_PIXELFORMAT_ABGR8888, goldenPixels,
                                                goldenWidth * 4);
    ASSERT_NE(golden, nullptr) << SDL_GetError();

    constexpr std::uint8_t kTolerance = 4;  // 每通道容差（软件渲染确定性场景应接近 0）
    const DiffSummary diff = CompareSurfaces(frame, golden, kTolerance);

    SDL_DestroySurface(golden);
    stbi_image_free(goldenPixels);
    SDL_DestroySurface(frame);

    EXPECT_NE(diff.differingPixels, std::numeric_limits<std::uint32_t>::max()) << "尺寸不一致";
    EXPECT_EQ(diff.differingPixels, 0U)
        << "截图回归差异: " << diff.differingPixels << " 像素超容差，最大通道差=" << diff.maxChannelDiff
        << "；实际帧在 " << actualPath;
}

}  // namespace ui::tests::screenshot
