#include "ui/api/Icon.hpp"

#include <string>
#include <cstdint>

#include "helper/Helper.hpp"

namespace ui::icon
{

void SetIcon(
    ui::entity entity, const std::string& textureId, policies::IconFlag iconflag, float iconSize, float spacing)
{
    ui::detail::icon::SetIcon(ui::detail::ToInternal(entity), textureId, iconflag, iconSize, spacing);
}

void SetIcon(ui::entity entity,
             const std::string& fontName,
             uint32_t codepoint,
             policies::IconFlag iconflag,
             float iconSize,
             float spacing)
{
    ui::detail::icon::SetIcon(ui::detail::ToInternal(entity), fontName, codepoint, iconflag, iconSize, spacing);
}

void RemoveIcon(ui::entity entity)
{
    ui::detail::icon::RemoveIcon(ui::detail::ToInternal(entity));
}
} // namespace ui::icon