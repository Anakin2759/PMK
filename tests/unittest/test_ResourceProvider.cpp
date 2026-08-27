#include <gtest/gtest.h>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"
#include "src/managers/ResourceProvider.hpp"
#include "src/utils/Logger.hpp"

namespace ui::tests
{

TEST(UiCoverageTest, BinaryResourceDefaultStateIsEmpty)
{
    const managers::BinaryResource resource{};

    EXPECT_TRUE(resource.empty());
    EXPECT_EQ(resource.data(), nullptr);
    EXPECT_EQ(resource.size(), 0U);
    EXPECT_FALSE(static_cast<bool>(resource));
}

TEST(UiCoverageTest, DefaultResourceProviderLoadsEmbeddedFont)
{
    utils::Logger logger;
    const auto provider = managers::GetDefaultUiResourceProvider(logger);

    ASSERT_NE(provider, nullptr);
    EXPECT_TRUE(provider->exists("assets/fonts/NotoSansSC-VariableFont_wght.ttf"));

    const auto resource = provider->loadBinary("assets/fonts/NotoSansSC-VariableFont_wght.ttf");
    ASSERT_TRUE(resource.has_value()) << resource.error().ToString();
    EXPECT_TRUE(static_cast<bool>(resource.value()));
    EXPECT_GT(resource->size(), 0U);
    EXPECT_NE(resource->data(), nullptr);
}

TEST(UiCoverageTest, DefaultResourceProviderLoadsEmbeddedIconCodepoints)
{
    utils::Logger logger;
    const auto provider = managers::GetDefaultUiResourceProvider(logger);

    ASSERT_NE(provider, nullptr);
    EXPECT_TRUE(provider->exists("assets/icons/MaterialSymbolsRounded[FILL,GRAD,opsz,wght].codepoints"));

    const auto resource = provider->loadBinary("assets/icons/MaterialSymbolsRounded[FILL,GRAD,opsz,wght].codepoints");
    ASSERT_TRUE(resource.has_value()) << resource.error().ToString();
    EXPECT_GT(resource->size(), 0U);
}

TEST(UiCoverageTest, DefaultResourceProviderReportsMissingResource)
{
    // 失败路径会通过 UiRuntime::current().logger() 记录日志，需要活动的 UiRuntime。
    UiRuntime runtime;
    UiRuntimeScope const scope(runtime);

    utils::Logger logger;
    const auto provider = managers::GetDefaultUiResourceProvider(logger);

    ASSERT_NE(provider, nullptr);
    EXPECT_FALSE(provider->exists("assets/does-not-exist.bin"));

    const auto resource = provider->loadBinary("assets/does-not-exist.bin");
    ASSERT_FALSE(resource.has_value());
    EXPECT_NE(static_cast<int>(resource.error().code), 0);
    EXPECT_FALSE(resource.error().ToString().empty());
}

}  // namespace ui::tests