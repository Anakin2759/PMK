#include <gtest/gtest.h>

#include "src/common/GPUWrappers.hpp"

#include <bit>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

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

} // namespace
} // namespace ui::tests