/**
 * ************************************************************************
 *
 * @file DamageSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-25
 * @version 0.1
 * @brief 处理伤害相关的系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include "src/server/context/GameContext.h"
#include "src/server/events/Events.h"
#include "src/server/components/Character.h"
class DamageSystem
{
public:
    explicit DamageSystem(GameContext& context) : m_context(&context) {};

    void init() { registerEvents(); };
    void destroy() { unregisterEvents(); };

private:
    void registerEvents() { m_context->dispatcher.sink<events::Damage>().connect<&DamageSystem::onDamageEvent>(this); };
    void unregisterEvents()
    {
        m_context->dispatcher.sink<events::Damage>().disconnect<&DamageSystem::onDamageEvent>(this);
    };
    void onDamageEvent(const events::Damage& damageEvent) const
    {
        auto [source, target, amount] = damageEvent;
        amount = std::max(0, amount);
        // 目标没有生命组件则忽略
        if (auto* attributes = m_context->registry.try_get<Attributes>(target))
        {
            int before = attributes->currentHealth;
            attributes->currentHealth -= amount;
            // 如果从存活变为死亡，触发濒死事件
            if (before > 0 && attributes->currentHealth <= 0)
            {
                m_context->dispatcher.trigger(events::NearDeath{
                    .killer = source, .character = target, .currentHealth = attributes->currentHealth});
            }
        }
    };

    void onDamagePreventionEvent(const events::DamagePrevention& event) const
    {
        // 处理伤害预防逻辑
        m_context->logger->info("伤害预防事件触发");
        auto target = event.target;
        int amount = event.amount;
        if (amount <= 0)
        {
            return;
        }
    };

    void onDamageRedirectionEvent(const events::DamageRedirection& event) const
    {
        // 处理伤害重定向逻辑
        m_context->logger->info("伤害重定向事件触发");
        auto newTarget = event.newTarget;
        int amount = event.amount;
        if (amount <= 0)
        {
            return;
        }
    };
    GameContext* m_context;
};