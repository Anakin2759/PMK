#include "Size.hpp"
#include "Scale.hpp"
#include "core/UiRuntime.hpp"
#include "Utils.hpp"
#include "entt/entity/fwd.hpp"
#include "common/components/Layout.hpp"
#include "common/Policies.hpp"

namespace ui::detail::size
{
namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return UiRuntime::current().registry();
}
} // namespace

void SetFixedSize(::entt::entity entity, float width, float height)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;

    auto& size = reg.get_or_emplace<components::Size>(entity);
    size.sizePolicy = policies::Size::FIXED;
    size.size = {scale::Metric(width), scale::Metric(height)};
    ui::utils::MarkLayoutDirty(entity);
}

void SetSizePolicy(::entt::entity entity, policies::Size policy)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& size = reg.get_or_emplace<components::Size>(entity);
    size.sizePolicy = policy;
    ui::utils::MarkLayoutDirty(entity);
}

void SetSize(::entt::entity entity, float width, float height)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& size = reg.get_or_emplace<components::Size>(entity);
    size.size = {scale::Metric(width), scale::Metric(height)};
    ui::utils::MarkLayoutDirty(entity);
}

void SetPosition(::entt::entity entity, float positionX, float positionY)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(entity)) return;
    auto& pos = reg.get_or_emplace<components::Position>(entity);
    pos.value = {scale::Metric(positionX), scale::Metric(positionY)};
    ui::utils::MarkLayoutDirty(entity);
}

} // namespace ui::detail::size
