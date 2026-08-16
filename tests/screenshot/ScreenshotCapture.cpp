/**
 * ************************************************************************
 *
 * @file ScreenshotCapture.cpp
 * @brief P1-5 截图回归工具实现（stb_image_write PNG 写出 + 像素比较）
 *
 * ************************************************************************
 */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "ScreenshotCapture.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

#include <SDL3/SDL_pixels.h>

namespace ui::tests::screenshot
{

bool WriteSurfaceToPng(const SDL_Surface* surface, const std::string& path, std::string& error)
{
    if (surface == nullptr || surface->pixels == nullptr || surface->w <= 0 || surface->h <= 0)
    {
        error = "invalid surface (null or empty)";
        return false;
    }

    // 统一按 4 通道写出；若源 surface 通道数不足，stb 仍按 stride 读取像素行。
    const int channels = SDL_BYTESPERPIXEL(surface->format) >= 3 ? 4 : SDL_BYTESPERPIXEL(surface->format);
    if (channels != 4)
    {
        error = "unsupported surface format (expected 4 bytes per pixel)";
        return false;
    }

    const int written = stbi_write_png(path.c_str(), surface->w, surface->h, channels, surface->pixels,
                                       surface->pitch);
    if (written == 0)
    {
        error = "stbi_write_png failed: " + path;
        return false;
    }
    return true;
}

DiffSummary CompareSurfaces(const SDL_Surface* lhs, const SDL_Surface* rhs, std::uint8_t tolerance)
{
    DiffSummary summary;

    if (lhs == nullptr || rhs == nullptr || lhs->pixels == nullptr || rhs->pixels == nullptr)
    {
        summary.differingPixels = std::numeric_limits<std::uint32_t>::max();
        return summary;
    }

    if (lhs->w != rhs->w || lhs->h != rhs->h || SDL_BYTESPERPIXEL(lhs->format) < 3 ||
        SDL_BYTESPERPIXEL(rhs->format) < 3)
    {
        summary.differingPixels = std::numeric_limits<std::uint32_t>::max();
        summary.width = lhs->w;
        summary.height = lhs->h;
        return summary;
    }

    summary.width = lhs->w;
    summary.height = lhs->h;
    const int channels = std::min(SDL_BYTESPERPIXEL(lhs->format), SDL_BYTESPERPIXEL(rhs->format));
    const int compareChannels = std::min(channels, 3);  // 忽略 alpha，只比较 RGB

    const auto* lhsBytes = static_cast<const std::uint8_t*>(lhs->pixels);
    const auto* rhsBytes = static_cast<const std::uint8_t*>(rhs->pixels);

    for (int y = 0; y < lhs->h; ++y)
    {
        const std::uint8_t* lhsRow = lhsBytes + (static_cast<std::size_t>(y) * static_cast<std::size_t>(lhs->pitch));
        const std::uint8_t* rhsRow = rhsBytes + (static_cast<std::size_t>(y) * static_cast<std::size_t>(rhs->pitch));
        for (int x = 0; x < lhs->w; ++x)
        {
            bool pixelDiffers = false;
            for (int channel = 0; channel < compareChannels; ++channel)
            {
                const std::uint8_t lhsValue = lhsRow[static_cast<std::size_t>(x) * static_cast<std::size_t>(channels) +
                                                     static_cast<std::size_t>(channel)];
                const std::uint8_t rhsValue = rhsRow[static_cast<std::size_t>(x) * static_cast<std::size_t>(channels) +
                                                     static_cast<std::size_t>(channel)];
                const std::uint8_t channelDiff =
                    static_cast<std::uint8_t>(lhsValue > rhsValue ? lhsValue - rhsValue : rhsValue - lhsValue);
                summary.maxChannelDiff = std::max(summary.maxChannelDiff, channelDiff);
                if (channelDiff > tolerance)
                {
                    pixelDiffers = true;
                }
            }
            if (pixelDiffers)
            {
                ++summary.differingPixels;
            }
        }
    }
    return summary;
}

}  // namespace ui::tests::screenshot
