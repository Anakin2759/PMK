#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntimeScope.hpp"

namespace ui::tests
{

class UiRuntimeTest : public ::testing::Test
{
protected:
    void SetUp() override { m_scope = std::make_unique<UiRuntimeScope>(m_runtime); }

    void TearDown() override
    {
        UiRuntime::current().dispatcher().update();
        m_scope.reset();
    }

    UiRuntime& runtime() { return m_runtime; }
    const UiRuntime& runtime() const { return m_runtime; }

private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
};

} // namespace ui::tests
