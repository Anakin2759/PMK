/**
 * ************************************************************************
 *
 * @file DeviceManager.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 管理 GPU 设备和窗口声明

    GPU 后端回退方案实现：
    - 直接在 DeviceManager 内部维护一个后端列表（如 D3D12、Vulkan），并在初始化时逐个尝试创建 GPU 设备。
    - 如果当前后端无法声明窗口（例如在 VM 中 D3D12 无法渲染），则自动切换到下一个后端并重试，直到成功或所有后端均失败。
    - 尝试后备方案使用 cmrc 内嵌资源库的 swiftshader 库提供的 vulkan 实现，确保在没有物理 GPU
 的环境中也能提供基本的渲染支持。
    - CPU/software fallback 由 RenderSystem 的 FallbackBackendRenderer 处理，不属于 SDL_GPU 后端列表。
    - 虚拟机环境可能遇到初始化成功，但无法声明窗口的情况（例如 D3D12 在某些 VM 中无法渲染）。
    - 在这种情况下，DeviceManager 将自动切换到下一个后端（如 Vulkan）并重试声明窗口，确保应用能够继续运行而不是崩溃。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <functional>
#include <SDL3/SDL.h>
#include "common/AppConfig.hpp"
#include "ui/Result.hpp"
#include "ui/ErrorCodes.hpp"

// CMRC_DECLARE(ui_swiftshader);

#include <SDL3/SDL_gpu.h>
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"
#include "common/GPUWrappers.hpp"

namespace ui::managers
{

/**
 * @brief 管理 GPU 设备和窗口声明
 *
 * 每个成功创建设备的代际同时创建并唯一持有一张 1x1 RGBA 白色纹理。
 * 后端切换或清理时，白色纹理在所属 GPU 设备销毁前释放。
 */
class DeviceManager
{
public:
    struct BackendConfig
    {
        std::string name;
        std::function<void(SDL_PropertiesID)> configure;
    };

