#include "ui/api/Size.hpp"

#include "helper/Helper.hpp"

namespace ui::size
{

void SetFixedSize(ui::entity entity, float width, float height)
{
    ui::detail::size::SetFixedSize(ui::detail::ToInternal(entity), width, height);
}

void SetSizePolicy(ui::entity entity, policies::Size policy)
{
    ui::detail::size::SetSizePolicy(ui::detail::ToInternal(entity), policy);
}

void SetSize(ui::entity entity, float width, float height)
{
    ui::detail::size::SetSize(ui::detail::ToInternal(entity), width, height);
}

void SetPosition(ui::entity entity, float positionX, float positionY)
{
    ui::detail::size::SetPosition(ui::detail::ToInternal(entity), positionX, positionY);
}

}  // namespace ui::size
