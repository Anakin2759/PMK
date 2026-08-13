/**
 * ************************************************************************
 *
 * @file FallbackBackendRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-24
 * @version 0.1
 * @brief SDL_Renderer 后备渲染器实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL3/SDL.h>
#include "interface/IBackendRenderer.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"

namespace ui::renderers
{

class FallbackBackendRenderer final : public interface::IBackendRenderer
{
   public:
    struct CachedBitmapTexture
    {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    FallbackBackendRenderer() = default;
    ~FallbackBackendRenderer() override
    {
        cleanup();
    }

    FallbackBackendRenderer(const FallbackBackendRenderer&) = delete;
    FallbackBackendRenderer& operator=(const FallbackBackendRenderer&) = delete;
    FallbackBackendRenderer(FallbackBackendRenderer&&) = delete;
    FallbackBackendRenderer& operator=(FallbackBackendRenderer&&) = delete;

    ui::Result<void> initialize(SDL_Window* window) override
    {
        if (window == nullptr)
        {
            return ui::Err(ui::UiErrc::INVALID_ARGUMENT, "window is null");
        }

        SDL_WindowID windowID = SDL_GetWindowID(window);
        if (m_renderer != nullptr && m_windowID == windowID)
        {
            return ui::Ok();
        }

        cleanup();

        static constexpr std::array<const char*, 4> DRIVER_CANDIDATES = {"direct3d11", "opengl", "opengles2",
                                                                         "software"};

#ifdef UI_FORCE_CPU_RENDER
        // 编译时强制 CPU 渲染：直接使用 software 驱动，跳过所有硬件加速后端
        static constexpr std::array<const char*, 1> activeDrivers = {"software"};
#else
        const auto& activeDrivers = DRIVER_CANDIDATES;
#endif

        for (const char* driver : activeDrivers)
        {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver);
            m_renderer = SDL_CreateRenderer(window, driver);
            if (m_renderer != nullptr)
            {
                SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
                m_windowID = windowID;
                ui::UiRuntime::current().logger().warn("[FallbackBackendRenderer] using SDL_Renderer driver: {}",
                                                       driver);
                return ui::Ok();
            }
        }

        ui::UiRuntime::current().logger().error("[FallbackBackendRenderer] create renderer failed: {}", SDL_GetError());
        return ui::Err(ui::UiErrc::BACKEND_UNAVAILABLE, SDL_GetError());
    }

    void cleanup() override
    {
        for (auto& cacheEntry : m_bitmapTextureCache)
        {
            if (cacheEntry.second.texture != nullptr)
            {
                SDL_DestroyTexture(cacheEntry.second.texture);
            }
        }
        m_bitmapTextureCache.clear();

        if (m_renderer != nullptr)
        {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
        m_windowID = 0;
    }

    ui::Result<void> beginFrame(const SDL_FColor& clearColor) override
    {
        if (m_renderer == nullptr)
        {
            return ui::Err(ui::UiErrc::BACKEND_UNAVAILABLE, "renderer not initialized");
        }

        SDL_SetRenderClipRect(m_renderer, nullptr);
        SDL_SetRenderDrawColorFloat(m_renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        SDL_RenderClear(m_renderer);
        return ui::Ok();
    }

    void drawBatch(const render::RenderBatch& batch, SDL_GPUTexture* whiteTextureTag) override
    {
        if (m_renderer == nullptr || batch.vertices.empty())
        {
            // 图片批次携带的是 SDL_GPUTexture*，SDL_Renderer 无法直接消费。
            // 原实现静默 return 导致图片在 CPU 路径下黑屏且无日志；此处至少告警一次，
            // 完整修复见 ImageManager::loadPixels + ImageRenderer fallback 分支（CPU_RENDER_ISSUES 方案 B）。
            if (!m_textureSkipWarned)
            {
                m_textureSkipWarned = true;
                ui::UiRuntime::current().logger().warn(
                    "[FallbackBackendRenderer] non-white texture batch cannot be rendered by SDL_Renderer; "
                    "image batch skipped (CPU fallback limitation)");
            }
            return;
        }

        if (batch.texture != nullptr && batch.texture != whiteTextureTag)
        {
            return;
        }

        if (batch.scissorRect.has_value())
        {
            SDL_SetRenderClipRect(m_renderer, &batch.scissorRect.value());
        }
        else
        {
            SDL_SetRenderClipRect(m_renderer, nullptr);
        }

        for (size_t i = 0; i + 3 < batch.vertices.size(); i += 4)
        {
            const auto& vertexTopLeft = batch.vertices[i];
            const auto& vertexTopRight = batch.vertices[i + 1];
            const auto& vertexBottomRight = batch.vertices[i + 2];
            const auto& vertexBottomLeft = batch.vertices[i + 3];

            const float minX = std::min({vertexTopLeft.position[0], vertexTopRight.position[0],
                                         vertexBottomRight.position[0], vertexBottomLeft.position[0]});
            const float minY = std::min({vertexTopLeft.position[1], vertexTopRight.position[1],
                                         vertexBottomRight.position[1], vertexBottomLeft.position[1]});
            const float maxX = std::max({vertexTopLeft.position[0], vertexTopRight.position[0],
                                         vertexBottomRight.position[0], vertexBottomLeft.position[0]});
            const float maxY = std::max({vertexTopLeft.position[1], vertexTopRight.position[1],
                                         vertexBottomRight.position[1], vertexBottomLeft.position[1]});

            const float alpha = std::clamp(vertexTopLeft.color[3] * batch.pushConstants.opacity, 0.0F, 1.0F);
            SDL_SetRenderDrawColorFloat(m_renderer, vertexTopLeft.color[0], vertexTopLeft.color[1],
                                        vertexTopLeft.color[2], alpha);

            SDL_FRect rect = {minX, minY, std::max(0.0F, maxX - minX), std::max(0.0F, maxY - minY)};
            if (rect.w <= 0.0F || rect.h <= 0.0F)
            {
                continue;
            }

            // BUG-1 修复：读取四角圆角半径。GPU 路径由 SDF 着色器在片元级计算；
            // CPU 路径此前只读 AABB 导致圆角退化为直角。此处取四角最大值作为统一圆角近似。
            const float radius = std::max({vertexTopLeft.radius[0], vertexTopRight.radius[1],
                                           vertexBottomRight.radius[2], vertexBottomLeft.radius[3]});
            const SDL_FColor color = {vertexTopLeft.color[0], vertexTopLeft.color[1], vertexTopLeft.color[2], alpha};

            if (radius > kMinRadius)
            {
                renderRoundedRect(m_renderer, rect, radius, color);
            }
            else
            {
                SDL_RenderFillRect(m_renderer, &rect);
            }
        }
    }

    ui::Result<void> drawCachedBitmap(std::string_view cacheKey, std::span<const std::uint8_t> rgbaPixels,
                                      int bitmapWidth, int bitmapHeight, const SDL_FRect& destinationRect,
                                      const std::optional<SDL_Rect>& scissorRect, std::uint8_t alphaMod) override
    {
        if (m_renderer == nullptr || bitmapWidth <= 0 || bitmapHeight <= 0 || rgbaPixels.empty())
        {
            return ui::Err(ui::UiErrc::INVALID_ARGUMENT);
        }

        CachedBitmapTexture* cachedTexture = getOrCreateBitmapTexture(cacheKey, rgbaPixels, bitmapWidth, bitmapHeight);
        if (cachedTexture == nullptr || cachedTexture->texture == nullptr)
        {
            return ui::Err(ui::UiErrc::ASSET_UPLOAD_FAILED, std::string(cacheKey));
        }

        if (scissorRect.has_value())
        {
            SDL_SetRenderClipRect(m_renderer, &scissorRect.value());
        }
        else
        {
            SDL_SetRenderClipRect(m_renderer, nullptr);
        }

        SDL_SetTextureAlphaMod(cachedTexture->texture, alphaMod);
        if (!SDL_RenderTexture(m_renderer, cachedTexture->texture, nullptr, &destinationRect))
        {
            return ui::Err(ui::UiErrc::ASSET_UPLOAD_FAILED, SDL_GetError());
        }
        return ui::Ok();
    }

    void endFrame() override
    {
        if (m_renderer == nullptr)
        {
            return;
        }

        SDL_SetRenderClipRect(m_renderer, nullptr);
        SDL_RenderPresent(m_renderer);
    }

    [[nodiscard]] interface::BackendType getType() const override
    {
        return interface::BackendType::FALLBACK;
    }

    [[nodiscard]] bool supports(interface::BackendCapability capability) const override
    {
        return capability == interface::BackendCapability::CACHED_BITMAP;
    }

   private:
    // 软件圆角近似用的几何常量（角度以弧度计，基于屏幕坐标系 y 轴向下）。
    static constexpr float kPi = 3.14159265358979323846F;
    static constexpr float kHalfPi = kPi / 2.0F;
    static constexpr float kThreeHalfPi = kPi * 3.0F / 2.0F;
    static constexpr float kTwoPi = kPi * 2.0F;
    static constexpr float kMinRadius = 0.5F;
    static constexpr int kCornerSegments = 8;

    CachedBitmapTexture* getOrCreateBitmapTexture(std::string_view cacheKey, std::span<const std::uint8_t> rgbaPixels,
                                                  int width, int height)
    {
        const std::string key(cacheKey);
        auto cacheIterator = m_bitmapTextureCache.find(key);
        const bool needsRecreate = (cacheIterator == m_bitmapTextureCache.end()) ||
                                   cacheIterator->second.texture == nullptr || cacheIterator->second.width != width ||
                                   cacheIterator->second.height != height;

        if (needsRecreate)
        {
            if (cacheIterator != m_bitmapTextureCache.end() && cacheIterator->second.texture != nullptr)
            {
                SDL_DestroyTexture(cacheIterator->second.texture);
            }

            CachedBitmapTexture newEntry{};
            newEntry.texture =
                SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
            newEntry.width = width;
            newEntry.height = height;

            if (newEntry.texture == nullptr)
            {
                ui::UiRuntime::current().logger().error("[FallbackBackendRenderer] create SDL_Texture failed: {}",
                                                        SDL_GetError());
                m_bitmapTextureCache.erase(key);
                return nullptr;
            }

            SDL_SetTextureBlendMode(newEntry.texture, SDL_BLENDMODE_BLEND);
            cacheIterator = m_bitmapTextureCache.insert_or_assign(key, newEntry).first;
        }

        if (!SDL_UpdateTexture(cacheIterator->second.texture, nullptr, rgbaPixels.data(), width * 4))
        {
            ui::UiRuntime::current().logger().error("[FallbackBackendRenderer] update SDL_Texture failed: {}",
                                                    SDL_GetError());
            return nullptr;
        }

        return &cacheIterator->second;
    }

    /// 软件圆角矩形：中心矩形 + 左右边 + 四角三角扇形近似（SDF 圆角的 CPU 降级等价物）。
    static void renderRoundedRect(SDL_Renderer* renderer, const SDL_FRect& rect, float radius, const SDL_FColor& color)
    {
        const float maxRadius = 0.5F * std::min(rect.w, rect.h);
        const float r = std::clamp(radius, 0.0F, maxRadius);
        if (r < kMinRadius)
        {
            SDL_RenderFillRect(renderer, &rect);
            return;
        }

        SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);

        const float left = rect.x;
        const float top = rect.y;
        const float right = rect.x + rect.w;
        const float bottom = rect.y + rect.h;
        const float doubleRadius = 2.0F * r;

        // 中心矩形（横向贯通）+ 左右两侧边（不含四角）
        const SDL_FRect centerRect = {left + r, top, rect.w - doubleRadius, rect.h};
        const SDL_FRect leftRect = {left, top + r, r, rect.h - doubleRadius};
        const SDL_FRect rightRect = {right - r, top + r, r, rect.h - doubleRadius};
        SDL_RenderFillRect(renderer, &centerRect);
        SDL_RenderFillRect(renderer, &leftRect);
        SDL_RenderFillRect(renderer, &rightRect);

        // 四角圆弧（左上、右上、右下、左下）
        renderCornerArc(renderer, left + r, top + r, r, kPi, kThreeHalfPi, color);
        renderCornerArc(renderer, right - r, top + r, r, kThreeHalfPi, kTwoPi, color);
        renderCornerArc(renderer, right - r, bottom - r, r, 0.0F, kHalfPi, color);
        renderCornerArc(renderer, left + r, bottom - r, r, kHalfPi, kPi, color);
    }

    static void renderCornerArc(SDL_Renderer* renderer, float centerX, float centerY, float radius, float startAngle,
                                float endAngle, const SDL_FColor& color)
    {
        std::vector<SDL_Vertex> vertices;
        vertices.reserve(static_cast<size_t>(kCornerSegments) + 2);
        vertices.push_back({SDL_FPoint{centerX, centerY}, color, SDL_FPoint{0.0F, 0.0F}});
        for (int i = 0; i <= kCornerSegments; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kCornerSegments);
            const float angle = startAngle + ((endAngle - startAngle) * t);
            const float px = centerX + (radius * std::cos(angle));
            const float py = centerY + (radius * std::sin(angle));
            vertices.push_back({SDL_FPoint{px, py}, color, SDL_FPoint{0.0F, 0.0F}});
        }

        std::vector<int> indices;
        indices.reserve(static_cast<size_t>(kCornerSegments) * 3);
        for (int i = 0; i < kCornerSegments; ++i)
        {
            indices.push_back(0);
            indices.push_back(i + 1);
            indices.push_back(i + 2);
        }

        SDL_RenderGeometry(renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()), indices.data(),
                           static_cast<int>(indices.size()));
    }

    SDL_Renderer* m_renderer = nullptr;
    SDL_WindowID m_windowID = 0;
    bool m_textureSkipWarned = false;
    std::unordered_map<std::string, CachedBitmapTexture> m_bitmapTextureCache;
};

}  // namespace ui::renderers
