/**
 * ************************************************************************
 *
 * @file WindowEntityLookup.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-03-26
 * @version 0.2
 * @brief 窗口控件查找服务实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <unordered_map>

#include <entt/entt.hpp>

#include "common/components/Window.hpp"
#include "utils/Registry.hpp"

namespace ui::window_lookup
{

struct WindowEntityLookupCache
{
    using is_component_tag = void;
    std::unordered_map<uint32_t, entt::entity> entitiesByWindowId;
};

inline WindowEntityLookupCache& Cache(Registry& registry)
{
    if (auto* cache = registry.findInCtx<WindowEntityLookupCache>())
    {
        return *cache;
    }

    return registry.getOrEmplaceInCtx<WindowEntityLookupCache>();
}

inline bool MatchesWindowId(const Registry& registry, entt::entity entity, uint32_t windowId)
{
    if (!registry.valid(entity) || !registry.all_of<components::Window>(entity))
    {
        return false;
    }

    return registry.get<components::Window>(entity).windowID == windowId;
}

inline void RememberWindowEntity(Registry& registry, entt::entity entity)
{
    if (!registry.valid(entity) || !registry.all_of<components::Window>(entity))
    {
        return;
    }

    const auto windowId = registry.get<components::Window>(entity).windowID;
    if (windowId == 0)
    {
        return;
    }

    Cache(registry).entitiesByWindowId[windowId] = entity;
}

inline void InvalidateWindowId(Registry& registry, uint32_t windowId)
{
    if (windowId == 0)
    {
        return;
    }

    if (auto* cache = registry.findInCtx<WindowEntityLookupCache>())
    {
        cache->entitiesByWindowId.erase(windowId);
    }
}

/**
 * @brief 使指定实体失效（从缓存中移除）
 *
 * @param entity 要失效的实体
 */
inline void InvalidateWindowEntity(Registry& registry, entt::entity entity)
{
    if (!registry.valid(entity) || !registry.all_of<components::Window>(entity))
    {
        return;
    }

    InvalidateWindowId(registry, registry.get<components::Window>(entity).windowID);
}

inline entt::entity FindWindowEntityById(Registry& registry, uint32_t windowId)
{
    if (windowId == 0)
    {
        return entt::null;
    }

    auto& cache = Cache(registry);
    if (const auto cacheEntry = cache.entitiesByWindowId.find(windowId); cacheEntry != cache.entitiesByWindowId.end())
    {
        if (MatchesWindowId(registry, cacheEntry->second, windowId))
        {
            return cacheEntry->second;
        }

        cache.entitiesByWindowId.erase(cacheEntry);
    }

    auto view = registry.view<components::Window>();
    for (auto entity : view)
    {
        if (view.get<components::Window>(entity).windowID == windowId)
        {
            cache.entitiesByWindowId[windowId] = entity;
            return entity;
        }
    }

    return entt::null;
}

}  // namespace ui::window_lookup