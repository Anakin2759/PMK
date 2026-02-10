/**
 * ************************************************************************
 *
 * @file UseCardSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief 处理卡牌使用的系统,通过监听全局事件实现卡牌的使用逻辑
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/server/components/Player.h"
#include "src/server/context/GameContext.h"
#include "src/server/events/Events.h"
#include "src/server/components/Card.h"

class UseCardSystem
{
public:
    explicit UseCardSystem(GameContext& context) : m_context(&context) {};
    void registerEvents() { m_context->dispatcher.sink<events::CardUsed>().connect<&UseCardSystem::onCardUsed>(this); };
    void unregisterEvents()
    {
        m_context->dispatcher.sink<events::CardUsed>().disconnect<&UseCardSystem::onCardUsed>(this);
    };

private:
    void onCardUsed(const events::CardUsed& event)
    {
        auto [user, target, card] = event;

        if (m_context->registry.all_of<CardEffect>(card))
        {
            auto& cet = m_context->registry.get<CardEffect>(card);

            cet.apply(user, target, m_context->registry); // 执行 delegate
        }
        auto& handCards = m_context->registry.get<HandCards>(user).handCards;
        std::erase(handCards, card);
    }

    void onCardShown(const events::CardShown& event)
    {
        auto [user, card] = event;
        m_context->logger->info("角色 {} 展示了卡牌 {}", static_cast<int>(user), static_cast<int>(card));
    }

    GameContext* m_context;
};