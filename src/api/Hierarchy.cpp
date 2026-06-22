#include "Hierarchy.hpp"

#include "detail/EntityCast.hpp"
#include "detail/Hierarchy.hpp"

namespace ui::hierarchy
{
void RemoveChild(ui::entity parent, ui::entity child)
{
    ui::detail::hierarchy::RemoveChild(detail::ToInternal(parent), detail::ToInternal(child));
}

void AddChild(ui::entity parent, ui::entity child)
{
    ui::detail::hierarchy::AddChild(detail::ToInternal(parent), detail::ToInternal(child));
}

std::vector<ui::entity> ChildrenPostOrder(ui::entity parent)
{
    std::vector<ui::entity> children;
    for (const auto child : ui::detail::hierarchy::ChildrenPostOrder(detail::ToInternal(parent)))
    {
        children.push_back(detail::ToPublic(child));
    }
    return children;
}

} // namespace ui::hierarchy
