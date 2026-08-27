#include "ui/api/Visibility.hpp"

#include "helper/Helper.hpp"

namespace ui::visibility
{

void SetVisible(UiRuntime& runtime, ui::entity entity, bool visible)
{
    ui::detail::visibility::SetVisible(runtime.registry(), ui::detail::ToInternal(entity), visible);
}

void Show(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::visibility::Show(runtime.registry(), runtime.logger(), ui::detail::ToInternal(entity));
}

void Hide(UiRuntime& runtime, ui::entity entity)
{
    ui::detail::visibility::Hide(runtime.registry(), ui::detail::ToInternal(entity));
}

void SetAlpha(UiRuntime& runtime, ui::entity entity, float alpha)
{
    ui::detail::visibility::SetAlpha(runtime.registry(), ui::detail::ToInternal(entity), alpha);
}

void SetBackgroundColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    ui::detail::visibility::SetBackgroundColor(runtime.registry(), ui::detail::ToInternal(entity), color);
}

void SetBorderRadius(UiRuntime& runtime, ui::entity entity, float radius)
{
    ui::detail::visibility::SetBorderRadius(runtime.registry(), ui::detail::ToInternal(entity), radius);
}

void SetBorderColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    ui::detail::visibility::SetBorderColor(runtime.registry(), ui::detail::ToInternal(entity), color);
}

void SetBorderThickness(UiRuntime& runtime, ui::entity entity, float thickness)
{
    ui::detail::visibility::SetBorderThickness(runtime.registry(), ui::detail::ToInternal(entity), thickness);
}

}  // namespace ui::visibility
