#include <gtest/gtest.h>

#include "src/common/GPUWrappers.hpp"
#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/managers/TextureAtlas.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui::tests
{
namespace
{

struct ReleaseObservation
{
    SDL_GPUDevice* device = nullptr;
    SDL_GPUTexture* texture = nullptr;
    int count = 0;
};

ReleaseObservation& Observation()
{
    static ReleaseObservation observation;
    return observation;
}

void FakeReleaseTexture(SDL_GPUDevice* device, SDL_GPUTexture* texture)
{
    auto& observation = Observation();
    observation = {.device = device, .texture = texture, .count = observation.count + 1};
}

using TestTextureOwner =
    std::unique_ptr<SDL_GPUTexture, wrappers::GPUResourceDeleter<FakeReleaseTexture>>;

template <typename T>
T* Sentinel(std::uintptr_t value)
{
    return std::bit_cast<T*>(value);
}

TestTextureOwner MakeOwner(SDL_GPUDevice* device, SDL_GPUTexture* texture)
{
    return TestTextureOwner(texture, TestTextureOwner::deleter_type(device));
}

std::optional<std::vector<uint8_t>> ReadTexture(
    SDL_GPUDevice* device, SDL_GPUTexture* texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    const uint32_t byteCount = width * height;
    const SDL_GPUTransferBufferCreateInfo transferInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, .size = byteCount, .props = 0};
    auto transferBuffer = wrappers::MakeGpuResource<wrappers::UniqueGPUTransferBuffer>(
        device, SDL_CreateGPUTransferBuffer, &transferInfo);
    if (!transferBuffer)
    {
        return std::nullopt;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr)
    {
        return std::nullopt;
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass == nullptr)
    {
        static_cast<void>(SDL_CancelGPUCommandBuffer(commandBuffer));
        return std::nullopt;
    }

    const SDL_GPUTextureRegion source{.texture = texture,
                                      .mip_level = 0,
                                      .layer = 0,
                                      .x = x,
                                      .y = y,
                                      .z = 0,
                                      .w = width,
                                      .h = height,
                                      .d = 1};
    const SDL_GPUTextureTransferInfo destination{
        .transfer_buffer = transferBuffer.get(), .offset = 0, .pixels_per_row = width, .rows_per_layer = height};
    SDL_DownloadFromGPUTexture(copyPass, &source, &destination);
    SDL_EndGPUCopyPass(copyPass);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commandBuffer);
    if (fence == nullptr)
    {
        return std::nullopt;
    }
    const bool waitSucceeded = SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);
    if (!waitSucceeded)
    {
        return std::nullopt;
    }

    const void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer.get(), false);
    if (mappedData == nullptr)
    {
        return std::nullopt;
    }
    std::vector<uint8_t> result(byteCount);
    std::memcpy(result.data(), mappedData, byteCount);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer.get());
    return result;
}

class TextureAtlasGpuTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0U)
        {
            if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
            {
                GTEST_SKIP() << "SDL Video initialization failed: " << SDL_GetError();
            }
            m_ownsVideoSubsystem = true;
        }

        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        m_device.reset(SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr));
        if (m_device == nullptr)
        {
            GTEST_SKIP() << "No SDL GPU backend available: " << SDL_GetError();
        }
    }

    void TearDown() override
    {
        m_device.reset();
        m_scope.reset();
        if (m_ownsVideoSubsystem)
        {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            m_ownsVideoSubsystem = false;
        }
    }

    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
    wrappers::UniqueGPUDevice m_device;
    bool m_ownsVideoSubsystem = false;
};

class GPUTextureOwnerTest : public ::testing::Test
{
protected:
    void SetUp() override { Observation() = {}; }
};

TEST_F(GPUTextureOwnerTest, ReleasesWithCreatingDevice)
{
    constexpr std::uintptr_t DEVICE_SENTINEL = 0x1000U;
    constexpr std::uintptr_t TEXTURE_SENTINEL = 0x2000U;
    auto* device = Sentinel<SDL_GPUDevice>(DEVICE_SENTINEL);
    auto* texture = Sentinel<SDL_GPUTexture>(TEXTURE_SENTINEL);
    auto owner = MakeOwner(device, texture);

    owner.reset();

    EXPECT_EQ(Observation().device, device);
    EXPECT_EQ(Observation().texture, texture);
    EXPECT_EQ(Observation().count, 1);
}

TEST_F(GPUTextureOwnerTest, MoveTransfersOwnershipAndReleasesExactlyOnce)
{
    constexpr std::uintptr_t DEVICE_SENTINEL = 0x1000U;
    constexpr std::uintptr_t TEXTURE_SENTINEL = 0x2000U;
    auto owner = MakeOwner(Sentinel<SDL_GPUDevice>(DEVICE_SENTINEL), Sentinel<SDL_GPUTexture>(TEXTURE_SENTINEL));
    TestTextureOwner moved = std::move(owner);

    EXPECT_EQ(owner, nullptr);
    moved.reset();
    EXPECT_EQ(Observation().count, 1);
}

