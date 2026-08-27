#include "managers/ImageManager.hpp"
#include "utils/Logger.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace ui::tests
{
namespace
{

TEST(ImageManagerPixelsTest, LoadsAndCachesTemporaryBmpWithoutGpuDevice)
{
    // 2x1、24 位 BMP：像素依次为红、绿，扫描行补齐到 4 字节边界。
    constexpr std::array<std::uint8_t, 62> BMP_BYTES = {
        0x42, 0x4D, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00};
    const std::filesystem::path imagePath =
        std::filesystem::path(::testing::TempDir()) / "vmp_ui_image_manager_pixels.bmp";
    std::filesystem::remove(imagePath);
    {
        std::ofstream output(imagePath, std::ios::binary);
        ASSERT_TRUE(output.is_open());
        for (const auto byte : BMP_BYTES)
        {
            output.put(static_cast<char>(byte));
        }
        ASSERT_TRUE(output.good());
    }

    utils::Logger logger;
    managers::ImageManager imageManager{nullptr, logger};
    const auto first = imageManager.loadPixels(imagePath.string());
    ASSERT_TRUE(first.has_value()) << first.error().ToString();
    ASSERT_NE(*first, nullptr);
    EXPECT_EQ((*first)->width, 2);
    EXPECT_EQ((*first)->height, 1);
    EXPECT_EQ((*first)->rgba, (std::vector<std::uint8_t>{255, 0, 0, 255, 0, 255, 0, 255}));

    const auto second = imageManager.loadPixels(imagePath.string());
    ASSERT_TRUE(second.has_value()) << second.error().ToString();
    EXPECT_EQ(*second, *first);
    EXPECT_EQ(imageManager.pixelCacheEntryCount(), 1U);
    EXPECT_EQ(imageManager.pixelCacheByteSize(), 8U);

    imageManager.clearPixels();
    EXPECT_EQ(imageManager.pixelCacheEntryCount(), 0U);
    EXPECT_EQ(imageManager.pixelCacheByteSize(), 0U);

    std::filesystem::remove(imagePath);
}

}  // namespace
}  // namespace ui::tests