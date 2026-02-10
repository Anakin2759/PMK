/**
 * ************************************************************************
 *
 * @file Player.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-18
 * @version 0.1
 * @brief 玩家定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <string>
#include <cstdint>
#include "src/shared/common/Common.h"

constexpr uint32_t DEFAULT_PLAYER_ID = 0xFFFFFFFF;
struct MetaPlayerInfo
{
    std::string playerName = "000";
    uint32_t playerID = DEFAULT_PLAYER_ID;
};

struct CharacterInfo
{
    entt::entity characterCard = entt::null; // 角色卡牌实体
};

struct HandCards
{
    std::vector<entt::entity> handCards;
};

struct Equipments
{
    entt::entity weapon = entt::null;
    entt::entity armor = entt::null;
    entt::entity attackhorse = entt::null;
    entt::entity defensehorse = entt::null;
};

struct Identity
{
    IdentityType type = IdentityType::MEMBER;
};

struct LiveStatus
{
    bool isAlive = true;
};

inline entt::entity CreatePlayer(entt::registry& registry,
                                 MetaPlayerInfo& metaInfo,
                                 CharacterInfo& characterInfo,
                                 HandCards& handCards,
                                 Equipments& equipments,LiveStatus& liveStatus)
{
    entt::entity player = registry.create();
    registry.emplace<MetaPlayerInfo>(player, metaInfo);
    registry.emplace<CharacterInfo>(player, characterInfo);
    registry.emplace<HandCards>(player, handCards);
    registry.emplace<Equipments>(player, equipments);
    registry.emplace<LiveStatus>(player, liveStatus);
    return player;
}
