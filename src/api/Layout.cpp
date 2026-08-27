#include "ui/api/Layout.hpp"

#include "helper/Helper.hpp"

namespace ui::layout
{

void SetLayoutDirection(ui::entity entity, policies::LayoutDirection direction)
{
    ui::detail::layout::SetLayoutDirection(UiRuntime::current().registry(), ui::detail::ToInternal(entity), direction);
}

void SetLayoutSpacing(ui::entity entity, float spacing)
{
    ui::detail::layout::SetLayoutSpacing(UiRuntime::current().registry(), ui::detail::ToInternal(entity), spacing);
}

void SetPadding(ui::entity entity, float left, float top, float right, float bottom)
{
    ui::detail::layout::SetPadding(UiRuntime::current().registry(), ui::detail::ToInternal(entity), left, top, right, bottom);
}

void SetPadding(ui::entity entity, float padding)
{
    ui::detail::layout::SetPadding(UiRuntime::current().registry(), ui::detail::ToInternal(entity), padding);
}

void CenterInParent(ui::entity entity)
{
    ui::detail::layout::CenterInParent(UiRuntime::current().registry(), ui::detail::ToInternal(entity));
}

}  // namespace ui::layout
