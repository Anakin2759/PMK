#include "ui/api/Factory.hpp"
#include "ui/api/Utils.hpp"
#include "src/common/Events.hpp"
#include "src/renderers/FallbackBackendRenderer.hpp"
#include "src/systems/render/WindowRenderState.hpp"

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

TEST(WindowRenderStateTest, TextScaleKeyIsStableAcrossWindowSwitches)
{
    EXPECT_EQ(systems::render_detail::MakeTextScaleKey(1.0F), 200);
    EXPECT_EQ(systems::render_detail::MakeTextScaleKey(2.0F), 200);
    EXPECT_EQ(systems::render_detail::MakeTextScaleKey(3.0F), 300);

    const int firstA = systems::render_detail::MakeTextScaleKey(1.0F);
    const int windowB = systems::render_detail::MakeTextScaleKey(3.0F);
    const int secondA = systems::render_detail::MakeTextScaleKey(1.0F);
    EXPECT_EQ(firstA, secondA);
    EXPECT_NE(firstA, windowB);
}

[[nodiscard]] std::unique_ptr<Application> CreateFallbackApplication()
{
    std::array<char, sizeof("ui_lifecycle")> programName{};
    std::array<char, sizeof("--backend=cpu")> backendOption{};
    std::ranges::copy(std::string_view{"ui_lifecycle"}, programName.begin());
    std::ranges::copy(std::string_view{"--backend=cpu"}, backendOption.begin());
    std::array<char*, 2> arguments{programName.data(), backendOption.data()};

    auto applicationResult = factory::CreateApplication(std::span<char*>(arguments));
    EXPECT_TRUE(applicationResult.has_value());
    return applicationResult.has_value() ? std::move(applicationResult.value()) : nullptr;
}

[[nodiscard]] render::Vertex MakeVertex(float x, float y, const std::array<float, 4>& color, float radius = 0.0F)
{
    render::Vertex vertex{};
    vertex.position[0] = x;
    vertex.position[1] = y;
    std::ranges::copy(color, vertex.color);
    std::ranges::fill(vertex.radius, radius);
    return vertex;
}

TEST(FallbackRendererPixelTest, DrawBatchRendersSolidAndRoundedRectangles)
{
    ASSERT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
    auto application = CreateFallbackApplication();
    ASSERT_NE(application, nullptr);
    SDL_Window* window = SDL_CreateWindow("Fallback pixels", 64, 64, SDL_WINDOW_HIDDEN);
    ASSERT_NE(window, nullptr) << SDL_GetError();

    utils::Logger logger;
    renderers::FallbackBackendRenderer backend{logger};
    {
        ASSERT_TRUE(backend.initialize(window).has_value());
        ASSERT_TRUE(backend.beginFrame(SDL_FColor{0.0F, 0.0F, 0.0F, 1.0F}).has_value());

        render::RenderBatch solidBatch;
        constexpr std::array RED = {1.0F, 0.0F, 0.0F, 1.0F};
        solidBatch.pushConstants.opacity = 1.0F;
        solidBatch.vertices = {MakeVertex(4.0F, 4.0F, RED), MakeVertex(28.0F, 4.0F, RED),
                               MakeVertex(28.0F, 28.0F, RED), MakeVertex(4.0F, 28.0F, RED)};
        ASSERT_TRUE(backend.drawBatch(solidBatch, nullptr).has_value());

        render::RenderBatch roundedBatch;
        constexpr std::array GREEN = {0.0F, 1.0F, 0.0F, 1.0F};
        roundedBatch.pushConstants.opacity = 1.0F;
        roundedBatch.vertices = {MakeVertex(36.0F, 4.0F, GREEN, 8.0F), MakeVertex(60.0F, 4.0F, GREEN, 8.0F),
                                 MakeVertex(60.0F, 28.0F, GREEN, 8.0F),
                                 MakeVertex(36.0F, 28.0F, GREEN, 8.0F)};
        ASSERT_TRUE(backend.drawBatch(roundedBatch, nullptr).has_value());

        SDL_Surface* frame = SDL_RenderReadPixels(SDL_GetRenderer(window), nullptr);
        ASSERT_NE(frame, nullptr) << SDL_GetError();
        Uint8 red = 0;
        Uint8 green = 0;
        Uint8 blue = 0;
        Uint8 alpha = 0;
        ASSERT_TRUE(SDL_ReadSurfacePixel(frame, 12, 12, &red, &green, &blue, &alpha));
        EXPECT_GT(red, 240);
        EXPECT_LT(green, 15);
        ASSERT_TRUE(SDL_ReadSurfacePixel(frame, 48, 12, &red, &green, &blue, &alpha));
        EXPECT_LT(red, 15);
        EXPECT_GT(green, 240);
        ASSERT_TRUE(SDL_ReadSurfacePixel(frame, 36, 4, &red, &green, &blue, &alpha));
        EXPECT_LT(green, 15) << "rounded corner must preserve the clear color";
        SDL_DestroySurface(frame);

        constexpr std::array<std::uint8_t, 16> BITMAP_PIXELS = {
            255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255};
        constexpr SDL_FRect BITMAP_DESTINATION = {4.0F, 36.0F, 24.0F, 16.0F};
        constexpr SDL_Rect BITMAP_SCISSOR = {12, 36, 16, 16};
        constexpr std::uint8_t HALF_ALPHA = 128;
        ASSERT_TRUE(backend.drawCachedBitmap("pixel-test", BITMAP_PIXELS, 2, 2, BITMAP_DESTINATION, BITMAP_SCISSOR,
                                             HALF_ALPHA)
                        .has_value());

        frame = SDL_RenderReadPixels(SDL_GetRenderer(window), nullptr);
        ASSERT_NE(frame, nullptr) << SDL_GetError();
        ASSERT_TRUE(SDL_ReadSurfacePixel(frame, 8, 44, &red, &green, &blue, &alpha));
        EXPECT_LT(red, 15) << "pixels outside the bitmap scissor must preserve the clear color";
        ASSERT_TRUE(SDL_ReadSurfacePixel(frame, 16, 44, &red, &green, &blue, &alpha));
        EXPECT_NEAR(red, HALF_ALPHA, 4) << "bitmap alpha modulation must blend over the clear color";
        EXPECT_LT(green, 15);
        EXPECT_LT(blue, 15);
        SDL_DestroySurface(frame);
        backend.cleanup();
    }
    SDL_DestroyWindow(window);
    application.reset();
    EXPECT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
}

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

        utils::CloseWindow(application->runtime(), handle.raw);
        application->runtime().dispatcher().update<events::CloseWindow>();

        EXPECT_FALSE(application->runtime().registry().valid(handle.raw)) << "iteration=" << iteration;
        EXPECT_EQ(SDL_GetWindowFromID(handle.windowId), nullptr)
            << "iteration=" << iteration << ", windowId=" << handle.windowId << ", SDL=" << SDL_GetError();
    }

    application.reset();
    EXPECT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
}

