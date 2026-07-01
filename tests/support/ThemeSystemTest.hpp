#pragma once

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/core/UiRuntime.hpp"
#include "src/core/UiRuntime.hpp"

#include <ui.hpp>

#include "common/Tags.hpp"
#include "common/components/Data.hpp"
#include "common/components/Layout.hpp"
#include "common/components/Visual.hpp"
#include "src/systems/ThemeSystem.hpp"

namespace ui::tests
{

class ThemeSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_scope = std::make_unique<UiRuntimeScope>(m_runtime);
        m_themeSystem = std::make_unique<systems::ThemeSystem>(UiRuntime::current().registry(),
                                                               UiRuntime::current().dispatcher());
        m_themeSystem->registerHandlers();
    }

    void TearDown() override
    {
        m_themeSystem->unregisterHandlers();
        m_themeSystem.reset();
        m_scope.reset();
    }

    static void triggerThemeUpdate() { UiRuntime::current().trigger(events::UpdateEvent{}); }

    static Registry& registry() { return UiRuntime::current().registry(); }

    static std::vector<entt::entity> popupChildren(entt::entity popupEntity)
    {
        const auto* hierarchy = registry().try_get<components::Hierarchy>(popupEntity);
        if (hierarchy == nullptr)
        {
            return {};
        }

        return hierarchy->children;
    }

private:
    UiRuntime m_runtime;
    std::unique_ptr<UiRuntimeScope> m_scope;
    std::unique_ptr<systems::ThemeSystem> m_themeSystem;
};

} // namespace ui::tests
