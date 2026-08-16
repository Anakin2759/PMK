/**
 * ************************************************************************
 *
 * @file ScreenshotCapture.hpp
 * @brief P1-5 截图回归工具：SDL_Surface → PNG 写出与像素比较
 *
 * 提供两个能力：
 * - WriteSurfaceToPng：把渲染读回的 SDL_Surface 写成 PNG（stb_image_write）
 * - CompareSurfaces：逐像素容差比较，返回差异像素数与最大通道差
 *
 * 使用方式（测试代码）：
 *   SDL_Surface* frame = SDL_RenderReadPixels(renderer, nullptr);
 *   screenshot::WriteSurfaceToPng(frame, "build/screenshot/frame.png", error);
 *   auto diff = screenshot::CompareSurfaces(frame, golden, kTolerance);
 *
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <string>

#include <SDL3/SDL_surface.h>

namespace ui::tests::screenshot
{

/// 像素比较汇总
struct DiffSummary
{
    std::uint32_t differingPixels = 0;  // 任一通道差超过容差的像素数
    std::uint8_t maxChannelDiff = 0;    // 所有通道最大绝对差
    int width = 0;                      // 被比较 surface 的宽度
    int height = 0;                     // 被比较 surface 的高度
};

/**
 * @brief 把 SDL_Surface 保存为 PNG（stb_image_write）。
 *
 * @param surface 源 surface（RGBA32 或 RGB 格式均可；按 pitch 逐行写出）
 * @param path 输出 PNG 路径（目录需已存在）
 * @param error 失败时填充原因
 * @return true 成功
 */
[[nodiscard]] bool WriteSurfaceToPng(const SDL_Surface* surface, const std::string& path, std::string& error);

/**
 * @brief 逐像素比较两个 surface，返回超出容差的统计。
 *
 * 两 surface 尺寸不一致时返回 differingPixels = UINT32_MAX。
 *
 * @param lhs 实际帧
 * @param rhs 基准图（golden）
 * @param tolerance 每通道最大允许差（0-255）
 */
[[nodiscard]] DiffSummary CompareSurfaces(const SDL_Surface* lhs, const SDL_Surface* rhs, std::uint8_t tolerance);

}  // namespace ui::tests::screenshot
