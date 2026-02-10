
#pragma once
#include <entt/entt.hpp>
#include "src/shared/common/Common.h"

namespace events
{
struct DealCards // 发牌事件
{
    entt::entity player; // 发牌角色
    uint8_t count;       // 发牌数量
};

struct FindCardInDrawPile
{
    std::string cardName; // 需要查找的牌名
    SuitType suitType;    // 需要查找的花色
    uint8_t rank;         // 需要查找的点数
};

struct FindCardInHandCardsArea
{
    std::string cardName; // 需要查找的牌名
    SuitType suitType;    // 需要查找的花色
    uint8_t rank;         // 需要查找的点数
};

struct FindCardInEquipmentArea
{
    std::string cardName; // 需要查找的牌名
    SuitType suitType;    // 需要查找的花色
    uint8_t rank;         // 需要查找的点数
};

struct FindCardInDiscardPile
{
    std::string cardName; // 需要查找的牌名
    SuitType suitType;    // 需要查找的花色
    uint8_t rank;         // 需要查找的点数
};

struct FindCardinAllAreas
{
    std::string cardName; // 需要查找的牌名
    SuitType suitType;    // 需要查找的花色
    uint8_t rank;         // 需要查找的点数
};

struct PickRandomCardFromSomeone
{
    entt::entity fromPlayer; // 被抽牌的角色
    std::string cardName;    // 需要查找的牌名
    SuitType suitType;       // 需要查找的花色
    uint8_t rank;            // 需要查找的点数
};

struct PickRandomCardFromSomeonesHand
{
    entt::entity fromPlayer; // 被抽牌的角色
    std::string cardName;    // 需要查找的牌名
    SuitType suitType;       // 需要查找的花色
    uint8_t rank;            // 需要查找的点数
};

struct ShuffleDeck
{
};

} // namespace events