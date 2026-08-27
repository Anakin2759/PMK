#include "ui/api/Query.hpp"

#include "helper/Helper.hpp"

namespace ui::query
{

bool IsValid(UiRuntime& runtime, entity ent) noexcept
{
    return bridge::IsValid(runtime.registry(), detail::ToInternal(ent));
}

Result<entity> FindByAlias(UiRuntime& runtime, std::string_view alias)
{
    return detail::ToPublic(bridge::FindByAlias(runtime.registry(), alias));
}

std::string GetAlias(UiRuntime& runtime, entity ent)
{
    return bridge::GetAlias(runtime.registry(), detail::ToInternal(ent));
}

}  // namespace ui::query