TEST(FallbackWindowLifecycleTest, TwoWindowsKeepIndependentRenderersAcrossAlternatingPresents)
{
    ASSERT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
    auto application = CreateFallbackApplication();
    ASSERT_NE(application, nullptr);
    ASSERT_STREQ(SDL_GetCurrentVideoDriver(), "offscreen");

    auto firstResult = factory::CreateWindow(application->runtime(), "Fallback A", "fallback_multi_a");
    auto secondResult = factory::CreateWindow(application->runtime(), "Fallback B", "fallback_multi_b");
    ASSERT_TRUE(firstResult.has_value()) << firstResult.error().ToString();
    ASSERT_TRUE(secondResult.has_value()) << secondResult.error().ToString();
    const WindowHandle first = firstResult.value();
    const WindowHandle second = secondResult.value();

    SDL_Window* firstWindow = SDL_GetWindowFromID(first.windowId);
    SDL_Window* secondWindow = SDL_GetWindowFromID(second.windowId);
    ASSERT_NE(firstWindow, nullptr);
    ASSERT_NE(secondWindow, nullptr);
    SDL_Renderer* firstRenderer = SDL_GetRenderer(firstWindow);
    SDL_Renderer* secondRenderer = SDL_GetRenderer(secondWindow);
    ASSERT_NE(firstRenderer, nullptr);
    ASSERT_NE(secondRenderer, nullptr);
    EXPECT_NE(firstRenderer, secondRenderer);
    ASSERT_TRUE(SDL_RenderPresent(firstRenderer));
    ASSERT_TRUE(SDL_RenderPresent(secondRenderer));
    ASSERT_TRUE(SDL_RenderPresent(firstRenderer));

    utils::CloseWindow(application->runtime(), second.raw);
    application->runtime().dispatcher().update<events::CloseWindow>();
    EXPECT_EQ(SDL_GetWindowFromID(second.windowId), nullptr);
    firstWindow = SDL_GetWindowFromID(first.windowId);
    ASSERT_NE(firstWindow, nullptr);
    EXPECT_EQ(SDL_GetRenderer(firstWindow), firstRenderer);
    EXPECT_TRUE(SDL_RenderPresent(firstRenderer));

    utils::CloseWindow(application->runtime(), first.raw);
    application->runtime().dispatcher().update<events::CloseWindow>();
    application.reset();
    EXPECT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
}

}  // namespace
}  // namespace ui::tests
