/**
 * ************************************************************************
 *
 * @file Registry.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-26
 * @version 0.1
 * @brief UI模块的全局实体注册表单例封装
  - 基于 EnTT 实现的全局实体注册表单例
  - 提供统一的实体和组件管理接口
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <entt/entt.hpp>
#include "SingletonBase.hpp"
#include "../traits/ComponentsTraits.hpp"
#include <memory>
namespace ui
{
using traits::ComponentOrUiTag;

class Registry : public SingletonBase<Registry>
{
    friend class SingletonBase<Registry>;

public:
    static std::shared_ptr<Registry> GetRegistryPtr()
    {
        return std::shared_ptr<Registry>(&getInstance(),
                                         [](Registry*)
                                         {
                                             // Do nothing, prevent deletion
                                         });
    }
    static entt::entity Create() { return getInstance().m_registry.create(); }

    template <ComponentOrUiTag... Type>
    static auto View()
    {
        return getInstance().m_registry.view<Type...>();
    }

    template <typename... Owned, typename... Get, typename... Exclude>
    static auto Group(entt::get_t<Get...> get = {}, entt::exclude_t<Exclude...> exclude = {})
    {
        return getInstance().m_registry.group<Owned...>(get, exclude);
    }

    template <ComponentOrUiTag Type>
    static auto Get(::entt::entity entity) -> Type&
    {
        return getInstance().m_registry.get<Type>(entity);
    }

    template <ComponentOrUiTag Type>
    static auto TryGet(::entt::entity entity) -> Type*
    {
        return getInstance().m_registry.try_get<Type>(entity);
    }

    template <ComponentOrUiTag Type, typename... Args>
    static decltype(auto) Emplace(::entt::entity entity, Args&&... args)
    {
        return getInstance().m_registry.emplace<Type>(entity, std::forward<Args>(args)...);
    }

    template <ComponentOrUiTag Type, typename... Args>
    static auto Replace(::entt::entity entity, Args&&... args) -> Type&
    {
        return getInstance().m_registry.replace<Type>(entity, std::forward<Args>(args)...);
    }

    template <ComponentOrUiTag Type, typename... Args>
    static void EmplaceOrReplace(::entt::entity entity, Args&&... args)
    {
        getInstance().m_registry.emplace_or_replace<Type>(entity, std::forward<Args>(args)...);
    }
    template <ComponentOrUiTag Type, typename... Args>
    static auto GetOrEmplace(::entt::entity entity, Args&&... args) -> Type&
    {
        return getInstance().m_registry.get_or_emplace<Type>(entity, std::forward<Args>(args)...);
    }

    template <ComponentOrUiTag Type>
    static void Remove(::entt::entity entity)
    {
        getInstance().m_registry.remove<Type>(entity);
    }

    template <ComponentOrUiTag... Type>
    static auto AnyOf(::entt::entity entity) -> bool
    {
        return getInstance().m_registry.any_of<Type...>(entity);
    }

    template <ComponentOrUiTag... Type>
    static auto AllOf(::entt::entity entity) -> bool
    {
        return getInstance().m_registry.all_of<Type...>(entity);
    }

    static bool Valid(::entt::entity entity) { return getInstance().m_registry.valid(entity); }

    static void Destroy(::entt::entity entity) { getInstance().m_registry.destroy(entity); }



    template <ComponentOrUiTag... Type>
    static auto Clear()
    {
        return getInstance().m_registry.clear<Type...>();
    }

    static void Clear() { return getInstance().m_registry.clear(); }

    template <ComponentOrUiTag Type>
    static auto OnUpdate()
    {
        return getInstance().m_registry.on_update<Type>();
    }
    template <ComponentOrUiTag Type>
    static auto OnDestroy()
    {
        return getInstance().m_registry.on_destroy<Type>();
    }
    /**
     * @brief 组件构造事件回调连接器
     * @return entt::sink<ComponentConstructedSignature>
     */
    template <ComponentOrUiTag Type>
    static auto OnConstruct()
    {
        return getInstance().m_registry.on_construct<Type>();
    }

    /**
     * @brief 获取全局上下文存储
     */
    static auto& ctx() { return getInstance().m_registry.ctx(); }

private:
    Registry() = default;

    entt::registry m_registry;
};
} // namespace ui