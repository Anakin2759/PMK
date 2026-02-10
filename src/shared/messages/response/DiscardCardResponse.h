/**
 * ************************************************************************
 *
 * @file DiscardCardResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 弃牌响应消息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"
#include <vector>
#include <cstdint>

struct DiscardCardResponse : public MessageBase<DiscardCardResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::DISCARD_CARD_RESP;

    uint32_t player;
    std::vector<uint32_t> cardIndexs;

    [[nodiscard]] nlohmann::json toJsonImpl() const { return {{"player", player}, {"cardIndexs", cardIndexs}}; }

    static std::expected<DiscardCardResponse, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            DiscardCardResponse resp;
            resp.player = json.at("player").get<uint32_t>();
            resp.cardIndexs = json.at("cardIndexs").get<std::vector<uint32_t>>();
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

    static std::expected<DiscardCardResponse, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};