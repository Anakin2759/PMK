#include "ui/Result.hpp"
#include <string>
#include "utils/Logger.hpp"
#include "ui/ErrorCodes.hpp"
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <exception>
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_stdinc.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ImageManager.hpp"
#include "DeviceManager.hpp"

#include <SDL3/SDL_gpu.h>

namespace ui::managers
{
namespace
{
void WriteStderr(const char* text) noexcept
{
    if (text == nullptr)
    {
        return;
    }

    const auto textSize = std::strlen(text);
    if (std::fwrite(text, 1U, textSize, stderr) != textSize)
    {
        std::clearerr(stderr);
    }
}
} // namespace

ImageManager::~ImageManager() noexcept
{
    try
    {
        releaseAll();
    }
    catch (const std::exception& exception)
    {
        WriteStderr("[ImageManager] destructor cleanup failed: ");
        WriteStderr(exception.what());
        WriteStderr("\n");
    }
    catch (...)
    {
        WriteStderr("[ImageManager] destructor cleanup failed with unknown exception\n");
    }
}

ui::Result<SDL_GPUTexture*> ImageManager::loadTexture(const std::string& path)
{
    if (path.empty())
    {
        UiRuntime::current().logger().error("[ImageManager] loadTexture: empty path");
        return ui::Err(ui::UiErrc::INVALID_ARGUMENT, "empty path");
    }

    SDL_GPUDevice* device = (m_deviceManager != nullptr) ? m_deviceManager->getDevice() : nullptr;
    if (device == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] loadTexture: device is null, path={}", path);
        return ui::Err(ui::UiErrc::DEVICE_UNAVAILABLE, path);
    }

    // 缓存命中
    if (auto textureIt = m_cache.find(path); textureIt != m_cache.end())
    {
        return textureIt->second.get();
    }

    wrappers::UniqueGPUTexture texture;

    // 按扩展名分派加载器
    const auto dot = path.rfind('.');
    if (dot != std::string::npos)
    {
        std::string ext = path.substr(dot + 1);
        for (auto& character : ext)
        {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        if (ext == "bmp")
        {
            texture = loadWithSdlBmp(path, device);
        }
        else
        {
            texture = loadWithStb(path, device);
        }
    }
    else
    {
        texture = loadWithStb(path, device);
    }

    if (texture != nullptr)
    {
        auto* rawTexture = texture.get();
        auto [textureIt, inserted] = m_cache.try_emplace(path, std::move(texture));
        UiRuntime::current().logger().info("[ImageManager] Loaded texture: {}", path);
        return inserted ? rawTexture : textureIt->second.get();
    }

    UiRuntime::current().logger().error("[ImageManager] Failed to load texture: {}", path);
    return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
}

void ImageManager::releaseAll()
{
    m_cache.clear();
}

wrappers::UniqueGPUTexture ImageManager::loadWithStb(const std::string& path, SDL_GPUDevice* device)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    // 强制加载为 RGBA 4 通道
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] stbi_load failed: {} — {}", path, stbi_failure_reason());
        return {};
    }

    auto texture = uploadToGpu(device, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    stbi_image_free(pixels);
    return texture;
}

wrappers::UniqueGPUTexture ImageManager::loadWithSdlBmp(const std::string& path, SDL_GPUDevice* device)
{
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (surface == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] SDL_LoadBMP failed: {} — {}", path, SDL_GetError());
        return {};
    }

    // 转换为 RGBA8
    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(surface);

    if (converted == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] SDL_ConvertSurface failed: {}", SDL_GetError());
        return {};
    }

    auto texture = uploadToGpu(device,
                               static_cast<const unsigned char*>(converted->pixels),
                               static_cast<uint32_t>(converted->w),
                               static_cast<uint32_t>(converted->h));
    SDL_DestroySurface(converted);
    return texture;
}

wrappers::UniqueGPUTexture
    ImageManager::uploadToGpu(SDL_GPUDevice* device, const unsigned char* pixels, uint32_t width, uint32_t height)
{
    // 创建 GPU 纹理
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = width;
    texInfo.height = height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    auto texture = wrappers::MakeGpuResource<wrappers::UniqueGPUTexture>(device, SDL_CreateGPUTexture, &texInfo);
    if (texture == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] SDL_CreateGPUTexture failed: {}", SDL_GetError());
        return {};
    }

    const uint32_t dataSize = width * height * 4U;

    // 创建传输缓冲区
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;

    auto transferBuffer =
        wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(device, SDL_CreateGPUTransferBuffer, &transferInfo);
    if (transferBuffer == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] SDL_CreateGPUTransferBuffer failed: {}", SDL_GetError());
        return {};
    }

    // 映射并拷贝像素
    void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
    if (mapped == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] SDL_MapGPUTransferBuffer failed: {}", SDL_GetError());
        return {};
    }

    SDL_memcpy(mapped, pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());

    // 提交上传命令
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (cmd == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
        return {};
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transferBuffer.get();
    srcInfo.pixels_per_row = width;
    srcInfo.rows_per_layer = height;

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = texture.get();
    dstRegion.w = width;
    dstRegion.h = height;
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    return texture;
}

} // namespace ui::managers
