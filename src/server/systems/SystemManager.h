/**
 * ************************************************************************
 *
 * @file SystemManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 系统管理器 注册和注销所有系统的事件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include "src/server/context/GameContext.h"
#include "src/server/systems/ISystem.h"

class SystemManager
{
public:
    explicit SystemManager(GameContext& context) : m_context(&context) {}
    void addSystem(auto system)
    {
        m_context->logger->info("添加系统");
        m_systems.emplace_back(std::move(system));
        m_context->logger->info("系统添加，当前系统数量：{}", m_systems.size());
    }

    void registryAll()
    {
        for (auto& system : m_systems)
        {
            system->registerEvents();
        }
    }

    void unregisterAll()
    {
        for (auto& system : m_systems)
        {
            system->unregisterEvents();
        }
    }

private:
    std::vector<entt::poly<ISystem>> m_systems;
    GameContext* m_context;
};
