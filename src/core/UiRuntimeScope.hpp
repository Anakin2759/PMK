/**
 * @file UiRuntimeScope.hpp
 * @brief RAII scope guard that pushes/pops the active UiRuntime on a thread.
 */
#pragma once

#include "UiRuntime.hpp"

namespace ui
{

class UiRuntimeScope
{
   public:
    explicit UiRuntimeScope(UiRuntime& runtime) noexcept : m_prev(UiRuntime::s_current)
    {
        UiRuntime::s_current = &runtime;
    }

    ~UiRuntimeScope()
    {
        UiRuntime::s_current = m_prev;
    }

    UiRuntimeScope(const UiRuntimeScope&) = delete;
    UiRuntimeScope& operator=(const UiRuntimeScope&) = delete;
    UiRuntimeScope(UiRuntimeScope&&) = delete;
    UiRuntimeScope& operator=(UiRuntimeScope&&) = delete;

   private:
    UiRuntime* m_prev;
};

}  // namespace ui