TEST_F(GPUTextureOwnerTest, ContainerClearReleasesEveryOwnerWithItsDevice)
{
    std::unordered_map<std::string, TestTextureOwner> cache;
    constexpr std::uintptr_t FIRST_DEVICE_SENTINEL = 0x1000U;
    constexpr std::uintptr_t FIRST_TEXTURE_SENTINEL = 0x2000U;
    constexpr std::uintptr_t SECOND_DEVICE_SENTINEL = 0x3000U;
    constexpr std::uintptr_t SECOND_TEXTURE_SENTINEL = 0x4000U;
    cache.try_emplace("first",
                      MakeOwner(Sentinel<SDL_GPUDevice>(FIRST_DEVICE_SENTINEL),
                                Sentinel<SDL_GPUTexture>(FIRST_TEXTURE_SENTINEL)));
    cache.try_emplace("second",
                      MakeOwner(Sentinel<SDL_GPUDevice>(SECOND_DEVICE_SENTINEL),
                                Sentinel<SDL_GPUTexture>(SECOND_TEXTURE_SENTINEL)));

    cache.clear();

    EXPECT_EQ(Observation().count, 2);
}

TEST_F(GPUTextureOwnerTest, EmptyOwnerDoesNotRelease)
{
    TestTextureOwner owner;
    owner.reset();

    EXPECT_EQ(Observation().count, 0);
}

TEST_F(TextureAtlasGpuTest, UploadsR8BitmapAndCachesOnlySuccessfulGlyphs)
{
    managers::TextureAtlas atlas(m_device.get(), 8, 0);
    constexpr std::array<uint8_t, 6> BITMAP{1, 2, 3, 4, 5, 6};

    const auto glyph = atlas.addGlyph(1, BITMAP.data(), 3, 2, 4, 5, 6.0F);
    ASSERT_TRUE(glyph.has_value());
    const auto readback = ReadTexture(m_device.get(), atlas.getTexture(), 0, 0, 3, 2);
    ASSERT_TRUE(readback.has_value()) << SDL_GetError();
    EXPECT_EQ(*readback, std::vector<uint8_t>(BITMAP.begin(), BITMAP.end()));
    EXPECT_TRUE(atlas.getGlyph(1).has_value());

    EXPECT_FALSE(atlas.addGlyph(2, nullptr, 3, 2, 0, 0, 0.0F).has_value());
    EXPECT_FALSE(atlas.addGlyph(3, BITMAP.data(), 0, 2, 0, 0, 0.0F).has_value());
    EXPECT_FALSE(atlas.addGlyph(4, BITMAP.data(), -1, 2, 0, 0, 0.0F).has_value());
    EXPECT_FALSE(atlas.getGlyph(2).has_value());
    EXPECT_EQ(atlas.getStats().glyphCount, 1U);
}

TEST_F(TextureAtlasGpuTest, ExpansionMigratesPixelsAndPreservesGlyphMetadata)
{
    managers::TextureAtlas atlas(m_device.get(), 4, 0);
    constexpr std::array<uint8_t, 16> FIRST_BITMAP{
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    constexpr std::array<uint8_t, 1> SECOND_BITMAP{99};

    const auto before = atlas.addGlyph(10, FIRST_BITMAP.data(), 4, 4, 2, 3, 5.0F);
    ASSERT_TRUE(before.has_value());
    const auto addedAfterExpansion = atlas.addGlyph(11, SECOND_BITMAP.data(), 1, 1, 0, 0, 1.0F);
    ASSERT_TRUE(addedAfterExpansion.has_value());
    ASSERT_EQ(atlas.getSize(), 8U);

    const auto after = atlas.getGlyph(10);
    ASSERT_TRUE(after.has_value());
    EXPECT_EQ(after->x, before->x);
    EXPECT_EQ(after->y, before->y);
    EXPECT_EQ(after->width, before->width);
    EXPECT_EQ(after->height, before->height);
    EXPECT_EQ(after->bearingX, before->bearingX);
    EXPECT_EQ(after->bearingY, before->bearingY);
    EXPECT_FLOAT_EQ(after->advanceX, before->advanceX);
    EXPECT_FLOAT_EQ(after->u0, static_cast<float>(after->x) / 8.0F);
    EXPECT_FLOAT_EQ(after->v0, static_cast<float>(after->y) / 8.0F);
    EXPECT_FLOAT_EQ(after->u1, static_cast<float>(after->x + after->width) / 8.0F);
    EXPECT_FLOAT_EQ(after->v1, static_cast<float>(after->y + after->height) / 8.0F);
    EXPECT_EQ(atlas.getStats().glyphCount, 2U);

    const auto migratedPixels = ReadTexture(m_device.get(), atlas.getTexture(), 0, 0, 4, 4);
    ASSERT_TRUE(migratedPixels.has_value()) << SDL_GetError();
    EXPECT_EQ(*migratedPixels, std::vector<uint8_t>(FIRST_BITMAP.begin(), FIRST_BITMAP.end()));
}

TEST_F(TextureAtlasGpuTest, MaximumSizeRejectionPreservesExistingGlyph)
{
    managers::TextureAtlas atlas(m_device.get(), 4096, 0);
    constexpr std::array<uint8_t, 1> BITMAP{77};
    ASSERT_TRUE(atlas.addGlyph(20, BITMAP.data(), 1, 1, 1, 2, 3.0F).has_value());
    const auto existingGlyph = atlas.getGlyph(20);
    ASSERT_TRUE(existingGlyph.has_value());

    EXPECT_FALSE(atlas.addGlyph(21, BITMAP.data(), 4097, 1, 0, 0, 0.0F).has_value());
    EXPECT_EQ(atlas.getSize(), 4096U);
    EXPECT_EQ(atlas.getStats().glyphCount, 1U);
    EXPECT_EQ(atlas.getGlyph(20)->x, existingGlyph->x);
    EXPECT_EQ(atlas.getGlyph(20)->y, existingGlyph->y);
}

} // namespace
} // namespace ui::tests