/**
 * ************************************************************************
 *
 * @file CreateRoomRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-23
 * @version 0.2
 * @brief 创建房间请求消息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"

struct CreateRoomRequest : public MessageBase<CreateRoomRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::CREATE_ROOM_REQ;

    std::string roomName;
    uint8_t maxPlayers = 4;
    std::string password; // 可为空

    void writeTo(shared::PacketWriter& writer) const
    {
        writer.writeString(roomName);
        writer.writeUint8(maxPlayers);
        writer.writeString(password);
    }

    void readFrom(shared::PacketReader& reader)
    {
        roomName = reader.readString();
        maxPlayers = reader.readUint8();
        password = reader.readString();
    }

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"roomName", roomName}, {"maxPlayers", maxPlayers}, {"password", password}};
    }
};
