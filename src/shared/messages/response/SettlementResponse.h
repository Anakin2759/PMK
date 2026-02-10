/**
 * ************************************************************************
 *
 * @file SettlementResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 结算响应消息
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

struct SettlementResponse : public MessageBase<SettlementResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::SETTLEMENT_RESP;

    uint32_t player;     // 发起结算的玩家
    uint32_t card;       // 结算的卡牌
    uint32_t target;     // 结算目标
    bool success;        // 结算是否成功
    std::string message; // 结算结果描述

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"player", player}, {"card", card}, {"target", target}, {"success", success}, {"message", message}};
    }

    static std::expected<SettlementResponse, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            SettlementResponse resp;
            resp.player = json.at("player").get<uint32_t>();
            resp.card = json.at("card").get<uint32_t>();
            resp.target = json.at("target").get<uint32_t>();
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

    static std::expected<SettlementResponse, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};
