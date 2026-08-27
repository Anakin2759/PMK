/**
 * @file UiRuntimeScope.hpp
 * @brief 用来在栈上管理 UiRuntime 生命周期的作用域对象，确保在作用域内可以访问当前运行时。
 */
#pragma once

#include "UiRuntime.hpp"

namespace ui
{

class UiRuntimeScope
{
   public:
    explicit UiRuntimeScope(UiRuntime& runtime) noexcept
        : m_prev(UiRuntime::s_current == nullptr ? nullptr : UiRuntime::s_current->m_lifetime)
    {
        runtime.m_lifetime->runtime = &runtime;
        UiRuntime::s_current = &runtime;
    }

    ~UiRuntimeScope()
    {
        UiRuntime::s_current = m_prev == nullptr ? nullptr : m_prev->runtime;
    }

    UiRuntimeScope(const UiRuntimeScope&) = delete;
    UiRuntimeScope& operator=(const UiRuntimeScope&) = delete;
    UiRuntimeScope(UiRuntimeScope&&) = delete;
    UiRuntimeScope& operator=(UiRuntimeScope&&) = delete;

   private:
    std::shared_ptr<UiRuntime::Lifetime> m_prev;
};

}  // namespace ui
