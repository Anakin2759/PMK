#pragma once

#include <gtest/gtest.h>

#include "common/Types.hpp"
#include "src/core/RuntimeFacade.hpp"

namespace ui::tests
{

template <typename Component>
Component& RequireComponent(entt::entity entity)
{
    auto* component = RuntimeFacade::current().registry().try_get<Component>(entity);
    EXPECT_NE(component, nullptr);
    return *component;
}

template <typename Tag>
void ExpectHasTag(entt::entity entity)
{
    EXPECT_TRUE(RuntimeFacade::current().registry().all_of<Tag>(entity));
}

template <typename Tag>
void ExpectNotHasTag(entt::entity entity)
{
    EXPECT_FALSE(RuntimeFacade::current().registry().all_of<Tag>(entity));
}

inline void ExpectColorRgbEq(const Color& actual, const Color& expected)
{
    EXPECT_FLOAT_EQ(actual.red, expected.red);
    EXPECT_FLOAT_EQ(actual.green, expected.green);
    EXPECT_FLOAT_EQ(actual.blue, expected.blue);
}

} // namespace ui::tests
