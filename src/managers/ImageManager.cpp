#include "ui/Result.hpp"
#include <algorithm>
#include <string>
#include "utils/Logger.hpp"
#include "ui/ErrorCodes.hpp"
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <span>
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_stdinc.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "ImageManager.hpp"
#include "DeviceManager.hpp"
#include "common/GpuFailureInjection.hpp"

#include <SDL3/SDL_gpu.h>

namespace ui::managers
{
namespace
{
struct StbiImageDeleter
{
    void operator()(unsigned char* pixels) const noexcept
    {
        stbi_image_free(pixels);
    }
};

std::string GetLowercaseExtension(const std::string& path)
{
    const auto dot = path.rfind('.');
    if (dot == std::string::npos)
    {
        return {};
    }

    std::string extension = path.substr(dot + 1);
    for (auto& character : extension)
    {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return extension;
}

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
}  // namespace

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

    auto generation =
        m_deviceManager != nullptr ? m_deviceManager->getGeneration() : detail::GpuDeviceGenerationHandle{};
    if (!generation || generation.Status() != detail::GpuDeviceGenerationStatus::ACTIVE)
    {
        UiRuntime::current().logger().error("[ImageManager] loadTexture: device is null, path={}", path);
        return ui::Err(ui::UiErrc::DEVICE_UNAVAILABLE, path);
    }

    if (m_cacheGenerationId != generation.Id())
    {
        m_cache.clear();
        m_cacheGenerationId = generation.Id();
    }

    // 缓存命中
    if (auto textureIt = m_cache.find(path); textureIt != m_cache.end())
    {
        return textureIt->second.get();
    }

    auto pixelsResult = loadPixels(path);
    if (!pixelsResult)
    {
        return ui::Err(pixelsResult.error());
    }
    const PixelBuffer& pixels = **pixelsResult;
    auto texture = uploadToGpu(generation, pixels.rgba.data(), static_cast<std::uint32_t>(pixels.width),
                               static_cast<std::uint32_t>(pixels.height));

    if (texture != nullptr && generation.Status() == detail::GpuDeviceGenerationStatus::ACTIVE &&
        m_cacheGenerationId == generation.Id())
    {
        auto* rawTexture = texture.get();
        auto [textureIt, inserted] = m_cache.try_emplace(path, std::move(texture));
        UiRuntime::current().logger().info("[ImageManager] Loaded texture: {}", path);
        return inserted ? rawTexture : textureIt->second.get();
    }

