/**
 * ************************************************************************
 *
 * @file Deck.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-19
 * @version 0.1
 * @brief 牌堆系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <vector>

struct Deck
{
    std::vector<entt::entity> drawPile;       // 抽牌堆
    std::vector<entt::entity> discardPile;    // 弃牌堆
    std::vector<entt::entity> processingArea; // 处理区
};
