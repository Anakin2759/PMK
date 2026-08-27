#include "ui/api/Size.hpp"

#include "helper/Helper.hpp"

namespace ui::size
{

void SetFixedSize(UiRuntime& runtime, ui::entity entity, float width, float height)
{
    ui::detail::size::SetFixedSize(runtime.registry(), ui::detail::ToInternal(entity), width, height);
}


void SetSizePolicy(UiRuntime& runtime, ui::entity entity, policies::Size policy)
{
    ui::detail::size::SetSizePolicy(runtime.registry(), ui::detail::ToInternal(entity), policy);
}


void SetSize(UiRuntime& runtime, ui::entity entity, float width, float height)
{
    ui::detail::size::SetSize(runtime.registry(), ui::detail::ToInternal(entity), width, height);
}


void SetPosition(UiRuntime& runtime, ui::entity entity, float positionX, float positionY)
{
    ui::detail::size::SetPosition(runtime.registry(), ui::detail::ToInternal(entity), positionX, positionY);
}


}  // namespace ui::size
