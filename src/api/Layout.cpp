#include "Layout.hpp"

#include "detail/EntityCast.hpp"
#include "detail/Layout.hpp"

namespace ui::layout
{

void SetLayoutDirection(ui::entity entity, policies::LayoutDirection direction)
{
    ui::detail::layout::SetLayoutDirection(ui::detail::ToInternal(entity), direction);
}

void SetLayoutSpacing(ui::entity entity, float spacing)
{
    ui::detail::layout::SetLayoutSpacing(ui::detail::ToInternal(entity), spacing);
}

void SetPadding(ui::entity entity, float left, float top, float right, float bottom)
{
    ui::detail::layout::SetPadding(ui::detail::ToInternal(entity), left, top, right, bottom);
}

void SetPadding(ui::entity entity, float padding)
{
    ui::detail::layout::SetPadding(ui::detail::ToInternal(entity), padding);
}

void CenterInParent(ui::entity entity)
{
    ui::detail::layout::CenterInParent(ui::detail::ToInternal(entity));
}

} // namespace ui::layout
