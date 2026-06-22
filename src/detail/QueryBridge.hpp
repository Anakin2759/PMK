#pragma once

#include <string>
#include <string_view>

#include "common/Result.hpp"
#include "entt/entity/fwd.hpp"

namespace ui::query::bridge
{

[[nodiscard]] bool IsValid(entt::entity entity) noexcept;
[[nodiscard]] Result<entt::entity> FindByAlias(std::string_view alias);
[[nodiscard]] std::string GetAlias(entt::entity entity);

} // namespace ui::query::bridge
