#include "Query.hpp"

#include "common/ErrorCodes.hpp"
#include "common/components/Data.hpp"
#include "core/RuntimeFacade.hpp"
#include "detail/EntityCast.hpp"

namespace ui::query
{

bool IsValid(entity ent) noexcept
{
    return RuntimeFacade::current().registry().valid(detail::ToInternal(ent));
}

Result<entity> FindByAlias(std::string_view alias)
{
    if (alias.empty()) return MakeError(UiErrc::INVALID_ARGUMENT);

    auto& reg = RuntimeFacade::current().registry();
    for (auto ent : reg.view<components::BaseInfo>())
    {
        if (reg.get<components::BaseInfo>(ent).alias == alias) return detail::ToPublic(ent);
    }
    return MakeError(UiErrc::INVALID_ENTITY);
}

std::string GetAlias(entity ent)
{
    auto& reg = RuntimeFacade::current().registry();
    const auto internalEntity = detail::ToInternal(ent);
    if (!reg.valid(internalEntity)) return {};
    const auto* info = reg.try_get<components::BaseInfo>(internalEntity);
    return info != nullptr ? info->alias : std::string{};
}

} // namespace ui::query
