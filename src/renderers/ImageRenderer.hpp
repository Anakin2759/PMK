/**
 * ************************************************************************
 *
 * @file ImageRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-18
 * @version 0.1
 * @brief 图像渲染器 - 渲染 Image 组件，支持从 ImageSource 路径懒加载纹理
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <algorithm>
#include <cmath>

#include "interface/IRenderer.hpp"
#include "interface/IBackendRenderer.hpp"
#include "utils/Registry.hpp"
#include "common/CustomizationPoints.hpp"
#include "common/components/Data.hpp"
#include "common/Tags.hpp"
#include "managers/BatchManager.hpp"
#include "managers/ImageManager.hpp"
#include <SDL3/SDL_gpu.h>

namespace ui::renderers
{

/**
 * @brief 图像渲染器
 *
 * 负责渲染：
 * - 带纹理的 Image 组件（直接使用 textureId）
 * - ImageSource 路径懒加载纹理（首帧自动上传到 GPU）
 */
class ImageRenderer : public core::IRenderer
{
   public:
    explicit ImageRenderer(Registry& reg) : m_reg(&reg)
    {
    }

    [[nodiscard]] bool canHandle(entt::entity entity) const override
    {
        return m_reg->any_of<components::ImageTag>(entity);
    }

    void collect(entt::entity entity, core::RenderContext& context) override
    {
        if (context.backendRenderer != nullptr &&
            context.backendRenderer->getType() == interface::BackendType::FALLBACK)
        {
            collectFallback(entity, context);
            return;
        }

        if (context.batchManager == nullptr || context.whiteTexture == nullptr)
        {
            return;
        }

        // 懒加载：若有 ImageSource 且尚未加载
        if (auto* src = m_reg->try_get<components::ImageSource>(entity))
        {
            if (!src->loaded && !src->loadFailed && !src->path.empty())
            {
                if (context.imageManager != nullptr)
                {
                    if (auto r = context.imageManager->loadTexture(src->path); r)
                    {
                        auto& img = m_reg->get_or_emplace<components::Image>(entity);
                        img.textureId = static_cast<void*>(*r);
                        src->loaded = true;
                    }
                    else
                    {
                        src->loadFailed = true;
                    }
                }
                else
                {
                    src->loadFailed = true;
                }
            }
        }

        const auto* img = m_reg->try_get<components::Image>(entity);
        if (img == nullptr || img->textureId == nullptr)
        {
            return;
        }

        auto* tex = static_cast<SDL_GPUTexture*>(img->textureId);

        render::UiPushConstants pushConstants{};
        pushConstants.screen_size[0] = context.screenWidth;
        pushConstants.screen_size[1] = context.screenHeight;
        pushConstants.rect_size[0] = context.size.x();
        pushConstants.rect_size[1] = context.size.y();
        pushConstants.opacity = context.alpha;
        pushConstants.padding = 1.0F;

        context.batchManager->beginBatch(tex, context.currentScissor, pushConstants);

        const Eigen::Vector4f tint{img->tintColor.red, img->tintColor.green, img->tintColor.blue, img->tintColor.alpha};

        context.batchManager->addRect(context.position, context.size, tint,
                                      Eigen::Vector2f{img->uvMin.x(), img->uvMin.y()},
                                      Eigen::Vector2f{img->uvMax.x(), img->uvMax.y()});
    }

    [[nodiscard]] int getPriority() const override
    {
        return IMAGE_RENDER_PRIORITY;
    }

   private:
    void collectFallback(entt::entity entity, core::RenderContext& context)
    {
        if (!ui::cpo::backend_supports(*context.backendRenderer, interface::BackendCapability::CACHED_BITMAP))
        {
            return;
        }

        auto* src = m_reg->try_get<components::ImageSource>(entity);
        if (src == nullptr || src->loadFailed || src->path.empty() || context.imageManager == nullptr)
        {
            return;
        }

        auto pixelsResult = context.imageManager->loadPixels(src->path);
        if (!pixelsResult)
        {
            src->loadFailed = true;
            return;
        }

        const auto* pixels = *pixelsResult;
        const auto* image = m_reg->try_get<components::Image>(entity);
        const float tintAlpha = image != nullptr ? image->tintColor.alpha : 1.0F;
        const float combinedAlpha = std::clamp(context.alpha * tintAlpha, 0.0F, 1.0F);
        const auto alphaMod = static_cast<std::uint8_t>(std::lround(combinedAlpha * 255.0F));
        const SDL_FRect destinationRect = {context.position.x(), context.position.y(), context.size.x(),
                                           context.size.y()};

        // 当前 drawCachedBitmap 仅支持整体 alpha 调制，RGB tint 暂由 CPU 后端忽略。
        // CPU 解码成功不设置 ImageSource::loaded；该标志保留为 GPU 纹理已就绪语义。
        if (!context.backendRenderer->drawCachedBitmap(src->path, pixels->rgba, pixels->width, pixels->height,
                                                       destinationRect, context.currentScissor, alphaMod))
        {
            return;
        }
    }

    static constexpr int IMAGE_RENDER_PRIORITY = 5;
    Registry* m_reg = nullptr;
};

}  // namespace ui::renderers
