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

#include <atomic>
#include <cstdio>
#include <exception>

#include <entt/entt.hpp>

#include "common/EntityTypes.hpp"
#include "traits/ComponentsTraits.hpp"


namespace ui
{
using traits::ComponentOrUiTag;

class UiRuntime;
class UiRuntimeScope;
class WorkerMailbox;

class Registry
{
    friend class UiRuntime;
    friend class UiRuntimeScope;
    friend class RuntimeFacade;
    friend class WorkerMailbox;

public:
    
    Registry() = default;
    // -------------------------------------------------------------------------
    // Instance methods — dependency-injected systems use these via Registry&
    // -------------------------------------------------------------------------

    [[nodiscard]] entt::entity create() { return m_registry.create(); }

    [[nodiscard]] bool valid(entt::entity entity) const noexcept { return m_registry.valid(entity); }

    void destroy(entt::entity entity) { m_registry.destroy(entity); }

    template <ComponentOrUiTag... Type>
    [[nodiscard]] auto view()
    {
        return m_registry.view<Type...>();
    }

    template <ComponentOrUiTag... Type, typename... Exclude>
    [[nodiscard]] auto view(entt::exclude_t<Exclude...> excl)
    {
        return m_registry.view<Type...>(excl);
    }

    template <ComponentOrUiTag... Type>
    [[nodiscard]] auto view() const
    {
        return m_registry.view<Type...>();
    }

    template <ComponentOrUiTag... Type, typename... Exclude>
    [[nodiscard]] auto view(entt::exclude_t<Exclude...> excl) const
    {
        return m_registry.view<Type...>(excl);
    }

    template <typename... Owned, typename... Get, typename... Exclude>
    [[nodiscard]] auto group(entt::get_t<Get...> get = {}, entt::exclude_t<Exclude...> exclude = {})
    {
        return m_registry.group<Owned...>(get, exclude);
    }

    template <ComponentOrUiTag Type>
    [[nodiscard]] Type& get(entt::entity entity)
    {
        return m_registry.get<Type>(entity);
    }



    template <ComponentOrUiTag Type>
    [[nodiscard]] const Type& get(entt::entity entity) const
    {
        return m_registry.get<Type>(entity);
    }


    template <ComponentOrUiTag Type>
    [[nodiscard]] Type* try_get(entt::entity entity) noexcept
    {
        return m_registry.try_get<Type>(entity);
    }



    template <ComponentOrUiTag Type>
    [[nodiscard]] const Type* try_get(entt::entity entity) const noexcept
    {
        return m_registry.try_get<Type>(entity);
    }



    template <ComponentOrUiTag Type, typename... Args>
    decltype(auto) emplace(entt::entity entity, Args&&... args)
    {
        return m_registry.emplace<Type>(entity, std::forward<Args>(args)...);
    }



    template <ComponentOrUiTag Type, typename... Args>
    Type& replace(entt::entity entity, Args&&... args)
    {
        return m_registry.replace<Type>(entity, std::forward<Args>(args)...);
    }



    template <ComponentOrUiTag Type, typename... Args>
    decltype(auto) emplace_or_replace(entt::entity entity, Args&&... args)
    {
        return m_registry.emplace_or_replace<Type>(entity, std::forward<Args>(args)...);
    }



    template <ComponentOrUiTag Type, typename... Args>
    Type& get_or_emplace(entt::entity entity, Args&&... args)
    {
        return m_registry.get_or_emplace<Type>(entity, std::forward<Args>(args)...);
    }

    /// @brief 订阅实体销毁信号（用于系统自动清理关联资源，如 Yoga 节点）
    /// @note 替代直接调用 raw().on_destroy<T>()，避免暴露内部 entt::registry
    template <ComponentOrUiTag Type>
    [[nodiscard]] auto onDestroy()
    {
        return m_registry.on_destroy<Type>();
    }

    template <ComponentOrUiTag Type>
    void remove(entt::entity entity)
    {
        m_registry.remove<Type>(entity);
    }


    template <ComponentOrUiTag... Type>
    [[nodiscard]] bool any_of(entt::entity entity) const
    {
        return m_registry.any_of<Type...>(entity);
    }


    template <ComponentOrUiTag... Type>
    [[nodiscard]] bool all_of(entt::entity entity) const
    {
        return m_registry.all_of<Type...>(entity);
    }


    template <ComponentOrUiTag... Type>
    void clear()
    {
        m_registry.clear<Type...>();
    }

    void clear() { m_registry.clear(); }

    template <ComponentOrUiTag Type>
    [[nodiscard]] auto on_construct()
    {
        return m_registry.on_construct<Type>();
    }

    template <ComponentOrUiTag Type>
    [[nodiscard]] auto on_destroy()
    {
        return m_registry.on_destroy<Type>();
    }

    template <ComponentOrUiTag Type>
    [[nodiscard]] auto on_update()
    {
        return m_registry.on_update<Type>();
    }

    template <ComponentOrUiTag Context>
    [[nodiscard]] auto findInCtx()const
     { return m_registry.ctx().find<Context>(); }


     template <ComponentOrUiTag Context>
    [[nodiscard]] auto getInCtx()const
     { return m_registry.ctx().get<Context>(); }

      template <ComponentOrUiTag Context>
    [[nodiscard]] auto emplaceInCtx()const
     { return m_registry.ctx().emplace<Context>(); }



    
private:

    entt::registry m_registry;
};
} // namespace ui