    DeviceManager()
    {
        m_backends = {{.name = "direct3d12",
                       .configure =
                           [](SDL_PropertiesID props)
                       {
                           SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "direct3d12");
                           SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
                           SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
                       }},
                      {.name = "vulkan",
                       .configure =
                           [](SDL_PropertiesID props)
                       {
                           SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
                           SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
                           SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
                       }}

        };
    }

    ~DeviceManager() { cleanup(); }
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    DeviceManager(DeviceManager&&) noexcept = default;
    DeviceManager& operator=(DeviceManager&&) noexcept = default;

    Result<void> initialize()
    {
        if (m_gpuDevice != nullptr) return Ok();

        ui::UiRuntime::current().logger().info("DeviceManager: 开始初始化 GPU 后端 (Strategy: Iterative Configuration)");

        applyPreferredBackend();

        for (size_t i = 0; i < m_backends.size(); ++i)
        {
            if (createDevice(i))
            {
                return Ok();
            }
        }

        ui::UiRuntime::current().logger().error("所有 GPU 后端方案均初始化失败！请检查显卡驱动或虚拟机 3D 加速设置。");
        return Err(UiErrc::BACKEND_UNAVAILABLE);
    }

    Result<void> claimWindow(SDL_Window* sdlWindow)
    {
        if (m_gpuDevice == nullptr || sdlWindow == nullptr)
        {
            ui::UiRuntime::current().logger().error("claimWindow: 无效的设备或窗口句柄");
            return Err(UiErrc::INVALID_ARGUMENT);
        }

        SDL_WindowID windowID = SDL_GetWindowID(sdlWindow);
        if (m_claimedWindows.contains(windowID))
        {
            return Ok();
        }

        // 尝试声明窗口
        if (SDL_ClaimWindowForGPUDevice(m_gpuDevice.get(), sdlWindow))
        {
            m_claimedWindows.insert(windowID);
            return Ok();
        }

        // 已有窗口和资源绑定当前设备后，禁止因后续窗口声明失败而销毁全局设备。
        if (!m_claimedWindows.empty())
        {
            return Err(UiErrc::WINDOW_CLAIM_FAILED, SDL_GetError());
        }

        // 核心修改：如果声明失败（例如 D3D12 在 VM 中无法渲染），尝试回退到其他后端
        ui::UiRuntime::current().logger().warn("当前后端 {} 无法声明窗口 ({}). 尝试切换其他后端...", m_gpuDriver, SDL_GetError());

        // 尝试后续的后端
        size_t nextIndex = m_currentBackendIndex + 1;
        while (nextIndex < m_backends.size())
        {
            // 清理当前失败的设备
            cleanup();

            // 尝试创建下一个设备
            if (createDevice(nextIndex))
            {
                ui::UiRuntime::current().logger().info("已切换至后端: {}，重试声明窗口...", m_gpuDriver);
                if (SDL_ClaimWindowForGPUDevice(m_gpuDevice.get(), sdlWindow))
                {
                    m_claimedWindows.insert(windowID);
                    return Ok();
                }
                ui::UiRuntime::current().logger().warn("后端 {} 也无法声明窗口，继续寻找...", m_gpuDriver);
            }
            nextIndex++;
        }

        ui::UiRuntime::current().logger().error("致命错误: 所有可用后端均无法声明/渲染窗口！");
        return Err(UiErrc::WINDOW_CLAIM_FAILED, SDL_GetError());
    }

    void unclaimWindow(SDL_Window* sdlWindow)
    {
        if (m_gpuDevice == nullptr || sdlWindow == nullptr) return;

        SDL_WindowID windowID = SDL_GetWindowID(sdlWindow);
        if (m_claimedWindows.contains(windowID))
        {
            SDL_ReleaseWindowFromGPUDevice(m_gpuDevice.get(), sdlWindow);
            m_claimedWindows.erase(windowID);
        }
    }

    /**
     * @brief 清理当前设备代际及其窗口声明和白色纹理
     * @note 等待 GPU 空闲后先释放白色纹理，再销毁 GPU 设备；可重复调用。
     */
    void cleanup()
    {
        if (m_gpuDevice == nullptr)
        {
            m_whiteTexture.reset();
            return;
        }

        SDL_WaitForGPUIdle(m_gpuDevice.get());
        m_whiteTexture.reset();

        // 释放所有窗口声明
        for (auto windowID : m_claimedWindows)
        {
            SDL_Window* window = SDL_GetWindowFromID(windowID);
            if (window != nullptr)
            {
                SDL_ReleaseWindowFromGPUDevice(m_gpuDevice.get(), window);
            }
        }
        m_claimedWindows.clear();

        m_gpuDevice.reset();
    }

    [[nodiscard]] SDL_GPUDevice* getDevice() const { return m_gpuDevice.get(); }
    [[nodiscard]] const std::string& getDriverName() const { return m_gpuDriver; }
    [[nodiscard]] bool hasClaimedWindows() const noexcept { return !m_claimedWindows.empty(); }

    /**
     * @brief 获取当前设备代际的白色纹理
     * @return 非拥有的纹理指针；设备未成功初始化或已清理时返回 nullptr
     * @note 返回指针的生命周期由 DeviceManager 管理，调用方不得释放。
     */
    [[nodiscard]] SDL_GPUTexture* getWhiteTexture() const { return m_whiteTexture.get(); }

