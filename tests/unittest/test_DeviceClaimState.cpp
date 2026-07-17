#include <gtest/gtest.h>

#include "src/systems/render/DeviceClaimState.hpp"

namespace ui::tests
{
namespace
{

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

} // namespace
} // namespace ui::tests