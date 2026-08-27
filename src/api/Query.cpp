#include "ui/api/Query.hpp"

#include "helper/Helper.hpp"

namespace ui::query
{

bool IsValid(entity ent) noexcept
{
    return bridge::IsValid(UiRuntime::current().registry(), detail::ToInternal(ent));
}

Result<entity> FindByAlias(std::string_view alias)
{
    return detail::ToPublic(bridge::FindByAlias(UiRuntime::current().registry(), alias));
}

std::string GetAlias(entity ent)
{
    return bridge::GetAlias(UiRuntime::current().registry(), detail::ToInternal(ent));
}

}  // namespace ui::query
