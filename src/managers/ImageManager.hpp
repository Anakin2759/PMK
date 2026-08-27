/**
 * ************************************************************************
 *
 * @file ImageManager.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-18
 * @version 0.1
 * @brief 图像文件管理器 - 从文件加载 bmp/png/jpeg 并上传到 GPU
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL3/SDL_gpu.h>

#include "common/GPUWrappers.hpp"
#include "ui/Result.hpp"

namespace ui::utils
{
class Logger;
}

namespace ui::managers
{

class DeviceManager;

/**
 * @brief 图像纹理管理器（多实例，按渲染上下文持有）
 *
 * - 持有一个 DeviceManager* 句柄，所有 GPU owner 绑定创建时的共享设备代际
 * - 缓存 `path -> UniqueGPUTexture`，设备代际变化时先清空旧代缓存
 * - 析构时自动释放所有缓存纹理，无需调用方显式 releaseAll
 * - 支持 bmp（SDL_LoadBMP）和 png/jpeg（stb_image）格式
 */
class ImageManager
{
   public:
    /**
     * @brief CPU fallback 使用的 RGBA8 像素缓存。
     */
    struct PixelBuffer
    {
        std::vector<std::uint8_t> rgba;
        int width = 0;
        int height = 0;
    };

    explicit ImageManager(DeviceManager* deviceManager, utils::Logger& logger)
        : m_deviceManager(deviceManager), m_logger(&logger)
    {
    }
    ~ImageManager() noexcept;

    ImageManager(const ImageManager&) = delete;
    ImageManager& operator=(const ImageManager&) = delete;
    ImageManager(ImageManager&&) = delete;
    ImageManager& operator=(ImageManager&&) = delete;

    /**
     * @brief 加载图像文件并上传到 GPU。
     *
     * 返回的纹理生命周期由本 ImageManager 持有，禁止外部调用 SDL_ReleaseGPUTexture。
     *
     * @param path  文件路径（bmp / png / jpeg）
     * @return Result<SDL_GPUTexture*> 成功时持有非空纹理指针；失败时携带 UiErrc。
     */
    [[nodiscard]] ui::Result<SDL_GPUTexture*> loadTexture(const std::string& path);

    /**
     * @brief 解码图像为 RGBA8 像素，结果按路径缓存且不依赖 GPU 设备。
     * @return 成功时返回由本 ImageManager 持有的只读缓存指针。
     */
    [[nodiscard]] ui::Result<const PixelBuffer*> loadPixels(const std::string& path);

    /// 释放所有 CPU RGBA 像素缓存。
    void clearPixels() noexcept;

    [[nodiscard]] std::size_t pixelCacheEntryCount() const noexcept
    {
        return m_pixelCache.size();
    }

    [[nodiscard]] std::size_t pixelCacheByteSize() const noexcept
    {
        std::size_t bytes = 0;
        for (const auto& [path, pixels] : m_pixelCache)
        {
            static_cast<void>(path);
            if (pixels != nullptr)
            {
                bytes += pixels->rgba.size();
            }
        }
        return bytes;
    }

    /**
     * @brief 主动释放所有已缓存纹理（析构会自动调用，正常路径下无需手动调用）。
     */
    void releaseAll();

   private:
    /**
    * @brief 按扩展名通过 SDL_LoadBMP 或 stb_image 解码为 RGBA8。
     */
    [[nodiscard]] static ui::Result<PixelBuffer> decodePixels(utils::Logger& logger, const std::string& path);

    /**
     * @brief 在指定设备代际创建纹理并提交 RGBA 像素上传
     * @return 上传命令提交成功时返回同代际纹理 owner；任一阶段失败时返回空 owner
     * @note 成功仅表示非阻塞提交已被 SDL 接受，不表示 GPU 已执行完成。
     */
    [[nodiscard]] static wrappers::UniqueGPUTexture uploadToGpu(
        utils::Logger& logger, const detail::GpuDeviceGenerationHandle& generation, const std::uint8_t* pixels,
        std::uint32_t width, std::uint32_t height);

    DeviceManager* m_deviceManager = nullptr;
    utils::Logger* m_logger = nullptr;
    // 路径 -> CPU RGBA8 像素；不随 GPU 设备代际变化而失效。
    std::unordered_map<std::string, std::unique_ptr<PixelBuffer>> m_pixelCache;
    // 路径 -> 已上传的 GPU 纹理 owner；公开调用方仅借用 `.get()`。
    std::unordered_map<std::string, wrappers::UniqueGPUTexture> m_cache;
    std::uint64_t m_cacheGenerationId = 0;
};

}  // namespace ui::managers
