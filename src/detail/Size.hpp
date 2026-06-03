#pragma once

#include "common/Policies.hpp"
#include "entt/entity/fwd.hpp"

namespace ui::detail::size
{
void SetFixedSize(entt::entity entity, float width, float height);
void SetSizePolicy(entt::entity entity, policies::Size policy);
void SetSize(entt::entity entity, float width, float height);
void SetPosition(entt::entity entity, float positionX, float positionY);

} // namespace ui::detail::size
