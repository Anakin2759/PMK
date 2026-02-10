/**
 * ************************************************************************
 *
 * @file SendMessageToChatResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 发送聊天消息响应
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"
#include <cstdint>
#include <string>

struct SendMessageToChatResponse : public MessageBase<SendMessageToChatResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::SEND_MESSAGE_RESP;

    uint32_t sender = 0;
    std::string chatMessage;

    void writeTo(shared::PacketWriter& writer) const
    {
        writer.writeUint32(sender);
        writer.writeString(chatMessage);
    }

    void readFrom(shared::PacketReader& reader)
    {
        sender = reader.readUint32();
        chatMessage = reader.readString();
    }

    [[nodiscard]] nlohmann::json toJsonImpl() const { return {{"sender", sender}, {"chatMessage", chatMessage}}; }
};