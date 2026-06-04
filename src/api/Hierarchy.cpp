#include "Hierarchy.hpp"

#include "Utils.hpp"
#include "detail/EntityCast.hpp"
#include "entt/entity/fwd.hpp"
#include "core/RuntimeFacade.hpp"
#include "common/components/Layout.hpp"
#include "entt/entity/entity.hpp"
#include "common/Tags.hpp"
namespace ui::hierarchy
{
namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}

void AppendChildrenPostOrder(Registry& reg, entt::entity parent, std::vector<ui::entity>& output)
{
    if (!reg.valid(parent)) return;

    const auto* hierarchy = reg.try_get<components::Hierarchy>(parent);
    if (hierarchy == nullptr || hierarchy->children.empty()) return;

    const auto childrenCopy = hierarchy->children;
    for (const entt::entity child : childrenCopy)
    {
        if (!reg.valid(child)) continue;
        AppendChildrenPostOrder(reg, child, output);
        output.push_back(detail::ToPublic(child));
    }
}
} // namespace

void RemoveChild(ui::entity parent, ui::entity child)
{
    auto& reg = CurrentRegistry();
    const entt::entity parentInternal = detail::ToInternal(parent);
    const entt::entity childInternal = detail::ToInternal(child);
    if (!reg.valid(parentInternal) || !reg.valid(childInternal)) return;

    auto* parentHierarchy = reg.try_get<components::Hierarchy>(parentInternal);
    auto* childHierarchy = reg.try_get<components::Hierarchy>(childInternal);

    if (parentHierarchy != nullptr && childHierarchy != nullptr && childHierarchy->parent == parentInternal)
    {
        auto& children = parentHierarchy->children;
        std::erase(children, childInternal);
        childHierarchy->parent = ::entt::null;

        // 子节点脱离父节点，重新成为根节点
        reg.emplace_or_replace<components::RootTag>(childInternal);

        utils::MarkLayoutAndVisualChanged(parent);
        utils::MarkLayoutAndVisualChanged(child);
    }
}

void AddChild(ui::entity parent, ui::entity child)
{
    auto& reg = CurrentRegistry();
    const entt::entity parentInternal = detail::ToInternal(parent);
    const entt::entity childInternal = detail::ToInternal(child);
    if (!reg.valid(parentInternal) || !reg.valid(childInternal)) return;

    auto& childHierarchy = reg.get_or_emplace<components::Hierarchy>(childInternal);
    if (childHierarchy.parent != ::entt::null && childHierarchy.parent != parentInternal)
    {
        RemoveChild(detail::ToPublic(childHierarchy.parent), child);
    }
    childHierarchy.parent = parentInternal;

    // 子节点不再是根节点，移除 RootTag
    reg.remove<components::RootTag>(childInternal);

    auto& parentHierarchy = reg.get_or_emplace<components::Hierarchy>(parentInternal);
    auto& children = parentHierarchy.children;
    bool alreadyChild = false;
    for (auto childEntity : children)
    {
        if (childEntity == childInternal)
        {
            alreadyChild = true;
            break;
        }
    }
    if (!alreadyChild)
    {
        children.push_back(childInternal);
    }

    utils::MarkLayoutAndVisualChanged(child);
}

std::vector<ui::entity> ChildrenPostOrder(ui::entity parent)
{
    std::vector<ui::entity> children;
    AppendChildrenPostOrder(CurrentRegistry(), detail::ToInternal(parent), children);
    return children;
}

} // namespace ui::hierarchy
