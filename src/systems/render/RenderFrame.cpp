/**
 * @file RenderFrame.cpp
 * @brief RenderSystem 渲染帧处理逻辑
 *
 */

#include "systems/RenderSystem.hpp"
#include "RenderSystemImpl.hpp"
#include "RenderDirty.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ranges>
#include <stack>
#include "common/GlobalContext.hpp"
#include "common/EigenConversions.hpp"
#include "common/components/Window.hpp"
#include "common/AppConfig.hpp"
#include "core/WindowSync.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"
#include "core/RenderContext.hpp"
#include "core/PlatformWindow.hpp"
#include "managers/BatchManager.hpp"
#include "managers/CommandBuffer.hpp"
#include "managers/DeviceManager.hpp"
#include "managers/PipelineCache.hpp"
#include "interface/IBackendRenderer.hpp"
#include "interface/IRenderer.hpp"
#include "Eigen/src/Core/Matrix.h"
#include "common/components/Layout.hpp"
#include "entt/entity/fwd.hpp"
#include <utility>
#include "common/components/Visual.hpp"
#include "common/components/Interaction.hpp"
#include "common/Types.hpp"
#include "SDL3/SDL_rect.h"
#include "utils/Registry.hpp"
#include "common/Tags.hpp"
#include "helper/Helper.hpp"

