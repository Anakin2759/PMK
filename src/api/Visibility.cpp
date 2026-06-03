#include "Visibility.hpp"

#include "detail/EntityCast.hpp"
#include "detail/Visibility.hpp"

namespace ui::visibility
{

void SetVisible(ui::entity entity, bool visible)
{
    ui::detail::visibility::SetVisible(ui::detail::ToInternal(entity), visible);
}

void Show(ui::entity entity)
{
    ui::detail::visibility::Show(ui::detail::ToInternal(entity));
}

void Hide(ui::entity entity)
{
    ui::detail::visibility::Hide(ui::detail::ToInternal(entity));
}

void SetAlpha(ui::entity entity, float alpha)
{
    ui::detail::visibility::SetAlpha(ui::detail::ToInternal(entity), alpha);
}

void SetBackgroundColor(ui::entity entity, const Color& color)
{
    ui::detail::visibility::SetBackgroundColor(ui::detail::ToInternal(entity), color);
}

void SetBorderRadius(ui::entity entity, float radius)
{
    ui::detail::visibility::SetBorderRadius(ui::detail::ToInternal(entity), radius);
}

void SetBorderColor(ui::entity entity, const Color& color)
{
    ui::detail::visibility::SetBorderColor(ui::detail::ToInternal(entity), color);
}

void SetBorderThickness(ui::entity entity, float thickness)
{
    ui::detail::visibility::SetBorderThickness(ui::detail::ToInternal(entity), thickness);
}

} // namespace ui::visibility
