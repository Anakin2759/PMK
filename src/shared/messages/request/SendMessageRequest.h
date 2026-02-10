/**
 * ************************************************************************
 *
 * @file SendMessageRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-23
 * @version 0.2
 * @brief 发送聊天消息请求
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"
#include <string>

struct SendMessageRequest : public MessageBase<SendMessageRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::SEND_MESSAGE_REQ;

    uint32_t channelId = 0; // 0: Global, 1: Room, etc.
    std::string content;

    void writeTo(shared::PacketWriter& writer) const
    {
        writer.writeUint32(channelId);
        writer.writeString(content);
    }

    void readFrom(shared::PacketReader& reader)
    {
        channelId = reader.readUint32();
        content = reader.readString();
    }

    [[nodiscard]] nlohmann::json toJsonImpl() const { return {{"channelId", channelId}, {"content", content}}; }
};
