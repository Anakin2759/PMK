/**
 * ************************************************************************
 *
 * @file TextureAtlas.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-10
 * @version 0.1
 * @brief GPU 纹理图集管理器，用于字形缓存
 *
 * 采用 Shelf Bin Packing 算法管理纹理图集：
 * - 每次分配从当前 shelf（行）尝试，不够则开新行
 * - 支持将图集扩大一倍，最大为 4096x4096；扩容时迁移旧纹理并保留字形与 shelf 状态
 * - 每个字形记录其 UV 坐标和偏移量
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstring>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>
#include "common/GpuFailureInjection.hpp"
#include "common/GPUWrappers.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"

namespace ui::wrappers
{
using GPUTexturePtr = UniqueGPUTexture;
}

namespace ui::managers
{

/**
 * @brief 字形在图集中的位置信息
 */
struct AtlasGlyph
{
    // UV 坐标 (归一化)
    float u0 = 0.0F;
    float v0 = 0.0F;
    float u1 = 0.0F;
    float v1 = 0.0F;

    // 像素坐标（在图集中的位置）
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    // 渲染偏移量
    int32_t bearingX = 0;   // 水平偏移
    int32_t bearingY = 0;   // 垂直偏移（基线到字形顶部）
    float advanceX = 0.0F;  // 水平前进量
};

/**
 * @brief 纹理图集管理器 (Shelf Bin Packing)
 */
class TextureAtlas
{
   public:
    /**
     * @brief 构造纹理图集
     * @param generation 图集纹理及上传资源共同绑定的 GPU 设备代际
     * @param initialSize 初始尺寸（宽高相同）
     * @param padding 字形之间的内边距（像素）
     */
    explicit TextureAtlas(detail::GpuDeviceGenerationHandle generation, utils::Logger& logger, uint32_t initialSize = 2048,
                          uint32_t padding = 2)
        : m_generation(std::move(generation)), m_logger(&logger), m_size(initialSize), m_padding(padding)
    {
        if (!createTexture())
        {
            m_logger->error("[TextureAtlas] Failed to create initial texture");
        }
    }

    ~TextureAtlas() = default;

    // 禁止拷贝和移动
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
    TextureAtlas(TextureAtlas&&) = delete;
    TextureAtlas& operator=(TextureAtlas&&) = delete;

    /**
     * @brief 获取图集纹理
     */
    [[nodiscard]] SDL_GPUTexture* getTexture() const
    {
        return m_texture.get();
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_texture != nullptr && m_generation.Status() == detail::GpuDeviceGenerationStatus::ACTIVE;
    }

    /**
     * @brief 获取图集尺寸
     */
    [[nodiscard]] uint32_t getSize() const
    {
        return m_size;
    }

