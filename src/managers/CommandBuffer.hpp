/**
 * ************************************************************************
 *
 * @file CommandBuffer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 命令缓冲区包装器 - 封装SDL GPU命令和资源管理
    池化
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <SDL3/SDL_gpu.h>
#include "managers/DeviceManager.hpp"
#include "managers/PipelineCache.hpp"
#include "common/RenderTypes.hpp"
#include "common/GPUWrappers.hpp"
#include "common/GpuFailureInjection.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"

namespace ui::managers
{

/**
 * @brief 命令缓冲区包装器
 *
 * 负责：
 * 1. 封装SDL GPU命令的提交、渲染通道等操作
 * 2. 管理与当前设备代际绑定的顶点、索引和传输缓冲区
 * 3. 代际变化时清理旧资源，并以候选资源保证扩容失败不会覆盖可用缓冲区
 */
class CommandBuffer
{
   public:
    CommandBuffer(DeviceManager& deviceManager, PipelineCache& pipelineCache)
        : m_deviceManager(deviceManager), m_pipelineCache(pipelineCache)
    {
    }

    ~CommandBuffer()
    {
        cleanup();
    }
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    /**
     * @brief 执行渲染批次
     * @param batches 渲染批次列表
     * @note SDL_SubmitGPUCommandBuffer 是命令缓冲消费边界：调用前的失败可以取消，
     * 调用后无论返回值如何均不再取消；仅提交成功后提交候选扩容并推进帧索引。
     */
    // NOLINTNEXTLINE(readability-function-cognitive-complexity, bugprone-easily-swappable-parameters) --
    // 失败回滚必须保持帧阶段串行可见。
    void execute(SDL_Window* window, int width, int height, float dpiScale, const SDL_FColor& clearColor,
                 const std::pmr::vector<render::RenderBatch>& batches)
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        const auto generation = m_deviceManager.getGeneration();
        if (device == nullptr || !generation || generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE)
        {
            return;
        }
        if (m_generationId != 0 && m_generationId != generation.Id())
        {
            cleanup();
        }

        auto [totalVertexCount, totalIndexCount] = calculateBatchTotals(batches);
        const bool hasGeometry = totalVertexCount > 0 && totalIndexCount > 0;

        uint32_t totalVertexSize = totalVertexCount * sizeof(render::Vertex);
        uint32_t totalIndexSize = totalIndexCount * sizeof(uint16_t);

        // 获取当前帧资源，避免对 std::array 使用运行时下标触发 clang-tidy 告警
        FrameResource& currentFrame = currentFrameResource();

        // 确保缓冲区足够大
        if (hasGeometry && !resizeBuffers(generation, currentFrame, totalVertexSize, totalIndexSize))
        {
            ui::UiRuntime::current().logger().error("Failed to resize buffers.");
            return;
        }

        if (hasGeometry && !uploadToTransferBuffer(device, batches, totalVertexSize))
        {
            ui::UiRuntime::current().logger().error("Failed to map transfer buffer.");
            rollbackBufferResize(currentFrame);
            return;
        }

