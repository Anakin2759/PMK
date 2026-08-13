#include "ui/api/Factory.hpp"
#include "ui/api/Utils.hpp"
#include "src/common/Events.hpp"
#include "src/core/UiRuntimeScope.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

namespace ui::tests
{
namespace
{

TEST(FallbackWindowLifecycleTest, CreatesAndClosesOneHundredOffscreenSoftwareWindows)
{
    ASSERT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);

    std::array<char, sizeof("ui_lifecycle")> programName{};
    std::array<char, sizeof("--backend=cpu")> backendOption{};
    std::ranges::copy(std::string_view{"ui_lifecycle"}, programName.begin());
    std::ranges::copy(std::string_view{"--backend=cpu"}, backendOption.begin());
    std::array<char*, 2> arguments{programName.data(), backendOption.data()};

    auto applicationResult = factory::CreateApplication(std::span<char*>(arguments));
    ASSERT_TRUE(applicationResult.has_value()) << applicationResult.error().ToString();
    auto application = std::move(applicationResult.value());

    ASSERT_STREQ(SDL_GetCurrentVideoDriver(), "offscreen");

    constexpr std::size_t ITERATION_COUNT = 100;
    for (std::size_t iteration = 0; iteration < ITERATION_COUNT; ++iteration)
    {
        const std::string alias = "fallback_lifecycle_" + std::to_string(iteration);
        auto windowResult = factory::CreateWindow(application->runtime(), "Fallback lifecycle", alias);
        ASSERT_TRUE(windowResult.has_value())
            << "iteration=" << iteration << ", error=" << windowResult.error().ToString();

        const WindowHandle handle = windowResult.value();
        ASSERT_NE(handle.raw, null_entity) << "iteration=" << iteration;
        ASSERT_NE(handle.windowId, 0U) << "iteration=" << iteration;
        ASSERT_TRUE(application->runtime().registry().valid(handle.raw)) << "iteration=" << iteration;

        SDL_Window* window = SDL_GetWindowFromID(handle.windowId);
        ASSERT_NE(window, nullptr) << "iteration=" << iteration << ", SDL=" << SDL_GetError();
        SDL_Renderer* renderer = SDL_GetRenderer(window);
        ASSERT_NE(renderer, nullptr) << "iteration=" << iteration << ", SDL=" << SDL_GetError();
        ASSERT_STREQ(SDL_GetRendererName(renderer), "software") << "iteration=" << iteration;

        {
            UiRuntimeScope scope(application->runtime());
            utils::CloseWindow(handle.raw);
            application->runtime().dispatcher().update<events::CloseWindow>();
        }

        EXPECT_FALSE(application->runtime().registry().valid(handle.raw)) << "iteration=" << iteration;
        EXPECT_EQ(SDL_GetWindowFromID(handle.windowId), nullptr)
            << "iteration=" << iteration << ", windowId=" << handle.windowId << ", SDL=" << SDL_GetError();
    }

    application.reset();
    EXPECT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
}

}  // namespace
}  // namespace ui::tests
