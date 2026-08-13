/**
 * ************************************************************************
 *
 * @file SwitchRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-08-13
 * @version 0.1
 * @brief Switch 二态开关渲染器 — 胶囊轨道 + 圆形滑块（checked 时滑块在右）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "interface/IRenderer.hpp"
#include "utils/Registry.hpp"
#include "common/components/Data.hpp"
#include "common/Tags.hpp"
#include "managers/BatchManager.hpp"

#include <SDL3/SDL_gpu.h>

namespace ui::renderers
{

class SwitchRenderer : public core::IRenderer
{
   public:
    explicit SwitchRenderer(Registry& reg) : m_reg(&reg)
    {
    }

    bool canHandle(entt::entity entity) const override
    {
        return m_reg->any_of<components::SwitchTag>(entity);
    }

    void collect(entt::entity entity, core::RenderContext& context) override
    {
        if (context.batchManager == nullptr || context.whiteTexture == nullptr)
            return;

        const auto* switchComp = m_reg->try_get<components::Switch>(entity);
        if (switchComp == nullptr)
            return;

        constexpr float THUMB_INSET = 2.0F;

        const float trackW = context.size.x();
        const float trackH = context.size.y();
        const float trackRadius = trackH * 0.5F;

        auto submitRect = [&](const Eigen::Vector2f& pos, const Eigen::Vector2f& size, const Color& color,
                              float radiusVal)
        {
            render::UiPushConstants pushConst{};
            pushConst.screen_size[0] = context.screenWidth;
            pushConst.screen_size[1] = context.screenHeight;
            pushConst.rect_size[0] = size.x();
            pushConst.rect_size[1] = size.y();
            pushConst.radius[0] = pushConst.radius[1] = pushConst.radius[2] = pushConst.radius[3] = radiusVal;
            pushConst.opacity = context.alpha;
            context.batchManager->beginBatch(context.whiteTexture, context.currentScissor, pushConst);
            const Eigen::Vector4f col{color.red, color.green, color.blue, color.alpha * context.alpha};
            context.batchManager->addRect(pos, size, col);
        };

        // 轨道
        const Color& trackColor = switchComp->checked ? switchComp->trackColorOn : switchComp->trackColor;
        submitRect(context.position, {trackW, trackH}, trackColor, trackRadius);

        // 滑块（圆形，checked 时靠右）
        const float thumbDia = std::max(0.0F, trackH - (THUMB_INSET * 2.0F));
        const float thumbRad = thumbDia * 0.5F;
        const float thumbY = context.position.y() + ((trackH - thumbDia) * 0.5F);
        const float thumbX = switchComp->checked ? (context.position.x() + trackW - thumbDia - THUMB_INSET)
                                                 : (context.position.x() + THUMB_INSET);
        submitRect({thumbX, thumbY}, {thumbDia, thumbDia}, switchComp->thumbColor, thumbRad);
    }

    int getPriority() const override
    {
        return 7;
    }

   private:
    Registry* m_reg = nullptr;
};

}  // namespace ui::renderers
