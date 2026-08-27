#include <gtest/gtest.h>

#include "src/common/GpuFailureInjection.hpp"
#include "src/common/GPUWrappers.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/managers/DeviceManager.hpp"
#include "src/systems/render/DeviceClaimState.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace ui::tests
{
namespace
{

bool ReadWhitePixel(const detail::GpuDeviceGenerationHandle& generation, SDL_GPUDevice* device, SDL_GPUTexture* texture,
                    std::array<uint8_t, 4>& pixel)
{
    const SDL_GPUTransferBufferCreateInfo transferInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, .size = static_cast<uint32_t>(pixel.size()), .props = 0};
    auto transferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
        generation, SDL_CreateGPUTransferBuffer, &transferInfo);
    if (!transferBuffer)
    {
        return false;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr)
    {
        return false;
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass == nullptr)
    {
        static_cast<void>(SDL_CancelGPUCommandBuffer(commandBuffer));
        return false;
    }

    const SDL_GPUTextureRegion source{
        .texture = texture, .mip_level = 0, .layer = 0, .x = 0, .y = 0, .z = 0, .w = 1, .h = 1, .d = 1};
    const SDL_GPUTextureTransferInfo destination{
        .transfer_buffer = transferBuffer.get(), .offset = 0, .pixels_per_row = 1, .rows_per_layer = 1};
    SDL_DownloadFromGPUTexture(copyPass, &source, &destination);
    SDL_EndGPUCopyPass(copyPass);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);
    if (fence == nullptr)
    {
        return false;
    }
    const bool waitSucceeded = SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);
    if (!waitSucceeded)
    {
        return false;
    }

    const void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
    if (mappedData == nullptr)
    {
        return false;
    }
    std::memcpy(pixel.data(), mappedData, pixel.size());
    SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());
    return true;
}

class DeviceManagerGpuTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0U)
        {
            return;
        }
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            GTEST_SKIP() << "SDL Video initialization failed: " << SDL_GetError();
        }
        m_ownsVideoSubsystem = true;
    }

    void TearDown() override
    {
        if (m_ownsVideoSubsystem)
        {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            m_ownsVideoSubsystem = false;
        }
    }

   private:
    bool m_ownsVideoSubsystem = false;
};

class DeviceManagerWhiteTextureFailureTest : public DeviceManagerGpuTest,
                                             public ::testing::WithParamInterface<detail::GpuFaultPoint>
{
};

TEST(DeviceClaimStateTest, ResourcesCannotBecomeReadyBeforeFirstWindowLocksDevice)
{
    systems::render::DeviceClaimState state;

    EXPECT_TRUE(state.MayTryAnotherBackend());
    EXPECT_FALSE(state.MayCreateDeviceResources());
    EXPECT_FALSE(state.MarkResourcesReady());
    EXPECT_FALSE(state.MayCreateWhiteTexture());
}

TEST(DeviceClaimStateTest, FirstSuccessfulClaimLocksBackendBeforeResources)
{
    systems::render::DeviceClaimState state;

    state.MarkDeviceLocked();

    EXPECT_FALSE(state.MayTryAnotherBackend());
    EXPECT_TRUE(state.MayCreateDeviceResources());
    EXPECT_TRUE(state.MarkResourcesReady());
    EXPECT_TRUE(state.AreResourcesReady());
    EXPECT_TRUE(state.MayCreateWhiteTexture());
}

TEST(DeviceClaimStateTest, RepeatedClaimAndResourceInitializationAreIdempotent)
{
    systems::render::DeviceClaimState state;
    state.MarkDeviceLocked();
    EXPECT_TRUE(state.MarkResourcesReady());

    state.MarkDeviceLocked();
    EXPECT_TRUE(state.MarkResourcesReady());

    EXPECT_TRUE(state.IsDeviceLocked());
    EXPECT_TRUE(state.AreResourcesReady());
    EXPECT_FALSE(state.MayCreateDeviceResources());
}

TEST(DeviceClaimStateTest, ResetReturnsToBackendSelectionPhase)
{
    systems::render::DeviceClaimState state;
    state.MarkDeviceLocked();
    EXPECT_TRUE(state.MarkResourcesReady());

    state.Reset();

    EXPECT_FALSE(state.IsDeviceLocked());
    EXPECT_FALSE(state.AreResourcesReady());
    EXPECT_TRUE(state.MayTryAnotherBackend());
}

