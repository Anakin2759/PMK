/**
 * ************************************************************************
 *
 * @file gpu_window_lifecycle_benchmark.cpp
 * @brief GPU 窗口生命周期压力基准（Google Benchmark）
 *
 * 从 test_DeviceClaimState.cpp 的 GpuWindowLifecycleStressTest 迁移而来，
 * 每个迭代执行一次完整真实窗口生命周期：SDL 视频初始化 → 创建窗口 →
 * GPU 设备初始化 → 认领窗口 → 渲染清屏帧 → 反认领 → 销毁窗口 → 清理设备 →
 * 退出视频子系统。用于测量 GPU 后端（D3D12 / Vulkan）完整生命周期的耗时
 * 与稳定性，不再阻塞单元测试。
 *
 * 用法：
 *   ui_gpu_lifecycle_benchmark --benchmark_filter=Direct3D12
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <benchmark/benchmark.h>

#include "src/common/AppConfig.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/managers/DeviceManager.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <string_view>

namespace ui::benchmarks
{
namespace
{

constexpr int kWINDOW_EXTENT = 64;

/// 渲染一帧清屏帧并等待 GPU 空闲。失败时填充 error 并返回 false。
bool RenderAndPresentClearFrame(SDL_GPUDevice* device, SDL_Window* window, std::string& error)
{
    constexpr SDL_FColor CLEAR_COLOR{0.05F, 0.1F, 0.2F, 1.0F};
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr)
    {
        error = std::string("acquire command buffer: ") + SDL_GetError();
        return false;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nullptr, nullptr))
    {
        error = std::string("acquire swapchain: ") + SDL_GetError();
        static_cast<void>(SDL_CancelGPUCommandBuffer(commandBuffer));
        return false;
    }
    if (swapchainTexture == nullptr)
    {
        error = "acquire swapchain: no texture returned";
        static_cast<void>(SDL_CancelGPUCommandBuffer(commandBuffer));
        return false;
    }

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchainTexture;
    colorTarget.clear_color = CLEAR_COLOR;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);
    if (renderPass == nullptr)
    {
        error = std::string("begin render pass: ") + SDL_GetError();
        static_cast<void>(SDL_CancelGPUCommandBuffer(commandBuffer));
        return false;
    }
    SDL_EndGPURenderPass(renderPass);

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
    {
        error = std::string("submit clear frame: ") + SDL_GetError();
        return false;
    }
    if (!SDL_WaitForGPUIdle(device))
    {
        error = std::string("wait for GPU idle: ") + SDL_GetError();
        return false;
    }
    return true;
}

/// 执行一次完整窗口生命周期。失败时填充 error 并返回 false。
bool RunSingleGpuWindowLifecycle(std::string_view expectedDriver, std::string& error)
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        error = std::string("SDL video init: ") + SDL_GetError();
        return false;
    }

    SDL_Window* window = SDL_CreateWindow("VMP-ui GPU lifecycle", kWINDOW_EXTENT, kWINDOW_EXTENT, 0);
    if (window == nullptr)
    {
        error = std::string("window create: ") + SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    bool ok = true;
    {
        UiRuntime runtime;
        UiRuntimeScope const scope(runtime);
        managers::DeviceManager manager;

        const auto initializeResult = manager.initialize();
        if (!initializeResult.has_value())
        {
            ok = false;
            error = "device initialize: " + initializeResult.error().ToString();
        }
        else
        {
            const char* driver = SDL_GetGPUDeviceDriver(manager.getDevice());
            const std::string actualDriver = driver != nullptr ? driver : "";
            if (actualDriver != expectedDriver)
            {
                ok = false;
                error = "driver mismatch: actual=" + actualDriver + " expected=" + std::string(expectedDriver);
            }
            else if (!manager.claimWindow(window).has_value())
            {
                ok = false;
                error = "window claim failed";
            }
            else if (!RenderAndPresentClearFrame(manager.getDevice(), window, error))
            {
                ok = false;
            }
            manager.unclaimWindow(window);
            manager.cleanup();
        }
    }

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return ok;
}

void RunGpuWindowLifecycle(benchmark::State& state, std::string_view expectedDriver)
{
    // 前置校验：视频子系统必须干净，否则跳过。
    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0U)
    {
        state.SkipWithError("SDL video subsystem was already initialized before benchmark");
        return;
    }

    const char* forcedDriver = SDL_GetHint(SDL_HINT_GPU_DRIVER);
    if (forcedDriver == nullptr)
    {
        SDL_SetHint(SDL_HINT_GPU_DRIVER, std::string(expectedDriver).c_str());
        forcedDriver = SDL_GetHint(SDL_HINT_GPU_DRIVER);
    }
    if (forcedDriver == nullptr || std::string_view(forcedDriver) != expectedDriver)
    {
        state.SkipWithError("failed to force GPU driver");
        return;
    }

    config::AppConfig::instance().setPreferredBackend(std::string(expectedDriver));

    for (auto _ : state)
    {
        std::string error;
        if (!RunSingleGpuWindowLifecycle(expectedDriver, error))
        {
            state.SkipWithError(error);
            break;
        }
    }
}

void BM_Direct3D12WindowLifecycle(benchmark::State& state)
{
    RunGpuWindowLifecycle(state, "direct3d12");
}

void BM_VulkanWindowLifecycle(benchmark::State& state)
{
    RunGpuWindowLifecycle(state, "vulkan");
}

BENCHMARK(BM_Direct3D12WindowLifecycle)->Iterations(100)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VulkanWindowLifecycle)->Iterations(100)->Unit(benchmark::kMillisecond);

}  // namespace

}  // namespace ui::benchmarks
