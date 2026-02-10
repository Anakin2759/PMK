/**
 * ************************************************************************
 *
 * @file ISystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 系统接口定义
    所有系统均实现该接口
    提供注册和注销事件的方法
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <entt/poly/poly.hpp>

struct ISystem : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void registerEvents() { entt::poly_call<0>(*this); }
        void unregisterEvents() { entt::poly_call<1>(*this); }
    };

    template <typename T>
    using impl = entt::value_list<&T::registerEvents, &T::unregisterEvents>;
};

template <typename Derived>
struct EnableRegister
{
    void registerEvents() { static_cast<Derived*>(this)->registerEventsImpl(); }
    void unregisterEvents() { static_cast<Derived*>(this)->unregisterEventsImpl(); }
};