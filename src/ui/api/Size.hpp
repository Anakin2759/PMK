#pragma once

#include <entt/entt.hpp>
#include "Utils.hpp"
#include "../common/Policies.hpp"
#include "Chains.hpp" // Changed: Include Chains.hpp for DSL

namespace ui::size
{
void SetFixedSize(::entt::entity entity, float width, float height);
void SetSizePolicy(::entt::entity entity, policies::Size policy);
void SetSize(::entt::entity entity, float width, float height);
void SetPosition(::entt::entity entity, float positionX, float positionY);

} // namespace ui::size

namespace ui::chains
{
inline auto FixedSize(float w, float h)
{
    return Call<ui::size::SetFixedSize>(w, h);
}
inline auto SizePolicy(ui::policies::Size p)
{
    return Call<ui::size::SetSizePolicy>(p);
}
inline auto Size(float w, float h)
{
    return Call<ui::size::SetSize>(w, h);
}
inline auto Position(float x, float y)
{
    return Call<ui::size::SetPosition>(x, y);
}
} // namespace ui::chains