namespace ui::systems
{

namespace
{

[[nodiscard]] float ComputeRenderScale(int pixelWidth, int pixelHeight, int logicalWidth, int logicalHeight,
                                       float fallback)
{
    const bool hasLogicalWidth = logicalWidth > 0;
    const bool hasLogicalHeight = logicalHeight > 0;
    const bool hasPixelWidth = pixelWidth > 0;
    const bool hasPixelHeight = pixelHeight > 0;

    const float scaleX =
        (hasPixelWidth && hasLogicalWidth) ? static_cast<float>(pixelWidth) / static_cast<float>(logicalWidth) : 0.0F;
    const float scaleY = (hasPixelHeight && hasLogicalHeight)
                             ? static_cast<float>(pixelHeight) / static_cast<float>(logicalHeight)
                             : 0.0F;

    if (std::isfinite(scaleX) && scaleX > 0.0F && std::isfinite(scaleY) && scaleY > 0.0F)
    {
        return std::max(scaleX, scaleY);
    }

    if (std::isfinite(scaleX) && scaleX > 0.0F)
    {
        return scaleX;
    }

    if (std::isfinite(scaleY) && scaleY > 0.0F)
    {
        return scaleY;
    }

    return (std::isfinite(fallback) && fallback > 0.0F) ? fallback : 1.0F;
}

[[nodiscard]] SDL_FColor DetermineClearColor(Registry& registry, entt::entity windowEntity)
{
    constexpr SDL_FColor OPAQUE_FALLBACK = {.r = 0.10F, .g = 0.10F, .b = 0.12F, .a = 1.0F};
    constexpr SDL_FColor TRANSPARENT_FALLBACK = {.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 0.0F};

    const bool isTransparentWindow = registry.any_of<components::DialogTag>(windowEntity);
    // Dialog 的背景由 ShapeRenderer 以 SDF 圆角绘制。若先用背景色清空整个
    // 交换链，SDF 在圆角外 discard 后仍会留下矩形底色，表现为圆角外尖角。
    if (isTransparentWindow)
    {
        return TRANSPARENT_FALLBACK;
    }

    if (const auto* background = registry.try_get<components::Background>(windowEntity);
        background != nullptr && background->enabled == policies::Feature::ENABLED)
    {
        return SDL_FColor{.r = background->color.red,
                          .g = background->color.green,
                          .b = background->color.blue,
                          .a = 1.0F};
    }

    return OPAQUE_FALLBACK;
}

// NOLINTNEXTLINE(readability-function-size,readability-function-cognitive-complexity)
void LogScalingSnapshotIfNeeded(render_detail::WindowRenderState& windowState, utils::Logger& logger,
                                Registry& registry, entt::entity windowEntity, const components::Window& windowComp,
                                SDL_Window* sdlWindow, int logicalWidth, int logicalHeight, int pixelWidth,
                                int pixelHeight, float dpiScale, const SDL_FColor& clearColor, int batchCount)
{
    if (!config::AppConfig::instance().debugScaling())
    {
        return;
    }

    const auto entityKey = static_cast<uint32_t>(windowEntity);
    const auto* rootSize = registry.try_get<components::Size>(windowEntity);
    const auto nativeMetrics = platform::GetNativeWindowMetrics(sdlWindow);
    render_detail::ScalingSnapshot snapshot{.pixelWidth = pixelWidth,
                             .pixelHeight = pixelHeight,
                             .logicalWidth = logicalWidth,
                             .logicalHeight = logicalHeight,
                             .rootWidth = rootSize != nullptr ? static_cast<int>(std::lround(rootSize->size.x())) : 0,
                             .rootHeight = rootSize != nullptr ? static_cast<int>(std::lround(rootSize->size.y())) : 0,
                             .batchCount = batchCount,
                             .dpiScale = dpiScale,
                             .clearAlpha = clearColor.a,
                             .nativeClientWidth = nativeMetrics.clientWidth,
                             .nativeClientHeight = nativeMetrics.clientHeight,
                             .nativeWindowWidth = nativeMetrics.windowWidth,
                             .nativeWindowHeight = nativeMetrics.windowHeight,
                             .borderTop = nativeMetrics.borderTop,
                             .borderBottom = nativeMetrics.borderBottom};

    if (windowState.scalingSnapshot.has_value() && windowState.scalingSnapshot.value() == snapshot)
    {
        return;
    }
    windowState.scalingSnapshot = snapshot;

    logger.info(
        "[Scaling][RenderFrame] entity={} windowId={} logical=({}, {}) pixel=({}, {}) rootSize=({}, {}) "
        "displayScale={:.3f} uiScale={:.3f} renderScale={:.3f} clear=({:.2f}, {:.2f}, {:.2f}, {:.2f}) "
        "batches={} nativeClient=({}, {}) nativeWindow=({}, {}) nativeBorderTB=({}, {})",
        entityKey, windowComp.windowID, logicalWidth, logicalHeight, pixelWidth, pixelHeight, snapshot.rootWidth,
        snapshot.rootHeight, windowComp.displayScale, windowComp.uiScale, dpiScale, clearColor.r, clearColor.g,
        clearColor.b, clearColor.a, batchCount, nativeMetrics.clientWidth, nativeMetrics.clientHeight,
        nativeMetrics.windowWidth, nativeMetrics.windowHeight, nativeMetrics.borderTop, nativeMetrics.borderBottom);
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity,readability-function-size)
void RenderSystem::update()
{
    // 统计事件处理入口次数而非实际 present 数；无脏窗口的调用同样属于帧调度指标。
    auto& frameContext = m_reg->getOrEmplaceInCtx<globalcontext::FrameContext>();
    ++frameContext.renderUpdateCount;
    if (frameContext.stage != globalcontext::FrameStage::RENDER || frameContext.renderUpdateCount != 1U)
    {
        return;
    }
    auto windowView = m_reg->view<components::Window, components::RenderDirtyTag>();

    if (windowView.begin() == windowView.end())
    {
        return;
    }

    if (m_impl->m_firstUpdate)
    {
        m_logger->info("[RenderSystem] update first call");
        m_impl->m_firstUpdate = false;
    }

    ensureInitialized();

    if (!m_impl->m_useFallback)
    {
        SDL_GPUDevice const* device = m_impl->m_deviceManager->getDevice();
        if (device == nullptr)
        {
            m_logger->warn("GPU device not ready");
            return;
        }
    }

    m_impl->m_stats.frameCount++;
    m_impl->m_stats.batchCount = 0;
    m_impl->m_stats.vertexCount = 0;

    for (auto windowEntity : windowView)
    {
        auto& windowComp = windowView.get<components::Window>(windowEntity);
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp.windowID);
        if (sdlWindow == nullptr)
        {
            m_logger->warn("Window entity has no SDL window");
            continue;
        }

        window_sync::SyncWindowDisplayMetrics(*m_logger, windowComp, sdlWindow);

        int width = static_cast<int>(windowComp.pixelSize.x());
        int height = static_cast<int>(windowComp.pixelSize.y());
        if (width <= 0 || height <= 0)
        {
            continue;
        }

        int logicalWidth = static_cast<int>(windowComp.logicalSize.x());
        int logicalHeight = static_cast<int>(windowComp.logicalSize.y());
        if (logicalWidth <= 0 || logicalHeight <= 0)
        {
            logicalWidth = width;
            logicalHeight = height;
        }

        const float dpiScale = ComputeRenderScale(width, height, logicalWidth, logicalHeight, windowComp.displayScale);
        const SDL_FColor clearColor = DetermineClearColor(*m_reg, windowEntity);
        auto& windowState = m_impl->m_windowStates[windowComp.windowID];
        windowState.windowID = windowComp.windowID;
        windowState.textScaleKey = render_detail::MakeTextScaleKey(dpiScale);
        windowState.capabilityDiagnostics.SetLogger(*m_logger);

        if (!m_impl->m_useFallback && !m_impl->m_deviceClaimState.AreResourcesReady())
        {
            if (auto claimResult = m_impl->m_deviceManager->claimWindow(sdlWindow); !claimResult.has_value())
            {
                m_logger->warn("[RenderSystem] claimWindow failed: {}",
                                                       claimResult.error().ToString());
                continue;
            }
            m_impl->m_deviceClaimState.MarkDeviceLocked();
            ensureGpuResourcesInitialized();
            if (!m_impl->m_deviceClaimState.AreResourcesReady())
            {
                continue;
            }
        }

        if (!m_impl->m_useFallback && m_impl->m_pipelineCache->getPipeline() == nullptr)
        {
            if (auto pipeResult = m_impl->m_pipelineCache->createPipeline(sdlWindow); !pipeResult.has_value())
            {
                m_logger->warn("[RenderSystem] pipeline creation failed: {}",
                                                       pipeResult.error().ToString());
            }

            if (m_impl->m_pipelineCache->getPipeline() == nullptr)
            {
                m_logger->warn(
                    "[RenderSystem] GPU pipeline unavailable; switching to fallback renderer. "
                    "Rebuild shaders with compile.bat to restore GPU rendering.");
                m_impl->m_useFallback = true;
                if (!tryInitializeFallback(sdlWindow))
                {
                    m_logger->error(
                        "[RenderSystem] fallback initialization failed; skipping this frame");
                }
                continue;
            }
        }

        if (!m_impl->m_useFallback && m_impl->m_deviceManager->getWhiteTexture() == nullptr)
        {
            m_logger->error("[RenderSystem] DeviceManager white texture unavailable");
            continue;
        }

        const float screenWidth = static_cast<float>(logicalWidth);
        const float screenHeight = static_cast<float>(logicalHeight);
        if (m_impl->m_fontManager != nullptr)
        {
            static_cast<void>(m_impl->m_fontManager->setDpiScale(dpiScale));
        }

        interface::IBackendRenderer* fallbackBackend = nullptr;
        if (m_impl->m_useFallback)
        {
            auto backendResult = tryInitializeFallback(sdlWindow);
            if (!backendResult.has_value())
            {
                m_logger->warn("[RenderSystem] fallback initialization failed: {}",
                               backendResult.error().ToString());
                continue;
            }
            fallbackBackend = backendResult.value();
        }

        m_impl->m_batchManager->clear();
        m_impl->m_renderQueue.clear();
        m_impl->m_submissionIndex = 0;

        if (m_reg->any_of<components::VisibleTag>(windowEntity))
        {
            assert(m_impl->m_batchManager != nullptr && "RenderSystem must own BatchManager during frame collection");
            assert(m_impl->m_fontManager != nullptr && "RenderSystem must own FontManager during frame collection");
            core::RenderContext rootContext;
            rootContext.screenWidth = screenWidth;
            rootContext.screenHeight = screenHeight;
            rootContext.dpiScale = dpiScale;
            rootContext.deviceManager = m_impl->m_deviceManager.get();
            rootContext.fontManager = m_impl->m_fontManager.get();
            rootContext.imageManager = m_impl->m_imageManager.get();
            rootContext.textTextureCache = m_impl->m_textTextureCache.get();
            rootContext.batchManager = m_impl->m_batchManager.get();
            rootContext.backendRenderer = fallbackBackend;
            rootContext.capabilityDiagnostics = &windowState.capabilityDiagnostics;
            rootContext.sdlWindow = sdlWindow;
            // GPU 路径仅借用 DeviceManager 唯一持有的白色纹理，不转移所有权。
            rootContext.whiteTexture =
                m_impl->m_useFallback ? m_impl->m_fallbackWhiteTextureTag : m_impl->m_deviceManager->getWhiteTexture();

            Eigen::Vector2f rootOffset = Eigen::Vector2f(0, 0);
            if (const auto* pos = m_reg->try_get<components::Position>(windowEntity))
            {
                rootOffset = -detail::eigen::ToEigen(pos->value);
            }

            rootContext.position = rootOffset;
            rootContext.alpha = 1.0F;

            collectRenderData(windowEntity, rootContext);
        }

        // Sort render queue by RenderKey (Z-Order primarily)
        std::ranges::sort(m_impl->m_renderQueue, {}, &RenderSystemImpl::RenderItem::sortKey);

        if (m_impl->m_useFallback)
        {
            if (fallbackBackend == nullptr || !fallbackBackend->beginFrame(clearColor))
            {
                continue;
            }

            bool frameSucceeded = true;
            for (auto& renderItem : m_impl->m_renderQueue)
            {
                if (auto collectResult = renderItem.renderer->collectChecked(renderItem.entity, renderItem.context);
                    !collectResult.has_value())
                {
                    frameSucceeded = false;
                    m_logger->warn("[RenderSystem] fallback collect failed: {}", collectResult.error().ToString());
                    break;
                }

                m_impl->m_batchManager->optimize();
                const auto& fallbackBatches = m_impl->m_batchManager->getBatches();
                for (const auto& batch : fallbackBatches)
                {
                    if (auto drawResult =
                            fallbackBackend->drawBatch(batch, m_impl->m_fallbackWhiteTextureTag);
                        !drawResult.has_value())
                    {
                        frameSucceeded = false;
                        m_logger->warn("[RenderSystem] fallback draw failed: {}", drawResult.error().ToString());
                        break;
                    }
                }

                m_impl->m_stats.batchCount += static_cast<uint32_t>(fallbackBatches.size());
                m_impl->m_stats.vertexCount += static_cast<uint32_t>(m_impl->m_batchManager->getTotalVertexCount());
                m_impl->m_batchManager->clear();
                if (!frameSucceeded)
                {
                    break;
                }
            }

            if (frameSucceeded)
            {
                auto presentResult = fallbackBackend->endFrame();
                if (!render_detail::CommitRenderDirtyOnSuccess(*m_reg, windowEntity, presentResult))
                {
                    m_logger->warn("[RenderSystem] fallback present failed: {}", presentResult.error().ToString());
                }
            }
            continue;
        }

        // Execute collected render commands
        bool collectSucceeded = true;
        for (auto& item : m_impl->m_renderQueue)
        {
            if (auto collectResult = item.renderer->collectChecked(item.entity, item.context);
                !collectResult.has_value())
            {
                collectSucceeded = false;
                m_logger->warn("[RenderSystem] render collect failed: {}", collectResult.error().ToString());
                break;
            }
        }
        if (!collectSucceeded)
        {
            continue;
        }

        m_impl->m_batchManager->optimize();

        const auto& batches = m_impl->m_batchManager->getBatches();
        LogScalingSnapshotIfNeeded(windowState, *m_logger, *m_reg, windowEntity, windowComp, sdlWindow,
                       logicalWidth, logicalHeight, width,
                                   height, dpiScale, clearColor, static_cast<int>(batches.size()));
        if (m_impl->m_useFallback)
        {
            if (fallbackBackend != nullptr && fallbackBackend->beginFrame(clearColor))
            {
                bool frameSucceeded = true;
                for (const auto& batch : batches)
                {
                    if (!fallbackBackend->drawBatch(batch, m_impl->m_fallbackWhiteTextureTag))
                    {
                        frameSucceeded = false;
                        break;
                    }
                }
                if (frameSucceeded)
                {
                    [[maybe_unused]] const bool committed = render_detail::CommitRenderDirtyOnSuccess(
                        *m_reg, windowEntity, fallbackBackend->endFrame());
                }
            }
        }
        else
        {
            auto submitResult = m_impl->m_commandBuffer->execute(sdlWindow, width, height, dpiScale, clearColor, batches);
            if (!render_detail::CommitRenderDirtyOnSuccess(*m_reg, windowEntity, submitResult))
            {
                m_logger->warn("[RenderSystem] GPU frame submission failed: {}", submitResult.error().ToString());
            }
        }

        if (!batches.empty())
        {
            m_impl->m_stats.batchCount += static_cast<uint32_t>(batches.size());
            m_impl->m_stats.vertexCount += static_cast<uint32_t>(m_impl->m_batchManager->getTotalVertexCount());
        }
    }

}

// NOLINTNEXTLINE(readability-function-cognitive-complexity,readability-function-size)
void RenderSystem::collectRenderData(entt::entity entity, core::RenderContext& context)
{
    struct StackFrame
    {
        entt::entity entity;
        core::RenderContext context;
    };

    std::stack<StackFrame> stack;
    stack.push({.entity = entity, .context = context});

    while (!stack.empty())
    {
        auto [currentEntity, currentContext] = std::move(stack.top());
        stack.pop();

        if (!m_reg->any_of<components::VisibleTag>(currentEntity))
            continue;
        if (m_reg->any_of<components::SpacerTag>(currentEntity))
            continue;

        const auto& pos = m_reg->get<components::Position>(currentEntity);
        const auto& size = m_reg->get<components::Size>(currentEntity);
        const auto* alphaComp = m_reg->try_get<components::Alpha>(currentEntity);
        const auto* scaleComp = m_reg->try_get<components::Scale>(currentEntity);
        const auto* offsetComp = m_reg->try_get<components::RenderOffset>(currentEntity);

        float const globalAlpha = currentContext.alpha * (alphaComp != nullptr ? alphaComp->value : 1.0F);
        const Eigen::Vector2f position = detail::eigen::ToEigen(pos.value);
        const Eigen::Vector2f componentSize = detail::eigen::ToEigen(size.size);
        Eigen::Vector2f absolutePos = currentContext.position + position;
        Eigen::Vector2f finalSize = componentSize;

        if (offsetComp != nullptr)
        {
            absolutePos += detail::eigen::ToEigen(offsetComp->value);
        }

        if (scaleComp != nullptr)
        {
            const Eigen::Vector2f scale = detail::eigen::ToEigen(scaleComp->value);
            Eigen::Vector2f const scaleDiff = componentSize.cwiseProduct(Eigen::Vector2f::Ones() - scale);
            absolutePos += scaleDiff * 0.5F;
            finalSize = componentSize.cwiseProduct(scale);
        }

        Eigen::Vector2f contentOffset(0.0F, 0.0F);

        core::RenderContext entityContext = currentContext;
        entityContext.position = absolutePos;
        entityContext.size = finalSize;
        entityContext.alpha = globalAlpha;
        core::RenderContext childBaseContext = entityContext;

        const auto* scrollArea = m_reg->try_get<components::ScrollArea>(currentEntity);
        if (scrollArea != nullptr)
        {
            const Rect viewportRect = ui::utils::GetScrollViewportRect(m_reg->runtime(), ui::detail::ToPublic(currentEntity));
            SDL_Rect scissorRect{};
            scissorRect.x = static_cast<int>(viewportRect.x());
            scissorRect.y = static_cast<int>(viewportRect.y());
            scissorRect.w = static_cast<int>(std::max(0.0F, viewportRect.width()));
            scissorRect.h = static_cast<int>(std::max(0.0F, viewportRect.height()));

            childBaseContext.pushScissor(scissorRect);
            contentOffset = -detail::eigen::ToEigen(scrollArea->scrollOffset);
        }
        else if (m_reg->any_of<components::LayoutInfo>(currentEntity))
        {
            const SDL_Rect containerScissor{.x = static_cast<int>(absolutePos.x()),
                                            .y = static_cast<int>(absolutePos.y()),
                                            .w = static_cast<int>(std::max(0.0F, finalSize.x())),
                                            .h = static_cast<int>(std::max(0.0F, finalSize.y()))};
            childBaseContext.pushScissor(containerScissor);
        }

        // Determine Z-Order
        int32_t zOrder = 0;
        if (const auto* zOrderComp = m_reg->try_get<components::ZOrderIndex>(currentEntity))
        {
            zOrder = zOrderComp->value;
        }

        // Shift to positive range for unsigned sorting (int32_min -> 0)
        auto encodedZ = static_cast<uint64_t>(static_cast<int64_t>(zOrder) + 2147483648LL);

        for (auto& renderer : m_impl->m_renderers)
        {
            if (renderer->canHandle(currentEntity))
            {
                RenderSystemImpl::RenderItem item;
                item.entity = currentEntity;
                item.renderer = renderer.get();
                item.context = entityContext;

                // Build Key: High=Z, Low=Order
                item.sortKey = (encodedZ << 32) | (m_impl->m_submissionIndex & 0xFFFFFFFF);

                m_impl->m_renderQueue.push_back(item);
                m_impl->m_submissionIndex++;
            }
        }

        const auto* hierarchy = m_reg->try_get<components::Hierarchy>(currentEntity);
        if (hierarchy != nullptr && !hierarchy->children.empty())
        {
            for (auto childEntity : std::views::reverse(hierarchy->children))
            {
                core::RenderContext childContext = childBaseContext;
                childContext.position = absolutePos + contentOffset;
                stack.push({.entity = childEntity, .context = std::move(childContext)});
            }
        }
    }
}

}  // namespace ui::systems
