/**
 * ************************************************************************
 *
 * @file Identity.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-14
 * @version 0.1
 * @brief 身份定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <entt/entity/fwd.hpp>
#include <string>
#include <cstdint>
#include <entt/entt.hpp>
#include "src/shared/common/Common.h"

struct MetaCharacterInfo
{
    std::string name;
    uint32_t id;
    std::string tag;
};

struct Gender
{
    GenderType type;
};

struct Faction
{
    FactionType type;
};

struct Skills
{
    std::vector<entt::entity> skillList;
};

struct Attributes
{
    uint32_t maxHealth = 4;
    uint32_t attackPower = 1;
    uint32_t defense = 0;
    uint32_t attackRange = 1;
    uint32_t movement = 1;
    uint8_t rank = 0;
    uint8_t attackTimes = 1;
    int32_t currentHealth = 4;
    bool weaponAreaEnabled = true;
    bool armorAreaEnabled = true;
    bool attackhorseAreaEnabled = true;
    bool defensehorseAreaEnabled = true;
    bool isAlive = true;
};

struct Status
{
    uint8_t duration = 0;                      // 持续回合数，0表示永久
    TurnPhase appliedPhase = TurnPhase::START; // 状态应用的阶段
    StatusType type = StatusType::FLIP;        // 状态类型
};

struct StatusFlags
{
    std::vector<Status> statusList; // 角色状态列表
};

inline entt::entity
    CreateCharacter(entt::registry& reg, const MetaCharacterInfo& info, FactionType faction, const Skills& skills)
{
    auto ent = reg.create();

    reg.emplace<MetaCharacterInfo>(ent, info);
    reg.emplace<Faction>(ent, Faction{faction});
    reg.emplace<Attributes>(ent, Attributes{});
    reg.emplace<StatusFlags>(ent);
    reg.emplace<Skills>(ent, skills);

    return ent;
}
