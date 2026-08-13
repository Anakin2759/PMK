/**
 * ************************************************************************
 *
 * @file RadioButtonRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-08-13
 * @version 0.1
 * @brief RadioButton 单选按钮渲染器 — 空心圆环 + 选中时中心圆点
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

class RadioButtonRenderer : public core::IRenderer
{
   public:
    explicit RadioButtonRenderer(Registry& reg) : m_reg(&reg)
    {
    }

    bool canHandle(entt::entity entity) const override
    {
        return m_reg->any_of<components::RadioButtonTag>(entity);
    }

    void collect(entt::entity entity, core::RenderContext& context) override
    {
        if (context.batchManager == nullptr || context.whiteTexture == nullptr)
            return;

        const auto* radioButton = m_reg->try_get<components::RadioButton>(entity);
        if (radioButton == nullptr)
            return;

        constexpr float DIAMETER = 16.0F;
        constexpr float MARGIN = 4.0F;
        constexpr float RING_THICKNESS = 2.0F;
        constexpr float DOT_DIAMETER = 8.0F;

        const float centerY = context.position.y() + ((context.size.y() - DIAMETER) * 0.5F);
        const Eigen::Vector2f ringPos{context.position.x() + MARGIN, centerY};
        const Eigen::Vector2f ringSize{DIAMETER, DIAMETER};
        const float ringRadius = DIAMETER * 0.5F;

        // 外圆环（SDF 描边模式）
        {
            render::UiPushConstants pushConst{};
            pushConst.screen_size[0] = context.screenWidth;
            pushConst.screen_size[1] = context.screenHeight;
            pushConst.rect_size[0] = ringSize.x();
            pushConst.rect_size[1] = ringSize.y();
            pushConst.radius[0] = pushConst.radius[1] = pushConst.radius[2] = pushConst.radius[3] = ringRadius;
            pushConst.opacity = context.alpha;
            pushConst.draw_mode = 1.0F;  // 描边
            pushConst.stroke_width = RING_THICKNESS;
            context.batchManager->beginBatch(context.whiteTexture, context.currentScissor, pushConst);
            const Eigen::Vector4f ringColor{radioButton->ringColor.red, radioButton->ringColor.green,
                                            radioButton->ringColor.blue, radioButton->ringColor.alpha * context.alpha};
            context.batchManager->addRect(ringPos, ringSize, ringColor);
        }

        // 选中时的中心圆点
        if (radioButton->checked)
        {
            const float dotOffset = (DIAMETER - DOT_DIAMETER) * 0.5F;
            const Eigen::Vector2f dotPos{ringPos.x() + dotOffset, ringPos.y() + dotOffset};
            const Eigen::Vector2f dotSize{DOT_DIAMETER, DOT_DIAMETER};
            const float dotRadius = DOT_DIAMETER * 0.5F;

            render::UiPushConstants pushConst{};
            pushConst.screen_size[0] = context.screenWidth;
            pushConst.screen_size[1] = context.screenHeight;
            pushConst.rect_size[0] = dotSize.x();
            pushConst.rect_size[1] = dotSize.y();
            pushConst.radius[0] = pushConst.radius[1] = pushConst.radius[2] = pushConst.radius[3] = dotRadius;
            pushConst.opacity = context.alpha;
            context.batchManager->beginBatch(context.whiteTexture, context.currentScissor, pushConst);
            const Eigen::Vector4f dotColor{radioButton->dotColor.red, radioButton->dotColor.green,
                                           radioButton->dotColor.blue, radioButton->dotColor.alpha * context.alpha};
            context.batchManager->addRect(dotPos, dotSize, dotColor);
        }
    }

    int getPriority() const override
    {
        return 7;
    }

   private:
    Registry* m_reg = nullptr;
};

}  // namespace ui::renderers
