/**
 * ************************************************************************
 *
 * @file CreateRoomResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 创建房间响应消息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"

struct CreateRoomResponse : public MessageBase<CreateRoomResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::CREATE_ROOM_RESP;

    uint32_t roomId = 0;   // 房间ID
    bool success = false;  // 是否成功
    uint8_t errorCode = 0; // 错误码

    static CreateRoomResponse createSuccess(uint32_t roomId)
    {
        CreateRoomResponse resp;
        resp.roomId = roomId;
        resp.success = true;
        resp.errorCode = 0;
        return resp;
    }

    static CreateRoomResponse createFailed(uint8_t errorCode)
    {
        CreateRoomResponse resp;
        resp.roomId = 0;
        resp.success = false;
        resp.errorCode = errorCode;
        return resp;
    }

    void writeTo(shared::PacketWriter& writer) const
    {
        writer.writeUint32(roomId);
        writer.writeBool(success);
        writer.writeUint8(errorCode);
    }

    void readFrom(shared::PacketReader& reader)
    {
        roomId = reader.readUint32();
        success = reader.readBool();
        errorCode = reader.readUint8();
    }

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"roomId", roomId}, {"success", success}, {"errorCode", errorCode}};
    }
};