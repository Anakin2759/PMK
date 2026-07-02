#include "QueryBridge.hpp"

#include "common/ErrorCodes.hpp"
#include "common/components/Data.hpp"
#include "core/UiRuntime.hpp"

namespace ui::query::bridge
{

bool IsValid(entt::entity ent) noexcept
{
    return UiRuntime::current().registry().valid(ent);
}

Result<entt::entity> FindByAlias(std::string_view alias)
{
    if (alias.empty()) return Err(UiErrc::INVALID_ARGUMENT, "empty alias");

    auto& reg = UiRuntime::current().registry();
    for (auto ent : reg.view<components::BaseInfo>())
    {
        if (reg.get<components::BaseInfo>(ent).alias == alias) return ent;
    }
    return Err(UiErrc::INVALID_ENTITY, std::string(alias));
}

std::string GetAlias(entt::entity ent)
{
    auto& reg = UiRuntime::current().registry();
    if (!reg.valid(ent)) return {};
    const auto* info = reg.try_get<components::BaseInfo>(ent);
    return info != nullptr ? info->alias : std::string{};
}

} // namespace ui::query::bridge