private:
    /**
     * @brief 为当前候选设备创建并提交 1x1 RGBA 白色纹理上传
     * @return 上传命令提交成功时返回 true，否则返回 false
     * @note 成功表示上传命令已提交，不表示 GPU 已执行完成。
     */
    bool createWhiteTexture()
    {
        SDL_GPUDevice* device = m_gpuDevice.get();
        if (device == nullptr)
        {
            return false;
        }

        const SDL_GPUTextureCreateInfo textureInfo{.type = SDL_GPU_TEXTURETYPE_2D,
                                                   .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                                   .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                                   .width = 1,
                                                   .height = 1,
                                                   .layer_count_or_depth = 1,
                                                   .num_levels = 1,
                                                   .sample_count = SDL_GPU_SAMPLECOUNT_1,
                                                   .props = 0};
        auto whiteTexture = wrappers::MakeGpuResource<wrappers::UniqueGPUTexture>(
            device, SDL_CreateGPUTexture, &textureInfo);
        if (!whiteTexture)
        {
            ui::UiRuntime::current().logger().error("DeviceManager: 创建白色纹理失败 ({})", SDL_GetError());
            return false;
        }

        constexpr std::array<uint8_t, 4> WHITE_PIXEL{255, 255, 255, 255};
        const SDL_GPUTransferBufferCreateInfo transferInfo{.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                           .size = static_cast<uint32_t>(WHITE_PIXEL.size()),
                                                           .props = 0};
        auto transferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
            device, SDL_CreateGPUTransferBuffer, &transferInfo);
        if (!transferBuffer)
        {
            ui::UiRuntime::current().logger().error("DeviceManager: 创建白纹理上传缓冲失败 ({})", SDL_GetError());
            return false;
        }

        void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
        if (mappedData == nullptr)
        {
            ui::UiRuntime::current().logger().error("DeviceManager: 映射白纹理上传缓冲失败 ({})", SDL_GetError());
            return false;
        }
        std::memcpy(mappedData, WHITE_PIXEL.data(), WHITE_PIXEL.size());
        SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());

        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (commandBuffer == nullptr)
        {
            ui::UiRuntime::current().logger().error("DeviceManager: 获取白纹理命令缓冲失败 ({})", SDL_GetError());
            return false;
        }

        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        if (copyPass == nullptr)
        {
            ui::UiRuntime::current().logger().error("DeviceManager: 开始白纹理复制通道失败 ({})", SDL_GetError());
            if (!SDL_CancelGPUCommandBuffer(commandBuffer))
            {
                ui::UiRuntime::current().logger().error("DeviceManager: 取消白纹理命令缓冲失败 ({})", SDL_GetError());
            }
            return false;
        }

        const SDL_GPUTextureTransferInfo source{
            .transfer_buffer = transferBuffer.get(), .offset = 0, .pixels_per_row = 1, .rows_per_layer = 1};
        const SDL_GPUTextureRegion destination{.texture = whiteTexture.get(),
                                               .mip_level = 0,
                                               .layer = 0,
                                               .x = 0,
                                               .y = 0,
                                               .z = 0,
                                               .w = 1,
                                               .h = 1,
                                               .d = 1};
        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);

        if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
        {
            ui::UiRuntime::current().logger().error("DeviceManager: 提交白纹理上传失败 ({})", SDL_GetError());
            return false;
        }

        m_whiteTexture = std::move(whiteTexture);
        return true;
    }

    void applyPreferredBackend()
    {
        auto preferred = config::AppConfig::instance().preferredBackend();
        if (preferred.empty()) return;

        if (config::AppConfig::instance().forceFallbackRenderer())
        {
            return;
        }

        auto backendIter = std::find_if(
            m_backends.begin(), m_backends.end(), [&](const BackendConfig& cfg) { return cfg.name == preferred; });
        if (backendIter == m_backends.end())
        {
            ui::UiRuntime::current().logger().warn("未知 GPU 后端 \"{}\"，使用默认顺序。可选: direct3d12 / vulkan", preferred);
            return;
        }
        if (backendIter != m_backends.begin())
        {
            std::rotate(m_backends.begin(), backendIter, backendIter + 1);
        }
        ui::UiRuntime::current().logger().info("应用命令行 GPU 后端偏好：优先尝试 {}", m_backends.front().name);
    }

    bool createDevice(size_t index)
    {
        if (index >= m_backends.size()) return false;

        const auto& config = m_backends[index];
        ui::UiRuntime::current().logger().info("尝试初始化后端: {}...", config.name);

        wrappers::UniquePropertiesID props(SDL_CreateProperties());
        if (config.configure)
        {
            config.configure(props);
        }

        SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);
        if (device != nullptr)
        {
            m_gpuDevice.reset(device);
            if (!createWhiteTexture())
            {
                m_whiteTexture.reset();
                m_gpuDevice.reset();
                m_gpuDriver.clear();
                ui::UiRuntime::current().logger().warn("后端 {} 白色纹理初始化失败，尝试下一后端", config.name);
                return false;
            }
            m_gpuDriver = config.name;
            m_currentBackendIndex = index;
            ui::UiRuntime::current().logger().info("GPU 初始化成功，锁定后端: {}", m_gpuDriver);
            return true;
        }

        ui::UiRuntime::current().logger().warn("后端 {} 初始化失败 ({})", config.name, SDL_GetError());
        return false;
    }

    wrappers::UniqueGPUDevice m_gpuDevice;
    wrappers::UniqueGPUTexture m_whiteTexture; ///< 当前设备代际唯一拥有的白色纹理
    std::string m_gpuDriver;
    std::unordered_set<SDL_WindowID> m_claimedWindows;

    std::vector<BackendConfig> m_backends;
    size_t m_currentBackendIndex = 0;
};

} // namespace ui::managers
