#pragma once

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <ui.hpp>

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
        m_themeSystem = std::make_unique<systems::ThemeSystem>(RuntimeFacade::current().registry(),
                                                               RuntimeFacade::current().dispatcher());
        m_themeSystem->registerHandlers();
    }

    void TearDown() override
    {
        m_themeSystem->unregisterHandlers();
        m_themeSystem.reset();
        m_scope.reset();
    }

    static void triggerThemeUpdate() { RuntimeFacade::current().trigger(events::UpdateEvent{}); }

    static std::vector<entt::entity> popupChildren(entt::entity popupEntity)
    {
        const auto* hierarchy = Registry::TryGet<components::Hierarchy>(popupEntity);
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
