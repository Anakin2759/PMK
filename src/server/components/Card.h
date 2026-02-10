/**
 * ************************************************************************
 *
 * @file Card.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-19
 * @version 0.1
 * @brief
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <entt/entt.hpp>
#include <span> // 需要包含 span
#include "src/shared/common/Common.h"

// --------------------------------------------------------------------------
// 1. 卡牌组件定义 (Component: Data Only)
// --------------------------------------------------------------------------

struct MetaCardInfo
{
    std::string name;
    std::string description;
    CardType type = CardType::BASIC;
};

struct CardCost
{
    uint32_t mana = 0;
    uint32_t energy = 0;
    uint32_t health = 0;
};
struct CardTarget
{
    bool needTarget = true;
    uint8_t maxTargets = 1;
    uint8_t minTargets = 1;
    uint8_t range = 0; // 0表示无限制

    std::function<bool(entt::entity)> filter; // 目标筛选规则
};

struct CardPointAndSuit
{
    uint8_t point = 0;
    SuitType suit = SuitType::JOKER; // JOKER表示无花色
};

// **已删除 CardEffect 结构体，逻辑将由 EffectSystem 处理**

// --------------------------------------------------------------------------
// 2. 卡牌类型标签 (Tag: Data Only)
// --------------------------------------------------------------------------

struct BasicCardTypeTag
{
    BasicCardType type;
};

struct StrategyCardTypeTag
{
    StrategyCardType type;
};

struct EquipCardTypeTag
{
    EquipCardType type;
};

// --------------------------------------------------------------------------
// 3. 实体创建函数 (Factory: Component Assembly)
// --------------------------------------------------------------------------

inline entt::entity CreateCard(entt::registry& reg,
                               const MetaCardInfo& metaInfo,
                               const CardCost& cost,
                               const CardTarget& target,
                               const CardPointAndSuit& pointAndSuit)
{
    auto ent = reg.create();
    reg.emplace<MetaCardInfo>(ent, metaInfo);
    reg.emplace<CardCost>(ent, cost);
    reg.emplace<CardTarget>(ent, target);
    reg.emplace<CardPointAndSuit>(ent, pointAndSuit);
    // **已删除 CardEffect 挂载**
    return ent;
}

inline entt::entity CreateBasicCard(entt::registry& reg,
                                    const MetaCardInfo& metaInfo,
                                    const CardCost& cost,
                                    const CardTarget& target,
                                    const CardPointAndSuit& pointAndSuit,
                                    BasicCardType basicType)
{
    auto ent = CreateCard(reg, metaInfo, cost, target, pointAndSuit);
    reg.emplace<BasicCardTypeTag>(ent, BasicCardTypeTag{basicType});
    return ent;
}

inline entt::entity CreateStrategyCard(entt::registry& reg,
                                       const MetaCardInfo& metaInfo,
                                       const CardCost& cost,
                                       const CardTarget& target,
                                       const CardPointAndSuit& pointAndSuit,
                                       StrategyCardType strategyType)
{
    auto ent = CreateCard(reg, metaInfo, cost, target, pointAndSuit);
    reg.emplace<StrategyCardTypeTag>(ent, StrategyCardTypeTag{strategyType});
    return ent;
}

inline entt::entity CreateEquipCard(entt::registry& reg,
                                    const MetaCardInfo& metaInfo,
                                    const CardCost& cost,
                                    const CardTarget& target,
                                    const CardPointAndSuit& pointAndSuit,
                                    EquipCardType equipType)
{
    auto ent = CreateCard(reg, metaInfo, cost, target, pointAndSuit);
    reg.emplace<EquipCardTypeTag>(ent, EquipCardTypeTag{equipType});
    return ent;
}

// --------------------------------------------------------------------------
// 4. 具体卡牌创建函数 (Factory: Specific Cards)
// --------------------------------------------------------------------------

/**
 * @brief 创建杀卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateStrickCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{
        .needTarget = true,
        .maxTargets = 1,
        .minTargets = 1,
        .range = 1,
    };
    CardCost cost{};

    // **Effect 逻辑已移除**

    return CreateBasicCard(reg,
                           {.name = "杀", .description = "需要使用一张闪否则造成一点伤害", .type = CardType::BASIC},
                           cost,
                           target,
                           pointAndSuit,
                           BasicCardType::STRIKE);
}

/**
 * @brief 创建闪卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateDodgeCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{.needTarget = false, .maxTargets = 0, .minTargets = 0, .range = 0};
    MetaCardInfo metaInfo{.name = "闪", .description = "用于抵消一张杀的伤害", .type = CardType::BASIC};
    CardCost cost{};

    // **Effect 逻辑已移除**

    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, BasicCardType::DODGE);
}

inline entt::entity CreatePeachCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    MetaCardInfo metaInfo{.name = "桃", .description = "回复一点体力", .type = CardType::BASIC};
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0; // 无距离限制
    CardCost cost{};

    // **Effect 逻辑已移除**

    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, BasicCardType::PEACH);
}

/**
 * @brief 创建酒卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateAlcoholCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0; // 无距离限制

    // **修复：MetaInfo -> MetaCardInfo**
    MetaCardInfo metaInfo{.name = "酒",
                          .description = "回合内使用后，下一次受到的伤害-1（至少为1）,濒死状态下使用可回复1点体力",
                          .type = CardType::BASIC};
    CardCost cost{};

    // **Effect 逻辑已移除**

    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, BasicCardType::ALCOHOL);
}

/**
 * @brief 创建火攻卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateFireAttackCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0;

    // **修复：MetaInfo -> MetaCardInfo**
    MetaCardInfo metaInfo{.name = "火攻",
                          .description = "对目标角色造成一点火焰伤害，目标角色可以使用一张闪避来抵消伤害",
                          .type = CardType::STRATEGY};
    CardCost cost{};

    // **Effect 逻辑已移除**

    return CreateStrategyCard(reg, metaInfo, cost, target, pointAndSuit, StrategyCardType::FIRE_ATTACK);
}

/**
 * @brief 创建决斗卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateDuelCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{
        .needTarget = true,
        .maxTargets = 1,
        .minTargets = 1,

        .range = 0xFF, // 无距离限制
    };

    // **修复：MetaInfo -> MetaCardInfo**
    MetaCardInfo metaInfo{.name = "决斗",
                          .description = "与你指定的角色进行决斗，双方轮流出杀，未能出杀的一方受到一点伤害",
                          .type = CardType::STRATEGY};
    CardCost cost{};

    // **Effect 逻辑已移除**

    return CreateStrategyCard(reg, metaInfo, cost, target, pointAndSuit, StrategyCardType::DUEL);
}