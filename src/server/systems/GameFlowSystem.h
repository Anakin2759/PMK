/**
 * ************************************************************************
 *
 * @file GameFlowSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.1
 * @brief 游戏流程系统定义
    处理游戏的整体流程控制
    包括游戏开始、回合切换、游戏结束等
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "absl/container/flat_hash_map.h"
#include "entt/signal/fwd.hpp"
#include "src/server/context/GameContext.h"
#include "src/server/events/GameFlowEvents.h"
#include "src/server/events/DeckEvents.h"
#include "src/shared/utils/RoundRobin.h"
#include "src/shared/common/Common.h"
#include "src/server/components/Player.h"

class GameFlowSystem : public EnableRegister<GameFlowSystem>
{
public:
    static constexpr int MAX_PLAYERS = 8;
    explicit GameFlowSystem(GameContext& context) : m_context(&context)
    {
        m_phaseHandlers[TurnPhase::GAME_START] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleGameStart>, this};
        m_phaseHandlers[TurnPhase::START] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleStartPhase>, this};
        m_phaseHandlers[TurnPhase::JUDGE] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleJudgePhase>, this};
        m_phaseHandlers[TurnPhase::DRAW] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleDrawPhase>, this};
        m_phaseHandlers[TurnPhase::PLAY] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handlePlayPhase>, this};
        m_phaseHandlers[TurnPhase::DISCARD] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleDiscardPhase>, this};
        m_phaseHandlers[TurnPhase::END] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleEndPhase>, this};
        m_phaseHandlers[TurnPhase::GAME_OVER] =
            entt::delegate<void()>{entt::connect_arg<&GameFlowSystem::handleGameOver>, this};
    };

private:
    void registerEventsImpl()
    {
        m_context->dispatcher.sink<events::GameStart>().connect<&GameFlowSystem::onGameStart>(this);
    };
    void unregisterEventsImpl()
    {
        m_context->dispatcher.sink<events::GameStart>().disconnect<&GameFlowSystem::onGameStart>(this);
    };
    void onGameStart(const events::GameStart& event)
    {
        // 处理游戏开始事件的逻辑
        for (auto player : m_playerQueue)
        {
        }
    };

    void onGameEnd(const events::GameEnd& event) {
        // 处理游戏结束事件的逻辑
    };
    void onNextTurn(const events::NextTurn& event) {
        // 处理下一回合事件的逻辑

    };

    void onLogin()
    {
        // // 处理玩家登录事件的逻辑
        // m_playerQueue.push_back(CreatePlayer(m_context->registry,
        //                                      MetaPlayerInfo{.playerName = "Player1", .playerID = 1},
        //                                      CharacterInfo{.characterCard = entt::null},
        //                                      HandCards{},
        //                                      Equipments{},
        //                                      LiveStatus{true}));
    };

    /**
     * @brief 执行当前阶段的处理逻辑
     */
    void executeCurrentPhase()
    {
        if (auto iter = m_phaseHandlers.find(m_currentPhase); iter != m_phaseHandlers.end())
        {
            iter->second();
        }
    }

    /**
     * @brief 切换到下一个阶段
     * @param nextPhase 下一个阶段
     */
    void transitionToPhase(TurnPhase nextPhase)
    {
        m_context->logger->info("阶段切换: {} -> {}", static_cast<int>(m_currentPhase), static_cast<int>(nextPhase));
        m_currentPhase = nextPhase;
        executeCurrentPhase();
    }

    // ========== 各阶段处理函数 ==========

    /**
     * @brief 处理游戏开始阶段
     */
    void handleGameStart()
    {
        m_context->logger->info("游戏开始");
        // 初始化玩家队列、发初始手牌等
        for (const auto& player : m_playerQueue)
        {
            auto& handCards = m_context->registry.get<HandCards>(player).handCards;
            m_context->dispatcher;
            m_context->dispatcher.trigger<events::DealCards>({.player = player, .count = 4});
        }
        transitionToPhase(TurnPhase::START);
    }
    /**
     * @brief 处理回合开始阶段
     */
    void handleStartPhase()
    {
        entt::entity currentPlayer = m_playerQueue.current();
        m_context->logger->info("回合开始 - 玩家: {}", entt::to_integral(currentPlayer));
        // 触发回合开始事件
        transitionToPhase(TurnPhase::JUDGE);
    }

    /**
     * @brief 处理判定阶段
     */
    void handleJudgePhase()
    {
        m_context->logger->info("判定阶段");
        // 处理延时锦囊判定
        transitionToPhase(TurnPhase::DRAW);
    }

    /**
     * @brief 处理摸牌阶段
     */
    void handleDrawPhase()
    {
        entt::entity currentPlayer = m_playerQueue.current();
        m_context->logger->info("摸牌阶段 - 玩家: {}", entt::to_integral(currentPlayer));
        // 触发摸牌事件
        transitionToPhase(TurnPhase::PLAY);
    }

    /**
     * @brief 处理出牌阶段
     */
    void handlePlayPhase()
    {
        m_context->logger->info("出牌阶段");
        // 等待玩家操作，玩家主动结束出牌阶段
        // 此阶段通常不自动切换，由玩家行动触发
    }

    /**
     * @brief 处理弃牌阶段
     */
    void handleDiscardPhase()
    {
        entt::entity currentPlayer = m_playerQueue.current();
        m_context->logger->info("弃牌阶段 - 玩家: {}", entt::to_integral(currentPlayer));
        // 检查手牌数量，超过体力值则需要弃牌
        transitionToPhase(TurnPhase::END);
    }

    /**
     * @brief 处理回合结束阶段
     */
    void handleEndPhase()
    {
        entt::entity currentPlayer = m_playerQueue.current();
        m_context->logger->info("回合结束 - 玩家: {}", entt::to_integral(currentPlayer));
        // 清理回合状态
        m_playerQueue.next();
        transitionToPhase(TurnPhase::START);
    }

    /**
     * @brief 处理游戏结束阶段
     */
    void handleGameOver()
    {
        m_context->logger->info("游戏结束");
        // 清理资源，显示结算信息
    }

    GameContext* m_context;
    utils::RoundRobin<entt::entity> m_playerQueue;
    TurnPhase m_currentPhase{TurnPhase::GAME_START};
    absl::flat_hash_map<TurnPhase, entt::delegate<void()>> m_phaseHandlers;
};