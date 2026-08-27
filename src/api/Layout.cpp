#include "ui/api/Layout.hpp"

#include "helper/Helper.hpp"

namespace ui::layout
{

void SetLayoutDirection(UiRuntime& runtime, ui::entity entity, policies::LayoutDirection direction)
{
    ui::detail::layout::SetLayoutDirection(runtime.registry(), ui::detail::ToInternal(entity), direction);
}

void SetLayoutSpacing(UiRuntime& runtime, ui::entity entity, float spacing)
{
    ui::detail::layout::SetLayoutSpacing(runtime.registry(), ui::detail::ToInternal(entity), spacing);
}

void SetPadding(UiRuntime& runtime, ui::entity entity, float left, float top, float right, float bottom)
{
    ui::detail::layout::SetPadding(runtime.registry(), ui::detail::ToInternal(entity), left, top, right, bottom);
}

void SetPadding(UiRuntime& runtime, ui::entity entity, float padding)
{
    ui::detail::layout::SetPadding(runtime.registry(), ui::detail::ToInternal(entity), padding);
}

void CenterInParent(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::layout::CenterInParent(runtime.registry(), ui::detail::ToInternal(entity));
}

}  // namespace ui::layout
