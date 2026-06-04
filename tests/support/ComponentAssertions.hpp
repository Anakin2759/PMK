#pragma once

#include <gtest/gtest.h>

#include "common/Types.hpp"
#include "src/singleton/Registry.hpp"

namespace ui::tests
{

template <typename Component>
Component& RequireComponent(entt::entity entity)
{
    auto* component = Registry::TryGet<Component>(entity);
    EXPECT_NE(component, nullptr);
    return *component;
}

template <typename Tag>
void ExpectHasTag(entt::entity entity)
{
    EXPECT_TRUE(Registry::AllOf<Tag>(entity));
}

template <typename Tag>
void ExpectNotHasTag(entt::entity entity)
{
    EXPECT_FALSE(Registry::AllOf<Tag>(entity));
}

inline void ExpectColorRgbEq(const Color& actual, const Color& expected)
{
    EXPECT_FLOAT_EQ(actual.red, expected.red);
    EXPECT_FLOAT_EQ(actual.green, expected.green);
    EXPECT_FLOAT_EQ(actual.blue, expected.blue);
}

} // namespace ui::tests