    /**
     * @brief 添加字形到图集
     * @param codepoint Unicode 码点（作为 key）
     * @param bitmap 字形位图数据（灰度，单通道）
     * @param width 位图宽度
     * @param height 位图高度
     * @param bearingX 水平偏移
     * @param bearingY 垂直偏移
     * @param advanceX 水平前进量
     * @return 字形信息，失败返回 nullopt
     * @note 新字形仅在上传命令提交成功后写入缓存；成功不表示 GPU 已执行完成。
     */
    std::optional<AtlasGlyph> addGlyph(uint32_t codepoint, const uint8_t* bitmap, int32_t width, int32_t height,
                                       int32_t bearingX, int32_t bearingY, float advanceX)
    {
        if (!isValid() || bitmap == nullptr || width <= 0 || height <= 0)
        {
            return std::nullopt;
        }

        // 检查是否已缓存
        auto iter = m_glyphMap.find(codepoint);
        if (iter != m_glyphMap.end())
        {
            return iter->second;
        }

        // 尝试分配空间
        auto shelvesBeforeAllocation = m_shelves;
        const uint32_t shelfYBeforeAllocation = m_currentShelfY;
        auto pos = allocate(width, height);
        if (!pos.has_value())
        {
            // 尝试扩展图集
            if (!expand())
            {
                m_logger->error("[TextureAtlas] Failed to expand atlas for codepoint {}",
                                                        codepoint);
                return std::nullopt;
            }
            shelvesBeforeAllocation = m_shelves;
            const uint32_t expandedShelfY = m_currentShelfY;
            pos = allocate(width, height);
            if (!pos.has_value())
            {
                m_logger->error("[TextureAtlas] Still cannot allocate after expansion");
                return std::nullopt;
            }
            if (!uploadBitmap(bitmap, pos->first, pos->second, width, height))
            {
                m_shelves = std::move(shelvesBeforeAllocation);
                m_currentShelfY = expandedShelfY;
                m_logger->error("[TextureAtlas] Failed to upload bitmap for codepoint {}",
                                                        codepoint);
                return std::nullopt;
            }
        }
        else if (!uploadBitmap(bitmap, pos->first, pos->second, width, height))
        {
            m_shelves = std::move(shelvesBeforeAllocation);
            m_currentShelfY = shelfYBeforeAllocation;
            m_logger->error("[TextureAtlas] Failed to upload bitmap for codepoint {}",
                                                    codepoint);
            return std::nullopt;
        }

        auto [xPos, yPos] = *pos;

        // 构造字形信息
        AtlasGlyph glyph;
        glyph.x = static_cast<int32_t>(xPos);
        glyph.y = static_cast<int32_t>(yPos);
        glyph.width = width;
        glyph.height = height;
        glyph.bearingX = bearingX;
        glyph.bearingY = bearingY;
        glyph.advanceX = advanceX;

        // 计算 UV 坐标（归一化）
        auto fSize = static_cast<float>(m_size);
        glyph.u0 = static_cast<float>(xPos) / fSize;
        glyph.v0 = static_cast<float>(yPos) / fSize;
        glyph.u1 = static_cast<float>(xPos + static_cast<uint32_t>(width)) / fSize;
        glyph.v1 = static_cast<float>(yPos + static_cast<uint32_t>(height)) / fSize;

        // 缓存
        m_glyphMap[codepoint] = glyph;
        return glyph;
    }

