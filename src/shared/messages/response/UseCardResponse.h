/**
 * ************************************************************************
 *
 * @file UseCardResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 使用卡牌响应消息
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
#include <vector>
#include <string>

struct UseCardResponse : public MessageBase<UseCardResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::USE_CARD_RESP;
    uint32_t player;               // 使用卡牌的玩家
    uint32_t card;                 // 使用的卡牌
    std::vector<uint32_t> targets; // 目标列表
    bool success;                  // 是否成功使用
    std::string message;           // 附加消息

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"player", player}, {"card", card}, {"targets", targets}, {"success", success}, {"message", message}};
    }

    static std::expected<UseCardResponse, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            UseCardResponse resp;
            resp.player = json.at("player").get<uint32_t>();
            resp.card = json.at("card").get<uint32_t>();
            resp.targets = json.at("targets").get<std::vector<uint32_t>>();
            resp.success = json.at("success").get<bool>();
            resp.message = json.at("message").get<std::string>();
            return resp;
        }
        catch (...)
        {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }

    std::expected<std::span<uint8_t>, MessageError> serializeImpl(std::span<uint8_t>) const
    {
        return std::unexpected(MessageError::SerializeFailed);
    }

    static std::expected<UseCardResponse, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};
