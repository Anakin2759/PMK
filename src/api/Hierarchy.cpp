#include "ui/api/Hierarchy.hpp"

#include "helper/Helper.hpp"

namespace ui::hierarchy
{
void RemoveChild(UiRuntime& runtime, ui::entity parent, ui::entity child)
{
    ui::detail::hierarchy::RemoveChild(runtime.registry(), detail::ToInternal(parent), detail::ToInternal(child));
}

void AddChild(UiRuntime& runtime, ui::entity parent, ui::entity child)
{
    ui::detail::hierarchy::AddChild(runtime.registry(), detail::ToInternal(parent), detail::ToInternal(child));
}

std::vector<ui::entity> ChildrenPostOrder(UiRuntime& runtime, ui::entity parent)
{
    std::vector<ui::entity> children;
    for (const auto CHILD : ui::detail::hierarchy::ChildrenPostOrder(runtime.registry(), detail::ToInternal(parent)))
    {
        children.push_back(detail::ToPublic(CHILD));
    }
    return children;
}

}  // namespace ui::hierarchy
