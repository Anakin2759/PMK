/**
 * ************************************************************************
 *
 * @file test_ScreenshotCapture.cpp
 * @brief P1-5 截图回归工具自测：PNG 写出/读回、像素比较
 *
 * 不依赖窗口/渲染环境，可普通运行。
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include <SDL3/SDL.h>
#include <stb_image.h>

#include "ScreenshotCapture.hpp"

namespace ui::tests::screenshot
{
namespace
{

TEST(ScreenshotCaptureTest, WritesPngAndReloadsWithExpectedPixels)
{
    constexpr int kWidth = 64;
    constexpr int kHeight = 48;

    SDL_Surface* surface = SDL_CreateSurface(kWidth, kHeight, SDL_PIXELFORMAT_ABGR8888);
    ASSERT_NE(surface, nullptr) << SDL_GetError();

    // 填充为红色（ABGR8888 下字节序为 R,G,B,A）
    const std::uint32_t color = SDL_MapSurfaceRGBA(surface, 255, 0, 0, 255);
    ASSERT_TRUE(SDL_FillSurfaceRect(surface, nullptr, color)) << SDL_GetError();

    const auto outputDir = std::filesystem::temp_directory_path() / "vmp_screenshot_test";
    std::filesystem::create_directories(outputDir);
    const std::string pngPath = (outputDir / "capture.png").string();

    std::string error;
    ASSERT_TRUE(WriteSurfaceToPng(surface, pngPath, error)) << error;

    // 用 stbi_load 读回（链接 ui 库内已定义的 STB_IMAGE 实现）
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(pngPath.c_str(), &width, &height, &channels, 4);
    ASSERT_NE(pixels, nullptr) << stbi_failure_reason();
    EXPECT_EQ(width, kWidth);
    EXPECT_EQ(height, kHeight);
    EXPECT_GE(channels, 3);

    // 首像素应为红色
    EXPECT_EQ(pixels[0], 255);
    EXPECT_EQ(pixels[1], 0);
    EXPECT_EQ(pixels[2], 0);

    stbi_image_free(pixels);
    SDL_DestroySurface(surface);
}

TEST(ScreenshotCaptureTest, CompareSurfacesDetectsDifference)
{
    constexpr int kWidth = 32;
    constexpr int kHeight = 32;

    SDL_Surface* red = SDL_CreateSurface(kWidth, kHeight, SDL_PIXELFORMAT_ABGR8888);
    SDL_Surface* blue = SDL_CreateSurface(kWidth, kHeight, SDL_PIXELFORMAT_ABGR8888);
    ASSERT_NE(red, nullptr);
    ASSERT_NE(blue, nullptr);

    const std::uint32_t redColor = SDL_MapSurfaceRGBA(red, 255, 0, 0, 255);
    const std::uint32_t blueColor = SDL_MapSurfaceRGBA(blue, 0, 0, 255, 255);
    ASSERT_TRUE(SDL_FillSurfaceRect(red, nullptr, redColor));
    ASSERT_TRUE(SDL_FillSurfaceRect(blue, nullptr, blueColor));

    // 相同图：零差异
    const DiffSummary identical = CompareSurfaces(red, red, 0);
    EXPECT_EQ(identical.differingPixels, 0U);

    // 红 vs 蓝：全部像素差异
    const DiffSummary different = CompareSurfaces(red, blue, 0);
    EXPECT_EQ(different.differingPixels, static_cast<std::uint32_t>(kWidth * kHeight));
    EXPECT_GE(different.maxChannelDiff, 255U);

    // 大容差下视为一致
    const DiffSummary tolerant = CompareSurfaces(red, blue, 255);
    EXPECT_EQ(tolerant.differingPixels, 0U);

    SDL_DestroySurface(red);
    SDL_DestroySurface(blue);
}

}  // namespace
}  // namespace ui::tests::screenshot
