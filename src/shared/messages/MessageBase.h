/**
 * ************************************************************************
 *
 * @file MessageBase.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-23
 * @version 0.2
 * @brief 消息基类，提供统一的序列化接口
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <vector>
#include <expected>
#include <cstdint>
#include <span>
#include <nlohmann/json.hpp>
#include "src/shared/common/PacketStream.h"

enum class MessageError
{
    BufferTooSmall,
    InvalidFormat,
    DeserializeFailed
};

/**
 * @brief 消息基类，提供统一的序列化接口
 * @tparam Derived 派生类类型
 */
template <typename Derived>
struct MessageBase
{
    // 序列化
    std::vector<uint8_t> serialize() const
    {
        shared::PacketWriter writer;
        static_cast<const Derived*>(this)->writeTo(writer);
        return writer.buffer;
    }

    // 反序列化
    static std::expected<Derived, MessageError> deserialize(std::span<const uint8_t> data)
    {
        shared::PacketReader reader(data);
        Derived msg;
        try
        {
            msg.readFrom(reader);
        }
        catch (...)
        {
            return std::unexpected(MessageError::InvalidFormat);
        }
        return msg;
    }

    // Json 接口用于调试
    [[nodiscard]] nlohmann::json toJson() const { return static_cast<const Derived*>(this)->toJsonImpl(); }
};
