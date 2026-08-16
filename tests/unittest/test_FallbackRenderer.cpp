#include "src/renderers/FallbackBackendRenderer.hpp"

#include <array>

#include <gtest/gtest.h>

namespace ui::tests
{
namespace
{

TEST(FallbackRendererGeometryTest, DetectsAxisAlignedQuad)
{
    const std::array<SDL_FPoint, 4> points = {
        SDL_FPoint{10.0F, 20.0F}, SDL_FPoint{30.0F, 20.0F}, SDL_FPoint{30.0F, 40.0F},
        SDL_FPoint{10.0F, 40.0F}};

    EXPECT_TRUE(renderers::detail::isAxisAlignedQuad(points));
}

TEST(FallbackRendererGeometryTest, RejectsRotatedQuad)
{
    const std::array<SDL_FPoint, 4> points = {
        SDL_FPoint{20.0F, 10.0F}, SDL_FPoint{40.0F, 20.0F}, SDL_FPoint{30.0F, 40.0F},
        SDL_FPoint{10.0F, 30.0F}};

    EXPECT_FALSE(renderers::detail::isAxisAlignedQuad(points));
}

}  // namespace
}  // namespace ui::tests
