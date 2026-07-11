// Tests for ui::Result<T> + ui::Error + ui::UiErrc.
// 错误载体为 ui::Error（错误码 + 上下文 + source_location），见 ui/Result.hpp。

#include <format>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "ui/ErrorCodes.hpp"
#include "ui/Result.hpp"

namespace
{

// ---------- Result<T> 成功路径 ----------
TEST(ResultTest, ValueSuccess)
{
    ui::Result<int> result = 42;
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(*result, 42);
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultTest, OkFactoryDeducesType)
{
    auto okResult = ui::Ok(7);
    static_assert(std::is_same_v<decltype(okResult), ui::Result<int>>);
    ASSERT_TRUE(okResult.has_value());
    EXPECT_EQ(*okResult, 7);
}

// ---------- Result<T> 失败路径 ----------
TEST(ResultTest, ValueFailure)
{
    ui::Result<int> result = ui::Err(ui::UiErrc::INVALID_ARGUMENT);
    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error(), ui::UiErrc::INVALID_ARGUMENT);
    EXPECT_EQ(result.error().code, ui::UiErrc::INVALID_ARGUMENT);
}

// ---------- Result<void> 成功 + 失败 ----------
TEST(ResultVoidTest, OkSuccess)
{
    ui::Result<void> const result = ui::Ok();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST(ResultVoidTest, Failure)
{
    ui::Result<void> result = ui::Err(ui::UiErrc::THEME_NOT_FOUND);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ui::UiErrc::THEME_NOT_FOUND);
}

// ---------- Error 语义 ----------
TEST(ErrorTest, CapturesSourceLocationAtCallSite)
{
    const auto unexpectedError = ui::Err(ui::UiErrc::ASSET_NOT_FOUND);
    const ui::Error& error = unexpectedError.error();
    EXPECT_EQ(error.code, ui::UiErrc::ASSET_NOT_FOUND);
    // origin 应指向本测试文件而非 Result.hpp。
    EXPECT_NE(std::string_view(error.origin.file_name()).find("test_result.cpp"), std::string_view::npos);
}

TEST(ErrorTest, CarriesContext)
{
    const auto unexpectedError = ui::Err(ui::UiErrc::ASSET_NOT_FOUND, "fonts/missing.ttf");
    EXPECT_EQ(unexpectedError.error().context, "fonts/missing.ttf");
}

TEST(ErrorTest, PropagationPreservesOrigin)
{
    const auto makeInner = []() -> ui::Result<int> { return ui::Err(ui::UiErrc::ASSET_LOAD_FAILED, "inner"); };
    const auto makeOuter = [&]() -> ui::Result<std::string>
    {
        auto inner = makeInner();
        if (!inner)
        {
            return ui::Err(inner.error()); // 传播已有 Error
        }
        return std::string("ok");
    };

    const auto outer = makeOuter();
    ASSERT_FALSE(outer.has_value());
    EXPECT_EQ(outer.error().code, ui::UiErrc::ASSET_LOAD_FAILED);
    EXPECT_EQ(outer.error().context, "inner");
}

TEST(ErrorTest, ToStringContainsCodeAndContext)
{
    const auto unexpectedError = ui::Err(ui::UiErrc::ASSET_DECODE_FAILED, "image.png");
    const std::string text = unexpectedError.error().ToString();
    EXPECT_NE(text.find("asset_decode_failed"), std::string::npos);
    EXPECT_NE(text.find("image.png"), std::string::npos);
    EXPECT_NE(text.find("test_result.cpp"), std::string::npos);
}

// ---------- TRY 宏 ----------
TEST(TryMacroTest, DeclaresVariableAndPropagates)
{
    const auto success = []() -> ui::Result<int> { return 5; };
    const auto failure = []() -> ui::Result<int> { return ui::Err(ui::UiErrc::INVALID_ARGUMENT); };

    const auto chainOk = [&]() -> ui::Result<int>
    {
        TRY(auto value, success());
        return value * 2;
    };
    const auto chainFail = [&]() -> ui::Result<int>
    {
        TRY(auto value, failure());
        return value * 2;
    };

    ASSERT_TRUE(chainOk().has_value());
    EXPECT_EQ(*chainOk(), 10);
    ASSERT_FALSE(chainFail().has_value());
    EXPECT_EQ(chainFail().error(), ui::UiErrc::INVALID_ARGUMENT);
}

TEST(TryMacroTest, TryVoidPropagates)
{
    const auto failure = []() -> ui::Result<void> { return ui::Err(ui::UiErrc::THEME_NOT_FOUND); };
    const auto chain = [&]() -> ui::Result<void>
    {
        TRY_VOID(failure());
        return ui::Ok();
    };

    ASSERT_FALSE(chain().has_value());
    EXPECT_EQ(chain().error(), ui::UiErrc::THEME_NOT_FOUND);
}

// ---------- std::formatter ----------
TEST(FormatterTest, EnumPrintsName)
{
    const std::string text = std::format("{}", ui::UiErrc::ASSET_DECODE_FAILED);
    EXPECT_EQ(text, "asset_decode_failed");

    const std::string prefixedText = std::format("err={}", ui::UiErrc::INVALID_ARGUMENT);
    EXPECT_EQ(prefixedText, "err=invalid_argument");
}

TEST(FormatterTest, ErrorFormatsAsToString)
{
    const auto unexpectedError = ui::Err(ui::UiErrc::THEME_TYPE_MISMATCH, "palette");
    EXPECT_EQ(std::format("{}", unexpectedError.error()), unexpectedError.error().ToString());
}

} // namespace