    /**
     * @brief 查询字形是否已缓存
     */
    [[nodiscard]] std::optional<AtlasGlyph> getGlyph(uint32_t codepoint) const
    {
        if (!isValid())
        {
            return std::nullopt;
        }
        auto iter = m_glyphMap.find(codepoint);
        if (iter != m_glyphMap.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    /**
     * @brief 清空字形缓存和 shelf 分配状态
     * @note 不清除 GPU 纹理中的既有像素；后续有效区域由新上传覆盖。
     */
    void clear()
    {
        m_glyphMap.clear();
        m_shelves.clear();
        m_currentShelfY = 0;
        m_logger->info("[TextureAtlas] Cleared all glyphs");
    }

    /**
     * @brief 获取统计信息
     */
    struct Stats
    {
        uint32_t atlasSize = 0;
        uint32_t glyphCount = 0;
        uint32_t shelfCount = 0;
        uint32_t usedPixels = 0;
        float utilization = 0.0F;
    };

    [[nodiscard]] Stats getStats() const
    {
        Stats stats;
        stats.atlasSize = m_size;
        stats.glyphCount = static_cast<uint32_t>(m_glyphMap.size());
        stats.shelfCount = static_cast<uint32_t>(m_shelves.size());

        uint32_t usedPixels = 0;
        for (const auto& [codepoint, glyph] : m_glyphMap)
        {
            usedPixels += static_cast<uint32_t>(glyph.width * glyph.height);
        }
        stats.usedPixels = usedPixels;

        uint32_t totalPixels = m_size * m_size;
        stats.utilization = totalPixels > 0 ? static_cast<float>(usedPixels) / static_cast<float>(totalPixels) : 0.0F;

        return stats;
    }

   private:
    /**
     * @brief Shelf 结构（一行）
     */
    struct Shelf
    {
        uint32_t y = 0;       // Shelf 起始 Y 坐标
        uint32_t height = 0;  // Shelf 高度
        uint32_t x = 0;       // 当前行的 X 游标
    };

    /**
     * @brief 创建 GPU 纹理
     */
    bool createTexture()
    {
        if (!m_generation || m_generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE || m_size == 0)
        {
            m_logger->error("[TextureAtlas] Cannot create texture with invalid device or size");
            return false;
        }

        SDL_GPUTextureCreateInfo textureInfo{};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;  // 单通道灰度
        textureInfo.width = m_size;
        textureInfo.height = m_size;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
        {
            return false;
        }
        auto texture =
            wrappers::MakeGpuResource<wrappers::GPUTexturePtr>(m_generation, SDL_CreateGPUTexture, &textureInfo);
        if (!texture)
        {
            m_logger->error("[TextureAtlas] Failed to create texture: {}", SDL_GetError());
            return false;
        }

        m_texture = std::move(texture);
        m_logger->info("[TextureAtlas] Created texture atlas {}x{}", m_size, m_size);
        return true;
    }

    /**
     * @brief 分配空间（Shelf Bin Packing）
     * @return 成功返回 (x, y) 坐标
     */
    std::optional<std::pair<uint32_t, uint32_t>> allocate(int32_t width, int32_t height)
    {
        if (width <= 0 || height <= 0)
        {
            return std::nullopt;
        }

        const auto unsignedWidth = static_cast<uint32_t>(width);
        const auto unsignedHeight = static_cast<uint32_t>(height);
        if (unsignedWidth > std::numeric_limits<uint32_t>::max() - m_padding ||
            unsignedHeight > std::numeric_limits<uint32_t>::max() - m_padding)
        {
            return std::nullopt;
        }

        uint32_t glyphWidth = unsignedWidth + m_padding;
        uint32_t glyphHeight = unsignedHeight + m_padding;

        // 尝试从现有 shelf 分配
        for (auto& shelf : m_shelves)
        {
            if (shelf.height >= glyphHeight && (shelf.x + glyphWidth) <= m_size)
            {
                uint32_t allocX = shelf.x;
                uint32_t allocY = shelf.y;
                shelf.x += glyphWidth;
                return std::make_pair(allocX, allocY);
            }
        }

        // 创建新 shelf
        if (m_currentShelfY + glyphHeight <= m_size)
        {
            Shelf newShelf;
            newShelf.y = m_currentShelfY;
            newShelf.height = glyphHeight;
            newShelf.x = glyphWidth;

            m_shelves.push_back(newShelf);
            m_currentShelfY += glyphHeight;

            return std::make_pair(0U, newShelf.y);
        }

        return std::nullopt;
    }

    /**
     * @brief 以事务方式将图集边长扩大一倍
     *
     * 将旧纹理内容复制到候选纹理；仅在复制命令提交成功后切换纹理和尺寸。
     * 已缓存字形及 shelf 布局保持不变，字形 UV 按新尺寸重新计算。
     *
     * @return 提交扩容复制命令成功时返回 true；失败时返回 false，并保留原图集状态
     * @note 成功表示复制命令已提交，不表示 GPU 已执行完成；本函数不会等待 GPU 空闲。
     */
    bool expand()
    {
        if (m_size >= 4096)
        {
            m_logger->warn("[TextureAtlas] Cannot expand beyond 4096x4096");
            return false;
        }

        const uint32_t oldSize = m_size;
        const uint32_t newSize = oldSize * 2;
        m_logger->info("[TextureAtlas] Expanding atlas from {}x{} to {}x{}", m_size, m_size,
                                               newSize, newSize);

        // 创建新纹理
        SDL_GPUTextureCreateInfo textureInfo{};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;
        textureInfo.width = newSize;
        textureInfo.height = newSize;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
        {
            return false;
        }
        auto newTexture =
            wrappers::MakeGpuResource<wrappers::GPUTexturePtr>(m_generation, SDL_CreateGPUTexture, &textureInfo);
        if (!newTexture)
        {
            m_logger->error("[TextureAtlas] Failed to create expanded texture: {}",
                                                    SDL_GetError());
            return false;
        }

        const auto activeDevice = m_generation.InvokeIfActive([](SDL_GPUDevice* device) { return device; });
        SDL_GPUDevice* const device = activeDevice.value_or(nullptr);
        if (device == nullptr)
        {
            return false;
        }

        SDL_GPUCommandBuffer* commandBuffer = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COMMAND_ACQUIRE))
        {
            commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        }
        if (commandBuffer == nullptr)
        {
            m_logger->error("[TextureAtlas] Failed to acquire expansion command buffer: {}",
                                                    SDL_GetError());
            return false;
        }

        SDL_GPUCopyPass* copyPass = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COPY_PASS_BEGIN))
        {
            copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        }
        if (copyPass == nullptr)
        {
            m_logger->error("[TextureAtlas] Failed to begin expansion copy pass: {}",
                                                    SDL_GetError());
            if (!SDL_CancelGPUCommandBuffer(commandBuffer))
            {
                m_logger->error("[TextureAtlas] Failed to cancel expansion command buffer: {}",
                                                        SDL_GetError());
            }
            return false;
        }

        const SDL_GPUTextureLocation source{
            .texture = m_texture.get(), .mip_level = 0, .layer = 0, .x = 0, .y = 0, .z = 0};
        const SDL_GPUTextureLocation destination{
            .texture = newTexture.get(), .mip_level = 0, .layer = 0, .x = 0, .y = 0, .z = 0};
        SDL_CopyGPUTextureToTexture(copyPass, &source, &destination, oldSize, oldSize, 1, false);
        SDL_EndGPUCopyPass(copyPass);

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::SUBMIT))
        {
            if (!SDL_CancelGPUCommandBuffer(commandBuffer))
            {
                m_logger->error("[TextureAtlas] Failed to cancel expansion command buffer: {}",
                                                        SDL_GetError());
            }
            return false;
        }
        if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
        {
            m_logger->error("[TextureAtlas] Failed to submit expansion copy: {}",
                                                    SDL_GetError());
            return false;
        }

        m_texture.swap(newTexture);
        m_size = newSize;

        const float atlasSize = static_cast<float>(newSize);
        for (auto& [codepoint, glyph] : m_glyphMap)
        {
            static_cast<void>(codepoint);
            glyph.u0 = static_cast<float>(glyph.x) / atlasSize;
            glyph.v0 = static_cast<float>(glyph.y) / atlasSize;
            glyph.u1 = static_cast<float>(glyph.x + glyph.width) / atlasSize;
            glyph.v1 = static_cast<float>(glyph.y + glyph.height) / atlasSize;
        }

        m_logger->info("[TextureAtlas] Migrated atlas content from {}x{} to {}x{}", oldSize,
                                               oldSize, newSize, newSize);
        return true;
    }

    /**
     * @brief 将单通道灰度位图上传到图集的 R8 纹理区域
     * @param bitmap 连续存放的 R8 位图数据
     * @param xPos 目标区域左上角的 X 像素坐标
     * @param yPos 目标区域左上角的 Y 像素坐标
     * @param width 位图宽度
     * @param height 位图高度
     * @return 上传命令提交成功时返回 true；参数、资源或提交失败时返回 false
     * @note 成功仅表示命令缓冲已提交，不表示 GPU 已执行完成；本函数不会阻塞等待 GPU。
     */
    bool uploadBitmap(const uint8_t* bitmap, uint32_t xPos, uint32_t yPos, int32_t width, int32_t height)
    {
        const auto activeDevice = m_generation.InvokeIfActive([](SDL_GPUDevice* device) { return device; });
        SDL_GPUDevice* const device = activeDevice.value_or(nullptr);
        if (device == nullptr || m_texture == nullptr || bitmap == nullptr || width <= 0 || height <= 0)
        {
            return false;
        }

        const auto uploadWidth = static_cast<uint32_t>(width);
        const auto uploadHeight = static_cast<uint32_t>(height);
        if (xPos > m_size || yPos > m_size || uploadWidth > m_size - xPos || uploadHeight > m_size - yPos ||
            uploadWidth > std::numeric_limits<uint32_t>::max() / uploadHeight)
        {
            return false;
        }

        const uint32_t uploadSize = uploadWidth * uploadHeight;
        const SDL_GPUTransferBufferCreateInfo transferInfo{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = uploadSize, .props = 0};
        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::TRANSFER_CREATE))
        {
            return false;
        }
        auto transferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
            m_generation, SDL_CreateGPUTransferBuffer, &transferInfo);
        if (!transferBuffer)
        {
            m_logger->error("[TextureAtlas] Failed to create upload transfer buffer: {}",
                                                    SDL_GetError());
            return false;
        }

        void* mappedData = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::MAP))
        {
            mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
        }
        if (mappedData == nullptr)
        {
            m_logger->error("[TextureAtlas] Failed to map upload transfer buffer: {}",
                                                    SDL_GetError());
            return false;
        }
        std::memcpy(mappedData, bitmap, uploadSize);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());

        SDL_GPUCommandBuffer* commandBuffer = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COMMAND_ACQUIRE))
        {
            commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        }
        if (commandBuffer == nullptr)
        {
            m_logger->error("[TextureAtlas] Failed to acquire upload command buffer: {}",
                                                    SDL_GetError());
            return false;
        }

        SDL_GPUCopyPass* copyPass = nullptr;
        if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COPY_PASS_BEGIN))
        {
            copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        }
        if (copyPass == nullptr)
        {
            m_logger->error("[TextureAtlas] Failed to begin upload copy pass: {}",
                                                    SDL_GetError());
            if (!SDL_CancelGPUCommandBuffer(commandBuffer))
            {
                m_logger->error("[TextureAtlas] Failed to cancel upload command buffer: {}",
                                                        SDL_GetError());
            }
            return false;
        }

        const SDL_GPUTextureTransferInfo source{.transfer_buffer = transferBuffer.get(),
                                                .offset = 0,
                                                .pixels_per_row = uploadWidth,
                                                .rows_per_layer = uploadHeight};
        const SDL_GPUTextureRegion destination{.texture = m_texture.get(),
                                               .mip_level = 0,
                                               .layer = 0,
                                               .x = xPos,
                                               .y = yPos,
                                               .z = 0,
                                               .w = uploadWidth,
                                               .h = uploadHeight,
                                               .d = 1};
        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);

        if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::SUBMIT))
        {
            if (!SDL_CancelGPUCommandBuffer(commandBuffer))
            {
                m_logger->error("[TextureAtlas] Failed to cancel upload command buffer: {}",
                                                        SDL_GetError());
            }
            return false;
        }
        if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
        {
            m_logger->error("[TextureAtlas] Failed to submit bitmap upload: {}",
                                                    SDL_GetError());
            return false;
        }
        return true;
    }

    detail::GpuDeviceGenerationHandle m_generation;
    utils::Logger* m_logger;
    wrappers::GPUTexturePtr m_texture;

    uint32_t m_size = 2048;
    uint32_t m_padding = 2;

    std::vector<Shelf> m_shelves;
    uint32_t m_currentShelfY = 0;

    std::unordered_map<uint32_t, AtlasGlyph> m_glyphMap;
};

}  // namespace ui::managers
