/**
 * ************************************************************************
 *
 * @file DeckSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief 牌堆管理系统，负责管理发牌/洗牌/检索
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <absl/random/random.h>
#include <algorithm>
#include "src/server/context/GameContext.h"
#include "src/server/components/Deck.h"
#include "src/server/events/Events.h"
#include "src/server/components/Player.h"
#include "src/server/components/Card.h"
#include "src/server/events/DeckEvents.h"
#include "src/server/events/GameFlowEvents.h"
#include "src/server/Interface/ISystem.h"
#include "absl/container/flat_hash_set.h"

class DeckSystem : public EnableRegister<DeckSystem>
{
public:
    explicit DeckSystem(GameContext& context) : m_context(&context) { m_context->logger->info("DeckSystem 初始化"); }

    // Disable copy and move operations due to reference member
    DeckSystem(const DeckSystem&) = default;
    DeckSystem& operator=(const DeckSystem&) = delete;
    DeckSystem(DeckSystem&&) = default;
    DeckSystem& operator=(DeckSystem&& other) = delete;
    ~DeckSystem() = default;

private:
    void registerEventsImpl() { initDeck(); };
    void unregisterEventsImpl() {

    };

    /**
     * @brief 初始化牌堆
     */
    void onInitDeck(events::ShuffleDeck event)
    {
        initDeck();
        onShuffleDeck({});
    }
    /**
     * @brief 处理卡牌弃置事件
     */
    void onCardDiscarded(events::CardDiscarded event)
    {
        auto& [player, cards, count] = event;

        auto& handCards = m_context->registry.try_get<HandCards>(player)->handCards;
        auto* equipments = m_context->registry.try_get<Equipments>(player);
        auto& [weapon, armor, attackhorse, defensehorse] = *equipments;
        // 用 unordered_set 提升查找效率
        absl::flat_hash_set<entt::entity> cardSet(cards.begin(), cards.end());

        // 移除手牌
        auto newEnd = std::ranges::remove_if(handCards, [&](entt::entity card) { return cardSet.contains(card); });
        handCards.erase(newEnd.begin(), newEnd.end());

        auto checkDiscarded = [](entt::entity& equipments, absl::flat_hash_set<entt::entity>& cardSet)
        {
            if (equipments != entt::null && cardSet.contains(equipments))
            {
                equipments = entt::null;
            }
        };
        checkDiscarded(weapon, cardSet);
        checkDiscarded(armor, cardSet);
        checkDiscarded(attackhorse, cardSet);
        checkDiscarded(defensehorse, cardSet);

        m_deck.processingArea.insert(m_deck.processingArea.end(), cards.begin(), cards.end());
    }

    void onProcessFinished()
    {
        m_deck.discardPile.insert(m_deck.discardPile.end(), m_deck.processingArea.begin(), m_deck.processingArea.end());
        m_deck.processingArea.clear();
    }

    void onShuffleDeck(events::ShuffleDeck event)
    {
        absl::BitGen gen; // 自动使用系统随机源初始化

        // 打乱弃牌堆
        std::shuffle(m_deck.discardPile.begin(), m_deck.discardPile.end(), gen);

        // 将弃牌堆放入抽牌堆
        m_deck.drawPile.insert(m_deck.drawPile.end(), m_deck.discardPile.begin(), m_deck.discardPile.end());

        // 清空弃牌堆
        m_deck.discardPile.clear();
    }

    /**
     * @brief 发牌
     * @param event 发牌事件，包含玩家实体和发牌数量
     */
    void onDealCards(events::DealCards event)
    {
        auto& [player, count] = event;
        auto& handCards = m_context->registry.get<HandCards>(player).handCards;

        if (m_deck.drawPile.empty() or m_deck.drawPile.size() < count)
        {
            // 如果摸牌堆为空，触发洗牌事件
            m_context->dispatcher.trigger<events::ShuffleDeck>();
        }
        if (!m_deck.drawPile.empty())
        {
            handCards.insert(handCards.end(), m_deck.drawPile.begin(), m_deck.drawPile.begin() + count);
            m_deck.drawPile.erase(m_deck.drawPile.begin(), m_deck.drawPile.begin() + count);
            m_context->dispatcher.trigger<events::CardDrawn>(
                {.player = player,
                 .cards = std::span<entt::entity>(handCards.end() - count, handCards.end()),
                 .count = count});
        }
        else
        {
            m_context->logger->warn("无法发牌，摸牌堆和弃牌堆均为空");
            m_context->dispatcher.trigger<events::GameEnd>({.reason = "无法发牌，游戏结束"});
        }
    }
    /**
     * @brief 在摸牌堆中查找指定卡牌
     * @param event 查找卡牌事件，包含卡牌名称、花色和点数
     */
    void onFindCardInDrawPile(events::FindCardInDrawPile event)
    {
        m_findCard = entt::null;
        auto& [cardName, suitType, rank] = event;
        {
            auto it1 = std::ranges::find_if(m_deck.drawPile.begin(),
                                            m_deck.drawPile.end(),
                                            [&](entt::entity card)
                                            {
                                                auto& meta = m_context->registry.get<MetaCardInfo>(card);
                                                return meta.name == cardName;
                                            });
            if (it1 != m_deck.drawPile.end())
            {
                m_findCard = *it1;
                return;
            }
        }
    }

    void onFindCardInHandCards(events::FindCardInHandCardsArea event)
    {
        m_findCard = entt::null;
        auto& [cardName, suitType, rank] = event;
        // 遍历所有玩家手牌区域
        for (auto player : m_context->registry.view<HandCards>())
        {
            auto& handCards = m_context->registry.get<HandCards>(player).handCards;
            {
                auto it1 = std::ranges::find_if(handCards.begin(),
                                                handCards.end(),
                                                [&](entt::entity card)
                                                {
                                                    auto& meta = m_context->registry.get<MetaCardInfo>(card);
                                                    return meta.name == cardName;
                                                });
                if (it1 != handCards.end())
                {
                    m_findCard = *it1;
                    return;
                }
            }
        }
    }

    void initDeck()
    {
        // 初始化牌堆，创建所有卡牌实体并加入摸牌堆
        // 这里简化为只创建少量示例卡牌
        auto& registry = m_context->registry;
        m_deck.drawPile.clear();
        m_deck.discardPile.clear();
        m_deck.processingArea.clear();
        // NOLINTNEXTLINE
        for (int i = 0; i < 52; ++i)
        {
            entt::entity card = registry.create();
            registry.emplace<MetaCardInfo>(card, MetaCardInfo{.name = "Card" + std::to_string(i + 1)});
            m_deck.drawPile.push_back(card);
        }
        m_context->logger->info("牌堆初始化完成，包含 {} 张卡牌", m_deck.drawPile.size());
    }
    GameContext* m_context;
    Deck m_deck;
    entt::entity m_findCard{entt::null};
};