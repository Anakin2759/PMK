#include "ui/api/Icon.hpp"

#include <string>
#include <cstdint>

#include "helper/Helper.hpp"

namespace ui::icon
{

void SetIcon(UiRuntime& runtime, ui::entity entity, const std::string& textureId, policies::IconFlag iconflag, float iconSize,
             float spacing)
{
    ui::detail::icon::SetIcon(runtime.registry(), ui::detail::ToInternal(entity), textureId, iconflag, iconSize, spacing);
}

void SetIcon(UiRuntime& runtime, ui::entity entity, const std::string& fontName, uint32_t codepoint, policies::IconFlag iconflag,
             float iconSize, float spacing)
{
    ui::detail::icon::SetIcon(runtime.registry(), ui::detail::ToInternal(entity), fontName, codepoint, iconflag, iconSize, spacing);
}

void RemoveIcon(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::icon::RemoveIcon(runtime.registry(), ui::detail::ToInternal(entity));
}
}  // namespace ui::icon