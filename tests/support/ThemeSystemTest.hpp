#pragma once

#include <gtest/gtest.h>

#include <memory>
#include <vector>

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
        m_themeSystem = std::make_unique<systems::ThemeSystem>(m_runtime);
        m_themeSystem->registerHandlers();
    }

    void TearDown() override
    {
        m_themeSystem->unregisterHandlers();
        m_themeSystem.reset();
    }

    UiRuntime& runtime()
    {
        return m_runtime;
    }

    void triggerThemeUpdate()
    {
        m_runtime.dispatcher().trigger(events::UpdateEvent{});
    }

    Registry& registry()
    {
        return m_runtime.registry();
    }

    const theme::ThemePalette& currentTheme()
    {
        return theme::CurrentTheme(m_runtime);
    }

    void setTheme(const theme::ThemePalette& palette)
    {
        theme::SetTheme(m_runtime, palette);
    }

    std::vector<entt::entity> popupChildren(ui::entity popupEntity)
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
    std::unique_ptr<systems::ThemeSystem> m_themeSystem;
};

}  // namespace ui::tests