    UiRuntime::current().logger().error("[ImageManager] Failed to load texture: {}", path);
    return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
}

ui::Result<const ImageManager::PixelBuffer*> ImageManager::loadPixels(const std::string& path)
{
    if (path.empty())
    {
        UiRuntime::current().logger().error("[ImageManager] loadPixels: empty path");
        return ui::Err(ui::UiErrc::INVALID_ARGUMENT, "empty path");
    }

    if (auto pixelsIt = m_pixelCache.find(path); pixelsIt != m_pixelCache.end())
    {
        return pixelsIt->second.get();
    }

    auto decoded = decodePixels(path);
    if (!decoded)
    {
        return ui::Err(decoded.error());
    }

    auto [pixelsIt, inserted] = m_pixelCache.try_emplace(path, std::make_unique<PixelBuffer>(std::move(*decoded)));
    static_cast<void>(inserted);
    return pixelsIt->second.get();
}

void ImageManager::releaseAll()
{
    m_cache.clear();
    m_cacheGenerationId = 0;
}

ui::Result<ImageManager::PixelBuffer> ImageManager::decodePixels(const std::string& path)
{
    if (GetLowercaseExtension(path) == "bmp")
    {
        using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;
        SurfacePtr surface{SDL_LoadBMP(path.c_str()), &SDL_DestroySurface};
        if (surface == nullptr)
        {
            UiRuntime::current().logger().error("[ImageManager] SDL_LoadBMP failed: {} — {}", path, SDL_GetError());
            return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
        }

        // RGBA32 明确表示内存中的字节顺序，避免 RGBA8888 在小端平台发生通道歧义。
        SurfacePtr converted{SDL_ConvertSurface(surface.get(), SDL_PIXELFORMAT_RGBA32), &SDL_DestroySurface};
        if (converted == nullptr)
        {
            UiRuntime::current().logger().error("[ImageManager] SDL_ConvertSurface failed: {}", SDL_GetError());
            return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
        }

        const auto width = converted->w;
        const auto height = converted->h;
        if (width <= 0 || height <= 0 || static_cast<std::size_t>(width) >
                                               std::numeric_limits<std::size_t>::max() / 4U /
                                                   static_cast<std::size_t>(height))
        {
            return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
        }

        PixelBuffer result;
        result.width = width;
        result.height = height;
        const std::size_t rowBytes = static_cast<std::size_t>(width) * 4U;
        if (converted->pitch <= 0 || static_cast<std::size_t>(converted->pitch) < rowBytes)
        {
            return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
        }
        result.rgba.resize(rowBytes * static_cast<std::size_t>(height));
        const auto* sourceBytes = static_cast<const std::uint8_t*>(converted->pixels);
        const auto sourceSize = static_cast<std::size_t>(converted->pitch) * static_cast<std::size_t>(height);
        const std::span<const std::uint8_t> source{sourceBytes, sourceSize};
        std::span<std::uint8_t> destination{result.rgba};
        for (int row = 0; row < height; ++row)
        {
            const auto rowIndex = static_cast<std::size_t>(row);
            const auto sourceRow = source.subspan(rowIndex * static_cast<std::size_t>(converted->pitch), rowBytes);
            std::copy(sourceRow.begin(), sourceRow.end(), destination.subspan(rowIndex * rowBytes, rowBytes).begin());
        }
        return result;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    std::unique_ptr<unsigned char, StbiImageDeleter> pixels{
        stbi_load(path.c_str(), &width, &height, &channels, 4)};
    if (pixels == nullptr)
    {
        UiRuntime::current().logger().error("[ImageManager] stbi_load failed: {} — {}", path, stbi_failure_reason());
        return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
    }

    if (width <= 0 || height <= 0 || static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / 4U /
                                                                     static_cast<std::size_t>(height))
    {
        return ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, path);
    }

    PixelBuffer result;
    result.width = width;
    result.height = height;
    const std::size_t dataSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    result.rgba.assign(pixels.get(), pixels.get() + dataSize);
    return result;
}

wrappers::UniqueGPUTexture ImageManager::uploadToGpu(const detail::GpuDeviceGenerationHandle& generation,
                                                     const std::uint8_t* pixels, std::uint32_t width,
                                                     std::uint32_t height)
{
    auto& logger = UiRuntime::current().logger();
    const auto activeDevice = generation.InvokeIfActive([](SDL_GPUDevice* device) { return device; });
    SDL_GPUDevice* const device = activeDevice.value_or(nullptr);
    if (device == nullptr || pixels == nullptr || width == 0 || height == 0 ||
        width > std::numeric_limits<uint32_t>::max() / height ||
        width * height > std::numeric_limits<uint32_t>::max() / 4U)
    {
        return {};
    }

    // 创建 GPU 纹理
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = width;
    texInfo.height = height;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::RESOURCE_CREATE))
    {
        return {};
    }
    auto texture = wrappers::MakeGpuResource<wrappers::UniqueGPUTexture>(generation, SDL_CreateGPUTexture, &texInfo);
    if (texture == nullptr)
    {
        logger.error("[ImageManager] SDL_CreateGPUTexture failed: {}", SDL_GetError());
        return {};
    }

    const uint32_t dataSize = width * height * 4U;

    // 创建传输缓冲区
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;

    if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::TRANSFER_CREATE))
    {
        return {};
    }
    auto transferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
        generation, SDL_CreateGPUTransferBuffer, &transferInfo);
    if (transferBuffer == nullptr)
    {
        logger.error("[ImageManager] SDL_CreateGPUTransferBuffer failed: {}", SDL_GetError());
        return {};
    }

    // 映射并拷贝像素
    void* mapped = nullptr;
    if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::MAP))
    {
        mapped = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
    }
    if (mapped == nullptr)
    {
        logger.error("[ImageManager] SDL_MapGPUTransferBuffer failed: {}", SDL_GetError());
        return {};
    }

    SDL_memcpy(mapped, pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());

    // 提交上传命令
    SDL_GPUCommandBuffer* cmd = nullptr;
    if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COMMAND_ACQUIRE))
    {
        cmd = SDL_AcquireGPUCommandBuffer(device);
    }
    if (cmd == nullptr)
    {
        logger.error("[ImageManager] SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
        return {};
    }

    SDL_GPUCopyPass* copyPass = nullptr;
    if (!detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::COPY_PASS_BEGIN))
    {
        copyPass = SDL_BeginGPUCopyPass(cmd);
    }
    if (copyPass == nullptr)
    {
        logger.error("[ImageManager] SDL_BeginGPUCopyPass failed: {}", SDL_GetError());
        if (!SDL_CancelGPUCommandBuffer(cmd))
        {
            logger.error("[ImageManager] SDL_CancelGPUCommandBuffer failed: {}", SDL_GetError());
        }
        return {};
    }

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
    if (detail::ShouldInjectGpuFailure(detail::GpuFaultPoint::SUBMIT))
    {
        if (!SDL_CancelGPUCommandBuffer(cmd))
        {
            logger.error("[ImageManager] SDL_CancelGPUCommandBuffer failed: {}", SDL_GetError());
        }
        return {};
    }
    if (!SDL_SubmitGPUCommandBuffer(cmd))
    {
        logger.error("[ImageManager] SDL_SubmitGPUCommandBuffer failed: {}", SDL_GetError());
        return {};
    }

    return texture;
}

}  // namespace ui::managers