        SDL_GPUCommandBuffer* cmdBuf = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COMMAND_ACQUIRE))
        {
            cmdBuf = SDL_AcquireGPUCommandBuffer(device);
        }
        if (cmdBuf == nullptr)
        {
            rollbackBufferResize(currentFrame);
            return;
        }

        SDL_GPUTexture* swapchainTexture = nullptr;
        uint32_t swapchainWidth = 0;
        uint32_t swapchainHeight = 0;
        const bool acquiredSwapchain =
            !detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::SWAPCHAIN_ACQUIRE) &&
            SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, window, &swapchainTexture, &swapchainWidth, &swapchainHeight);
        if (!acquiredSwapchain)
        {
            ui::UiRuntime::current().logger().warn("Swapchain texture not ready yet.");
            if (!SDL_CancelGPUCommandBuffer(cmdBuf))
            {
                ui::UiRuntime::current().logger().error("Failed to cancel command buffer: {}", SDL_GetError());
            }
            rollbackBufferResize(currentFrame);
            return;
        }

        if (swapchainTexture == nullptr)
        {
            if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::SUBMIT))
            {
                if (!SDL_CancelGPUCommandBuffer(cmdBuf))
                {
                    ui::UiRuntime::current().logger().error("Failed to cancel command buffer: {}", SDL_GetError());
                }
                rollbackBufferResize(currentFrame);
                return;
            }
            if (!SDL_SubmitGPUCommandBuffer(cmdBuf))
            {
                ui::UiRuntime::current().logger().error("Failed to submit command buffer: {}", SDL_GetError());
                rollbackBufferResize(currentFrame);
            }
            else
            {
                commitBufferResize();
            }
            return;
        }

        const int renderWidth = swapchainWidth > 0 ? static_cast<int>(swapchainWidth) : width;
        const int renderHeight = swapchainHeight > 0 ? static_cast<int>(swapchainHeight) : height;
        if (renderWidth != width || renderHeight != height)
        {
            ui::UiRuntime::current().logger().info("[Scaling][CommandBuffer] requested=({}, {}) swapchain=({}, {})",
                                                   width, height, renderWidth, renderHeight);
        }

        if (hasGeometry)
        {
            if (!recordCopyPass(cmdBuf, currentFrame, totalVertexSize, totalIndexSize))
            {
                if (!SDL_CancelGPUCommandBuffer(cmdBuf))
                {
                    ui::UiRuntime::current().logger().error("Failed to cancel command buffer: {}", SDL_GetError());
                }
                rollbackBufferResize(currentFrame);
                return;
            }
        }

        if (!recordRenderPass(cmdBuf, swapchainTexture, renderWidth, renderHeight, dpiScale, clearColor, currentFrame,
                              batches))
        {
            if (!SDL_CancelGPUCommandBuffer(cmdBuf))
            {
                ui::UiRuntime::current().logger().error("Failed to cancel command buffer: {}", SDL_GetError());
            }
            rollbackBufferResize(currentFrame);
            return;
        }

        // 提交命令缓冲区
        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::SUBMIT))
        {
            if (!SDL_CancelGPUCommandBuffer(cmdBuf))
            {
                ui::UiRuntime::current().logger().error("Failed to cancel command buffer: {}", SDL_GetError());
            }
            rollbackBufferResize(currentFrame);
            return;
        }
        if (!SDL_SubmitGPUCommandBuffer(cmdBuf))
        {
            ui::UiRuntime::current().logger().error("Failed to submit command buffer: {}", SDL_GetError());
            rollbackBufferResize(currentFrame);
            return;
        }

        commitBufferResize();
        // 切换到下一帧
        m_frameIndex++;
    }

    /**
     * @brief 清理资源
     */
    void cleanup()
    {
        // RAII handles destruction
        for (auto& frame : m_frameResources)
        {
            frame.vertexBuffer.reset();
            frame.indexBuffer.reset();
            frame.vertexBufferSize = 0;
            frame.indexBufferSize = 0;
        }
        m_transferBuffer.reset();
        m_transferBufferSize = 0;
        commitBufferResize();
        m_generationId = 0;
        m_pendingGenerationId = 0;
    }

   private:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct FrameResource
    {
        wrappers::UniqueGPUBuffer vertexBuffer;
        wrappers::UniqueGPUBuffer indexBuffer;
        uint32_t vertexBufferSize = 0;
        uint32_t indexBufferSize = 0;
    };

    FrameResource& currentFrameResource()
    {
        return (m_frameIndex % MAX_FRAMES_IN_FLIGHT) == 0U ? m_frameResources.front() : m_frameResources.back();
    }

    [[nodiscard]] std::pair<uint32_t, uint32_t> calculateBatchTotals(
        const std::pmr::vector<render::RenderBatch>& batches) const
    {
        uint32_t vertices = 0;
        uint32_t indices = 0;
        for (const auto& batch : batches)
        {
            vertices += static_cast<uint32_t>(batch.vertices.size());
            indices += static_cast<uint32_t>(batch.indices.size());
        }
        return {vertices, indices};
    }

    bool uploadToTransferBuffer(SDL_GPUDevice* device, const std::pmr::vector<render::RenderBatch>& batches,
                                uint32_t totalVertexSize)
    {
        void* mapData = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::MAP))
        {
            mapData = SDL_MapGPUTransferBuffer(device, m_transferBuffer.get(), true);
        }
        if (mapData == nullptr)
            return false;

        auto* ptr = static_cast<uint8_t*>(mapData);
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = totalVertexSize;

        for (const auto& batch : batches)
        {
            if (batch.vertices.empty())
                continue;
            auto vSize = static_cast<uint32_t>(batch.vertices.size() * sizeof(render::Vertex));
            SDL_memcpy(ptr + vertexOffset, batch.vertices.data(), vSize);  // NOLINT
            vertexOffset += vSize;
        }

        for (const auto& batch : batches)
        {
            if (batch.indices.empty())
                continue;
            auto iSize = static_cast<uint32_t>(batch.indices.size() * sizeof(uint16_t));
            SDL_memcpy(ptr + indexOffset, batch.indices.data(), iSize);
            indexOffset += iSize;
        }

        SDL_UnmapGPUTransferBuffer(device, m_transferBuffer.get());
        return true;
    }

    bool recordCopyPass(SDL_GPUCommandBuffer* cmdBuf, const FrameResource& currentFrame, uint32_t totalVertexSize,
                        uint32_t totalIndexSize)
    {
        SDL_GPUCopyPass* copyPass = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COPY_PASS_BEGIN))
        {
            copyPass = SDL_BeginGPUCopyPass(cmdBuf);
        }
        if (copyPass == nullptr)
        {
            return false;
        }

        SDL_GPUTransferBufferLocation srcLoc = {};
        srcLoc.transfer_buffer = m_transferBuffer.get();
        srcLoc.offset = 0;

        SDL_GPUBufferRegion dstReg = {};
        dstReg.buffer = currentFrame.vertexBuffer.get();
        dstReg.offset = 0;
        dstReg.size = totalVertexSize;

        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false);

        srcLoc.offset = totalVertexSize;
        dstReg.buffer = currentFrame.indexBuffer.get();
        dstReg.size = totalIndexSize;
        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false);

        SDL_EndGPUCopyPass(copyPass);
        return true;
    }

    bool recordRenderPass(SDL_GPUCommandBuffer* cmdBuf, SDL_GPUTexture* swapchainTexture, int width, int height,
                          float dpiScale, const SDL_FColor& clearColor, const FrameResource& currentFrame,
                          const std::pmr::vector<render::RenderBatch>& batches)
    {
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTexture;
        colorTarget.clear_color = clearColor;
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;
        colorTarget.cycle = true;

        SDL_GPURenderPass* renderPass = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RENDER_PASS_BEGIN))
        {
            renderPass = SDL_BeginGPURenderPass(cmdBuf, &colorTarget, 1, nullptr);
        }

        if (renderPass == nullptr)
        {
            return false;
        }

        if (batches.empty())
        {
            SDL_EndGPURenderPass(renderPass);
            return true;
        }

        SDL_BindGPUGraphicsPipeline(renderPass, m_pipelineCache.getPipeline());

        SDL_GPUViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.w = static_cast<float>(width);
        viewport.h = static_cast<float>(height);
        viewport.min_depth = 0.0F;
        viewport.max_depth = 1.0F;
        SDL_SetGPUViewport(renderPass, &viewport);

        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = currentFrame.vertexBuffer.get();
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = currentFrame.indexBuffer.get();
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        uint32_t currentVertexOffset = 0;
        uint32_t currentIndexOffset = 0;
        float const effectiveDpiScale = dpiScale > 0.0F ? dpiScale : 1.0F;

        auto scaleScissor = [effectiveDpiScale](SDL_Rect rect)
        {
            return SDL_Rect{.x = static_cast<int>(std::floor(static_cast<float>(rect.x) * effectiveDpiScale)),
                            .y = static_cast<int>(std::floor(static_cast<float>(rect.y) * effectiveDpiScale)),
                            .w = static_cast<int>(std::ceil(static_cast<float>(rect.w) * effectiveDpiScale)),
                            .h = static_cast<int>(std::ceil(static_cast<float>(rect.h) * effectiveDpiScale))};
        };

        for (const auto& batch : batches)
        {
            if (batch.vertices.empty() || batch.indices.empty())
                continue;

            if (batch.scissorRect.has_value())
            {
                SDL_Rect scaledScissor = scaleScissor(batch.scissorRect.value());
                SDL_SetGPUScissor(renderPass, &scaledScissor);
            }
            else
            {
                SDL_Rect fullViewport = {0, 0, width, height};
                SDL_SetGPUScissor(renderPass, &fullViewport);
            }

            if (batch.texture != nullptr)
            {
                SDL_GPUTextureSamplerBinding texSamplerBinding = {};
                texSamplerBinding.texture = static_cast<SDL_GPUTexture*>(batch.texture);
                texSamplerBinding.sampler = m_pipelineCache.getSampler();
                SDL_BindGPUFragmentSamplers(renderPass, 0, &texSamplerBinding, 1);
            }

            SDL_PushGPUVertexUniformData(cmdBuf, 0, &batch.pushConstants, sizeof(render::UiPushConstants));
            SDL_PushGPUFragmentUniformData(cmdBuf, 0, &batch.pushConstants, sizeof(render::UiPushConstants));

            SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<uint32_t>(batch.indices.size()), 1, currentIndexOffset,
                                         static_cast<int32_t>(currentVertexOffset), 0);

            currentVertexOffset += static_cast<uint32_t>(batch.vertices.size());
            currentIndexOffset += static_cast<uint32_t>(batch.indices.size());
        }

        SDL_EndGPURenderPass(renderPass);
        return true;
    }

    bool resizeBuffers(const detail::GpuDeviceGenerationHandle& generation, FrameResource& frame, uint32_t vSize,
                       uint32_t iSize)
    {
        // 传输缓冲区 (Shared across frames, handled by cycle=true)
        const uint32_t neededTransfer = vSize + iSize;
        uint32_t candidateTransferSize = m_transferBufferSize;
        uint32_t candidateVertexSize = frame.vertexBufferSize;
        uint32_t candidateIndexSize = frame.indexBufferSize;
        wrappers::UniqueGPUTransferBuffer candidateTransferBuffer;
        wrappers::UniqueGPUBuffer candidateVertexBuffer;
        wrappers::UniqueGPUBuffer candidateIndexBuffer;

        if (m_transferBufferSize < neededTransfer)
        {
            candidateTransferSize =
                neededTransfer > m_transferBufferSize * 2 ? neededTransfer : m_transferBufferSize * 2;
            if (candidateTransferSize < neededTransfer)
                candidateTransferSize = neededTransfer;

            SDL_GPUTransferBufferCreateInfo tInfo = {};
            tInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            tInfo.size = candidateTransferSize;

            if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::TRANSFER_CREATE))
                return false;
            candidateTransferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
                generation, SDL_CreateGPUTransferBuffer, &tInfo);
            if (!candidateTransferBuffer)
                return false;
        }

        // 顶点缓冲区 (Per Frame)
        if (frame.vertexBufferSize < vSize)
        {
            candidateVertexSize = vSize > frame.vertexBufferSize * 2 ? vSize : frame.vertexBufferSize * 2;

            SDL_GPUBufferCreateInfo bInfo = {};
            bInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            bInfo.size = candidateVertexSize;
            if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
                return false;
            candidateVertexBuffer =
                wrappers::MakeGpuResource<wrappers::UniqueGPUBuffer>(generation, SDL_CreateGPUBuffer, &bInfo);
            if (!candidateVertexBuffer)
                return false;
        }

        // 索引缓冲区 (Per Frame)
        if (frame.indexBufferSize < iSize)
        {
            candidateIndexSize = iSize > frame.indexBufferSize * 2 ? iSize : frame.indexBufferSize * 2;

            SDL_GPUBufferCreateInfo bInfo = {};
            bInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            bInfo.size = candidateIndexSize;
            if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
                return false;
            candidateIndexBuffer =
                wrappers::MakeGpuResource<wrappers::UniqueGPUBuffer>(generation, SDL_CreateGPUBuffer, &bInfo);
            if (!candidateIndexBuffer)
                return false;
        }

        if (candidateTransferBuffer)
        {
            m_previousTransferBuffer = std::move(m_transferBuffer);
            m_previousTransferBufferSize = m_transferBufferSize;
            m_transferBufferResized = true;
            m_transferBuffer = std::move(candidateTransferBuffer);
            m_transferBufferSize = candidateTransferSize;
        }
        if (candidateVertexBuffer)
        {
            m_previousVertexBuffer = std::move(frame.vertexBuffer);
            m_previousVertexBufferSize = frame.vertexBufferSize;
            m_vertexBufferResized = true;
            frame.vertexBuffer = std::move(candidateVertexBuffer);
            frame.vertexBufferSize = candidateVertexSize;
        }
        if (candidateIndexBuffer)
        {
            m_previousIndexBuffer = std::move(frame.indexBuffer);
            m_previousIndexBufferSize = frame.indexBufferSize;
            m_indexBufferResized = true;
            frame.indexBuffer = std::move(candidateIndexBuffer);
            frame.indexBufferSize = candidateIndexSize;
        }
        m_pendingGenerationId = generation.Id();
        return true;
    }

    void rollbackBufferResize(FrameResource& frame)
    {
        if (m_transferBufferResized)
        {
            m_transferBuffer = std::move(m_previousTransferBuffer);
            m_transferBufferSize = m_previousTransferBufferSize;
        }
        if (m_vertexBufferResized)
        {
            frame.vertexBuffer = std::move(m_previousVertexBuffer);
            frame.vertexBufferSize = m_previousVertexBufferSize;
        }
        if (m_indexBufferResized)
        {
            frame.indexBuffer = std::move(m_previousIndexBuffer);
            frame.indexBufferSize = m_previousIndexBufferSize;
        }
        clearBufferResizeState();
    }

    void commitBufferResize()
    {
        m_previousTransferBuffer.reset();
        m_previousVertexBuffer.reset();
        m_previousIndexBuffer.reset();
        if (m_pendingGenerationId != 0)
        {
            m_generationId = m_pendingGenerationId;
        }
        clearBufferResizeState();
    }

    void clearBufferResizeState() noexcept
    {
        m_previousTransferBufferSize = 0;
        m_previousVertexBufferSize = 0;
        m_previousIndexBufferSize = 0;
        m_transferBufferResized = false;
        m_vertexBufferResized = false;
        m_indexBufferResized = false;
        m_pendingGenerationId = 0;
    }

    DeviceManager& m_deviceManager;
    PipelineCache& m_pipelineCache;

    // 帧资源池
    std::array<FrameResource, MAX_FRAMES_IN_FLIGHT> m_frameResources;
    uint32_t m_frameIndex = 0;

    wrappers::UniqueGPUTransferBuffer m_transferBuffer;
    uint32_t m_transferBufferSize = 0;
    wrappers::UniqueGPUTransferBuffer m_previousTransferBuffer;
    wrappers::UniqueGPUBuffer m_previousVertexBuffer;
    wrappers::UniqueGPUBuffer m_previousIndexBuffer;
    uint32_t m_previousTransferBufferSize = 0;
    uint32_t m_previousVertexBufferSize = 0;
    uint32_t m_previousIndexBufferSize = 0;
    bool m_transferBufferResized = false;
    bool m_vertexBufferResized = false;
    bool m_indexBufferResized = false;
    std::uint64_t m_generationId = 0;
    std::uint64_t m_pendingGenerationId = 0;
};

}  // namespace ui::managers
