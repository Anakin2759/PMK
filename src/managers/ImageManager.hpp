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

#include <string>
#include <unordered_map>
#include <SDL3/SDL_gpu.h>

#include "common/GPUWrappers.hpp"
#include "ui/Result.hpp"

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
    explicit ImageManager(DeviceManager* deviceManager) : m_deviceManager(deviceManager)
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
     * @brief 主动释放所有已缓存纹理（析构会自动调用，正常路径下无需手动调用）。
     */
    void releaseAll();

   private:
    /**
     * @brief 通过 stb_image 加载 png/jpeg 并上传
     */
    wrappers::UniqueGPUTexture loadWithStb(const std::string& path,
                                           const detail::GpuDeviceGenerationHandle& generation);

    /**
     * @brief 通过 SDL_LoadBMP 加载 bmp 并上传
     */
    wrappers::UniqueGPUTexture loadWithSdlBmp(const std::string& path,
                                              const detail::GpuDeviceGenerationHandle& generation);

    /**
     * @brief 在指定设备代际创建纹理并提交 RGBA 像素上传
     * @return 上传命令提交成功时返回同代际纹理 owner；任一阶段失败时返回空 owner
     * @note 成功仅表示非阻塞提交已被 SDL 接受，不表示 GPU 已执行完成。
     */
    wrappers::UniqueGPUTexture uploadToGpu(const detail::GpuDeviceGenerationHandle& generation,
                                           const unsigned char* pixels, uint32_t width, uint32_t height);

    DeviceManager* m_deviceManager = nullptr;
    // 路径 -> 已上传的 GPU 纹理 owner；公开调用方仅借用 `.get()`。
    std::unordered_map<std::string, wrappers::UniqueGPUTexture> m_cache;
    std::uint64_t m_cacheGenerationId = 0;
};

}  // namespace ui::managers
