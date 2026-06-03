#include "Layout.hpp"
#include "Scale.hpp"
#include "core/RuntimeFacade.hpp"
#include "common/Policies.hpp"
#include "Utils.hpp"
#include "entt/entity/fwd.hpp"
#include "common/components/Layout.hpp"
#include <algorithm>
namespace ui::detail::layout
{
namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}
} // namespace

void SetLayoutDirection(::entt::entity entity, policies::LayoutDirection direction)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& layout = reg.get_or_emplace<components::LayoutInfo>(entity);
    layout.direction = direction;
    ui::utils::MarkLayoutDirty(entity);
}

void SetLayoutSpacing(::entt::entity entity, float spacing)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    if (auto* layout = reg.try_get<components::LayoutInfo>(entity))
    {
        layout->spacing = std::max(0.0F, ui::scale::Metric(spacing));
        ui::utils::MarkLayoutDirty(entity);
    }
}

void SetPadding(::entt::entity entity, float left, float top, float right, float bottom)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& padding = reg.get_or_emplace<components::Padding>(entity);
    padding.values = Vec4(ui::scale::Metric(top), ui::scale::Metric(right), ui::scale::Metric(bottom), ui::scale::Metric(left));
    ui::utils::MarkLayoutDirty(entity);
}

void SetPadding(::entt::entity entity, float padding)
{
    SetPadding(entity, padding, padding, padding, padding);
}

void CenterInParent(::entt::entity entity)
{
    ui::utils::MarkLayoutDirty(entity);
}

} // namespace ui::detail::layout
