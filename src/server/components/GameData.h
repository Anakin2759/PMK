/**
 * ************************************************************************
 *
 * @file GameData.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-19
 * @version 0.1
 * @brief 游戏数据结构定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include "src/shared/common/Common.h"

constexpr static int RESPONSE_TIME = 10; // 默认响应时间10秒
struct GameData
{
    TurnPhase currentPhase = TurnPhase::START;
    GameMode mode = GameMode::CHANLLENGE_PEST;
    entt::entity currentPlayer = entt::null;
    uint32_t round = 0;
    uint8_t responseTime = RESPONSE_TIME;
};