TEST_F(DeviceManagerGpuTest, WhiteTextureBelongsToDeviceGenerationAndContainsWhitePixel)
{
    UiRuntime runtime;
    UiRuntimeScope const scope(runtime);
    utils::Logger logger;
    managers::DeviceManager manager{logger};

    const auto initializeResult = manager.initialize();
    if (!initializeResult.has_value())
    {
        GTEST_SKIP() << "No SDL GPU backend available: " << SDL_GetError();
    }

    ASSERT_NE(manager.getDevice(), nullptr);
    const auto firstGeneration = manager.getGeneration();
    ASSERT_TRUE(firstGeneration);
    SDL_GPUTexture* const whiteTexture = manager.getWhiteTexture();
    ASSERT_NE(whiteTexture, nullptr);
    EXPECT_EQ(manager.getWhiteTexture(), whiteTexture);

    std::array<uint8_t, 4> pixel{};
    ASSERT_TRUE(ReadWhitePixel(firstGeneration, manager.getDevice(), whiteTexture, pixel)) << SDL_GetError();
    EXPECT_EQ(pixel, (std::array<uint8_t, 4>{255, 255, 255, 255}));

    manager.cleanup();
    EXPECT_EQ(manager.getDevice(), nullptr);
    EXPECT_EQ(manager.getWhiteTexture(), nullptr);
    EXPECT_FALSE(manager.getGeneration());
    EXPECT_TRUE(manager.getDriverName().empty());
    EXPECT_EQ(firstGeneration.Status(), detail::GpuDeviceGenerationStatus::INVALID);
    EXPECT_EQ(firstGeneration.LateReleaseSkipped(), 0U);
    manager.cleanup();
    EXPECT_EQ(manager.getDevice(), nullptr);
    EXPECT_EQ(manager.getWhiteTexture(), nullptr);

    const auto reinitializeResult = manager.initialize();
    ASSERT_TRUE(reinitializeResult.has_value()) << reinitializeResult.error().ToString();
    EXPECT_NE(manager.getDevice(), nullptr);
    EXPECT_NE(manager.getWhiteTexture(), nullptr);
    const auto secondGeneration = manager.getGeneration();
    ASSERT_TRUE(secondGeneration);
    EXPECT_LT(firstGeneration.Id(), secondGeneration.Id());
    manager.cleanup();
    EXPECT_EQ(secondGeneration.LateReleaseSkipped(), 0U);
}

TEST_P(DeviceManagerWhiteTextureFailureTest, FailureDoesNotPublishCandidateState)
{
    UiRuntime runtime;
    UiRuntimeScope const scope(runtime);
    utils::Logger logger;
    managers::DeviceManager manager{logger};
    detail::ScopedGpuFault fault(GetParam(), 1, true);

    const auto initializeResult = manager.initialize();
    if (fault.HitCount() == 0U)
    {
        GTEST_SKIP() << "No SDL GPU backend reached the white texture stage: " << SDL_GetError();
    }

    EXPECT_FALSE(initializeResult.has_value());
    EXPECT_GE(fault.HitCount(), 1U);
    EXPECT_EQ(manager.getDevice(), nullptr);
    EXPECT_EQ(manager.getWhiteTexture(), nullptr);
    EXPECT_FALSE(manager.getGeneration());
    EXPECT_TRUE(manager.getDriverName().empty());
    EXPECT_FALSE(manager.hasClaimedWindows());
    manager.cleanup();
    manager.cleanup();
    EXPECT_EQ(manager.getDevice(), nullptr);
    EXPECT_EQ(manager.getWhiteTexture(), nullptr);
    EXPECT_FALSE(manager.getGeneration());
}

INSTANTIATE_TEST_SUITE_P(WhiteTextureStages, DeviceManagerWhiteTextureFailureTest,
                         ::testing::Values(detail::GpuFaultPoint::RESOURCE_CREATE,
                                           detail::GpuFaultPoint::TRANSFER_CREATE, detail::GpuFaultPoint::MAP,
                                           detail::GpuFaultPoint::COMMAND_ACQUIRE,
                                           detail::GpuFaultPoint::COPY_PASS_BEGIN, detail::GpuFaultPoint::SUBMIT));

TEST_F(DeviceManagerGpuTest, DeviceCreateFailureDoesNotPublishCandidateState)
{
    UiRuntime runtime;
    UiRuntimeScope const scope(runtime);
    utils::Logger logger;
    managers::DeviceManager manager{logger};
    detail::ScopedGpuFault fault(detail::GpuFaultPoint::DEVICE_CREATE, 1, true);

    const auto initializeResult = manager.initialize();

    EXPECT_FALSE(initializeResult.has_value());
    EXPECT_EQ(manager.getDevice(), nullptr);
    EXPECT_EQ(manager.getWhiteTexture(), nullptr);
    EXPECT_FALSE(manager.getGeneration());
    EXPECT_TRUE(manager.getDriverName().empty());
}

TEST_F(DeviceManagerGpuTest, DeliberatelyInvalidatedGenerationSkipsLateOwner)
{
    UiRuntime runtime;
    UiRuntimeScope const scope(runtime);
    utils::Logger logger;
    managers::DeviceManager manager{logger};

    const auto initializeResult = manager.initialize();
    if (!initializeResult.has_value())
    {
        GTEST_SKIP() << "No SDL GPU backend available: " << SDL_GetError();
    }

    const auto generation = manager.getGeneration();
    ASSERT_TRUE(generation);
    constexpr std::uintptr_t TEXTURE_SENTINEL = 0x1000U;
    auto lateOwner = wrappers::MakeGpuResource<wrappers::UniqueGPUTexture>(
        generation, [](SDL_GPUDevice*, std::uintptr_t sentinel) { return std::bit_cast<SDL_GPUTexture*>(sentinel); },
        TEXTURE_SENTINEL);

    manager.cleanup();
    lateOwner.reset();

    EXPECT_EQ(generation.Status(), detail::GpuDeviceGenerationStatus::INVALID);
    EXPECT_EQ(generation.LateReleaseSkipped(), 1U);
}

}  // namespace
}  // namespace ui::tests