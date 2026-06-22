#include "Hierarchy.hpp"

#include "Utils.hpp"
#include "core/RuntimeFacade.hpp"
#include "common/components/Layout.hpp"
#include "entt/entity/entity.hpp"
#include "common/Tags.hpp"

namespace ui::detail::hierarchy
{
namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}

void AppendChildrenPostOrder(Registry& reg, entt::entity parent, std::vector<entt::entity>& output)
{
    if (!reg.valid(parent)) return;

    const auto* hierarchy = reg.try_get<components::Hierarchy>(parent);
    if (hierarchy == nullptr || hierarchy->children.empty()) return;

    const auto childrenCopy = hierarchy->children;
    for (const entt::entity child : childrenCopy)
    {
        if (!reg.valid(child)) continue;
        AppendChildrenPostOrder(reg, child, output);
        output.push_back(child);
    }
}
} // namespace

void RemoveChild(::entt::entity parent, ::entt::entity child)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(parent) || !reg.valid(child)) return;

    auto* parentHierarchy = reg.try_get<components::Hierarchy>(parent);
    auto* childHierarchy = reg.try_get<components::Hierarchy>(child);

    if (parentHierarchy != nullptr && childHierarchy != nullptr && childHierarchy->parent == parent)
    {
        auto& children = parentHierarchy->children;
        std::erase(children, child);
        childHierarchy->parent = ::entt::null;

        // 子节点脱离父节点，重新成为根节点
        reg.emplace_or_replace<components::RootTag>(child);

        utils::MarkLayoutAndVisualChanged(parent);
        utils::MarkLayoutAndVisualChanged(child);
    }
}

void AddChild(::entt::entity parent, ::entt::entity child)
{
    auto& reg = CurrentRegistry();
    if (!reg.valid(parent) || !reg.valid(child)) return;

    auto& childHierarchy = reg.get_or_emplace<components::Hierarchy>(child);
    if (childHierarchy.parent != ::entt::null && childHierarchy.parent != parent)
    {
        RemoveChild(childHierarchy.parent, child);
    }
    childHierarchy.parent = parent;

    // 子节点不再是根节点，移除 RootTag
    reg.remove<components::RootTag>(child);

    auto& parentHierarchy = reg.get_or_emplace<components::Hierarchy>(parent);
    auto& children = parentHierarchy.children;
    bool alreadyChild = false;
    for (auto childEntity : children)
    {
        if (childEntity == child)
        {
            alreadyChild = true;
            break;
        }
    }
    if (!alreadyChild)
    {
        children.push_back(child);
    }

    utils::MarkLayoutAndVisualChanged(child);
}

std::vector<entt::entity> ChildrenPostOrder(entt::entity parent)
{
    std::vector<entt::entity> children;
    AppendChildrenPostOrder(CurrentRegistry(), parent, children);
    return children;
}

} // namespace ui::detail::hierarchy
