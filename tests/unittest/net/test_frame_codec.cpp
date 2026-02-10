/**
 * ************************************************************************
 *
 * @file test_frame_codec.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 帧编解码器单元测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <gtest/gtest.h>
#include "src/net/protocol/FrameCodec.h"
#include "src/net/protocol/FrameHeader.h"
#include <vector>
#include <array>

class FrameCodecTest : public ::testing::Test
{
protected:
    std::vector<uint8_t> m_buffer;

    void SetUp() override
    {
        m_buffer.resize(65536); // 64KB buffer
    }
};

// 测试 1: 编码有效数据
TEST_F(FrameCodecTest, EncodeValidData)
{
    std::array<uint8_t, 4> payload = {0x01, 0x02, 0x03, 0x04};
    uint16_t cmd = 0x1234;

    auto result = encodeFrame(m_buffer, cmd, payload);

    ASSERT_TRUE(result.has_value());
    auto encoded = result.value();

    EXPECT_EQ(encoded.size(), sizeof(FrameHeader) + payload.size());

    // 验证魔数
    auto* header = reinterpret_cast<const FrameHeader*>(encoded.data());
    EXPECT_EQ(header->magic, FRAME_MAGIC);
    EXPECT_EQ(header->cmd, cmd);
    EXPECT_EQ(header->length, payload.size());
}

// 测试 2: 编码空数据
TEST_F(FrameCodecTest, EncodeEmptyData)
{
    std::vector<uint8_t> emptyPayload;
    uint16_t cmd = 0x5678;

    auto result = encodeFrame(m_buffer, cmd, emptyPayload);

    ASSERT_TRUE(result.has_value());
    auto encoded = result.value();
    EXPECT_EQ(encoded.size(), sizeof(FrameHeader));
}

// 测试 3: 编码大数据
TEST_F(FrameCodecTest, EncodeLargeData)
{
    std::vector<uint8_t> largePayload(10240, 0xFF);
    uint16_t cmd = 0xABCD;

    auto result = encodeFrame(m_buffer, cmd, largePayload);

    ASSERT_TRUE(result.has_value());
    auto encoded = result.value();
    EXPECT_EQ(encoded.size(), sizeof(FrameHeader) + largePayload.size());

    auto* header = reinterpret_cast<const FrameHeader*>(encoded.data());
    EXPECT_EQ(header->length, largePayload.size());
}

// 测试 4: 解码有效数据
TEST_F(FrameCodecTest, DecodeValidData)
{
    std::array<uint8_t, 4> originalPayload = {0xAA, 0xBB, 0xCC, 0xDD};
    uint16_t cmd = 0x9999;

    auto encodeResult = encodeFrame(m_buffer, cmd, originalPayload);
    ASSERT_TRUE(encodeResult.has_value());

    auto decodeResult = decodeFrame(encodeResult.value());

    ASSERT_TRUE(decodeResult.has_value());
    EXPECT_EQ(decodeResult->cmd, cmd);
    EXPECT_EQ(decodeResult->payload.size(), originalPayload.size());
    EXPECT_TRUE(std::equal(decodeResult->payload.begin(), decodeResult->payload.end(), originalPayload.begin()));
}

// 测试 5: 解码不完整数据
TEST_F(FrameCodecTest, DecodeIncompleteData)
{
    std::array<uint8_t, 4> originalPayload = {0x11, 0x22, 0x33, 0x44};
    uint16_t cmd = 0x1111;

    auto encodeResult = encodeFrame(m_buffer, cmd, originalPayload);
    ASSERT_TRUE(encodeResult.has_value());

    // 只提供部分数据（只有头部的一半）
    auto partial = encodeResult->subspan(0, 3);
    auto decodeResult = decodeFrame(partial);

    EXPECT_FALSE(decodeResult.has_value());
    EXPECT_EQ(decodeResult.error(), CodecError::BufferTooSmall);
}

// 测试 6: 解码无效魔数
TEST_F(FrameCodecTest, DecodeInvalidMagic)
{
    std::array<uint8_t, 10> invalidData = {0};

    // 设置错误的魔数
    auto* badHeader = reinterpret_cast<FrameHeader*>(invalidData.data());
    badHeader->magic = 0xDEAD; // 错误的魔数
    badHeader->cmd = 1;
    badHeader->length = 4;

    auto result = decodeFrame(invalidData);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CodecError::InvalidMagic);
}

// 测试 7: 解码载荷不完整
TEST_F(FrameCodecTest, DecodeIncompletePayload)
{
    std::array<uint8_t, 4> originalPayload = {0x55, 0x66, 0x77, 0x88};
    uint16_t cmd = 0x2222;

    auto encodeResult = encodeFrame(m_buffer, cmd, originalPayload);
    ASSERT_TRUE(encodeResult.has_value());

    // 截断载荷，只保留头部和部分载荷
    auto incomplete = encodeResult->subspan(0, sizeof(FrameHeader) + 2);
    auto decodeResult = decodeFrame(incomplete);

    EXPECT_FALSE(decodeResult.has_value());
    EXPECT_EQ(decodeResult.error(), CodecError::IncompletePayload);
}

// 测试 8: 编码解码往返
TEST_F(FrameCodecTest, EncodeDecodeRoundTrip)
{
    for (size_t size : {0, 1, 10, 100, 1000, 10000})
    {
        std::vector<uint8_t> original(size);
        for (size_t i = 0; i < size; i++)
        {
            original[i] = static_cast<uint8_t>(i & 0xFF);
        }

        uint16_t cmd = static_cast<uint16_t>(size & 0xFFFF);

        auto encodeResult = encodeFrame(m_buffer, cmd, original);
        ASSERT_TRUE(encodeResult.has_value());

        auto decodeResult = decodeFrame(encodeResult.value());

        ASSERT_TRUE(decodeResult.has_value());
        EXPECT_EQ(decodeResult->cmd, cmd);
        EXPECT_TRUE(std::equal(decodeResult->payload.begin(), decodeResult->payload.end(), original.begin()));
    }
}

// 测试 9: 缓冲区太小
TEST_F(FrameCodecTest, BufferTooSmall)
{
    std::array<uint8_t, 4> payload = {1, 2, 3, 4};
    std::array<uint8_t, 4> tinyBuffer; // 太小的缓冲区

    auto result = encodeFrame(tinyBuffer, 0x1234, payload);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CodecError::BufferTooSmall);
}

// 测试 10: 最大有效负载
TEST_F(FrameCodecTest, MaxPayloadSize)
{
    const size_t maxSize = 65535; // uint16_t max
    std::vector<uint8_t> maxPayload(maxSize, 0xAB);
    m_buffer.resize(maxSize + sizeof(FrameHeader));

    auto encodeResult = encodeFrame(m_buffer, 0xFFFF, maxPayload);
    ASSERT_TRUE(encodeResult.has_value());

    auto decodeResult = decodeFrame(encodeResult.value());
    ASSERT_TRUE(decodeResult.has_value());
    EXPECT_EQ(decodeResult->payload.size(), maxSize);
}
