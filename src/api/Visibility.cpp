#include "ui/api/Visibility.hpp"

#include "helper/Helper.hpp"

namespace ui::visibility
{

void SetVisible(ui::entity entity, bool visible)
{
    ui::detail::visibility::SetVisible(UiRuntime::current().registry(), ui::detail::ToInternal(entity), visible);
}

void Show(ui::entity entity)
{
    auto& runtime = UiRuntime::current();
    ui::detail::visibility::Show(runtime.registry(), runtime.logger(), ui::detail::ToInternal(entity));
}

void Hide(ui::entity entity)
{
    ui::detail::visibility::Hide(UiRuntime::current().registry(), ui::detail::ToInternal(entity));
}

void SetAlpha(ui::entity entity, float alpha)
{
    ui::detail::visibility::SetAlpha(UiRuntime::current().registry(), ui::detail::ToInternal(entity), alpha);
}

void SetBackgroundColor(ui::entity entity, const Color& color)
{
    ui::detail::visibility::SetBackgroundColor(UiRuntime::current().registry(), ui::detail::ToInternal(entity), color);
}

void SetBorderRadius(ui::entity entity, float radius)
{
    ui::detail::visibility::SetBorderRadius(UiRuntime::current().registry(), ui::detail::ToInternal(entity), radius);
}

void SetBorderColor(ui::entity entity, const Color& color)
{
    ui::detail::visibility::SetBorderColor(UiRuntime::current().registry(), ui::detail::ToInternal(entity), color);
}

void SetBorderThickness(ui::entity entity, float thickness)
{
    ui::detail::visibility::SetBorderThickness(UiRuntime::current().registry(), ui::detail::ToInternal(entity), thickness);
}

}  // namespace ui::visibility
