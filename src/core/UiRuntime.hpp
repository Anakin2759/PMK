/**
 * ************************************************************************
 *
 * @file UiRuntime.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-03-26
 * @version 0.1
 * @brief UI运行时上下文 - 提供全局访问点和上下文管理功能
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once


#include "utils/ThreadPool.hpp"
#include "utils/Dispatcher.hpp"
#include "utils/Logger.hpp"
#include "utils/Registry.hpp"
#include <memory>

namespace ui
{

class UiRuntime
{
public:
    UiRuntime(): m_registry(std::make_unique<Registry>()),
                 m_dispatcher(std::make_unique<Dispatcher>()),
                 m_logger(std::make_unique<utils::Logger>())
    {
        s_current = this;
    }

    ~UiRuntime()
    {
        if (s_current == this) s_current = nullptr;
    }

    UiRuntime(const UiRuntime&) = delete;
    UiRuntime& operator=(const UiRuntime&) = delete;
    UiRuntime(UiRuntime&&) = delete;
    UiRuntime& operator=(UiRuntime&&) = delete;

    /// 获取当前线程活跃的 UiRuntime 实例（由构造 / 析构设定）
    [[nodiscard]] static UiRuntime& current() { return *s_current; }

    [[nodiscard]] Registry& registry() noexcept { return *m_registry; }

    [[nodiscard]] const Registry& registry() const noexcept { return *m_registry; }

    [[nodiscard]] Dispatcher& dispatcher() noexcept { return *m_dispatcher; }

    [[nodiscard]] const Dispatcher& dispatcher() const noexcept { return *m_dispatcher; }

    [[nodiscard]] utils::Logger& logger() noexcept { return *m_logger; }

    [[nodiscard]] const utils::Logger& logger() const noexcept { return *m_logger; }

    [[nodiscard]] std::uintptr_t token() const noexcept { return reinterpret_cast<std::uintptr_t>(this); }

    /// 获取或创建类型化 context（委托 Registry::getOrEmplaceInCtx）
    template <typename T>
    [[nodiscard]] T& ensureContext()
    {
        return m_registry->getOrEmplaceInCtx<T>();
    }

    /// 尝试获取类型化 context，不存在则返回 nullptr
    template <typename T>
    [[nodiscard]] T* tryContext()
    {
        return m_registry->findInCtx<T>();
    }

    template <typename T>
    [[nodiscard]] const T* tryContext() const
    {
        return m_registry->findInCtx<T>();
    }


    inline static thread_local UiRuntime* s_current = nullptr;

private:
    friend class UiRuntimeScope;

    std::unique_ptr<utils::ThreadPool> m_threadPool;
    std::unique_ptr<Registry> m_registry;
    std::unique_ptr<Dispatcher> m_dispatcher;
    std::unique_ptr<utils::Logger> m_logger;
};


} // namespace ui
