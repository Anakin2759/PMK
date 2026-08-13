#include "ui/api/Query.hpp"

#include "helper/Helper.hpp"

namespace ui::query
{

bool IsValid(entity ent) noexcept
{
    return bridge::IsValid(detail::ToInternal(ent));
}

Result<entity> FindByAlias(std::string_view alias)
{
    return detail::ToPublic(bridge::FindByAlias(alias));
}

std::string GetAlias(entity ent)
{
    return bridge::GetAlias(detail::ToInternal(ent));
}

}  // namespace ui::query
