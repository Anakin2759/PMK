#include "Icon.hpp"
#include <unordered_set>
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"
#include "../common/Policies.hpp"
namespace ui::icon
{
void SetIcon(
    entt::entity entity, const std::string& textureId, policies::IconFlag iconflag, float iconSize, float spacing)
{
    if (!Registry::Valid(entity)) return;

    auto& icon = Registry::GetOrEmplace<components::Icon>(entity);
    icon.type |= policies::IconFlag::Texture;
    icon.textureId = textureId;
    icon.fontHandle = nullptr;
    icon.codepoint = 0;

    icon.size = {iconSize, iconSize};
    icon.spacing = spacing;

    // 标记布局需要重新计算
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
}

void SetIcon(entt::entity entity,
             const std::string& fontName,
             uint32_t codepoint,
             policies::IconFlag iconflag,
             float iconSize,
             float spacing)
{
    if (!Registry::Valid(entity)) return;

    auto& icon = Registry::GetOrEmplace<components::Icon>(entity);
    icon.type |= ~policies::IconFlag::Texture;
    // 暂时将字体名转换为 const char* 存储在 fontHandle 中，IconRenderer 会读取它
    // 理想情况下应该重构 Icon 组件，但为了保持兼容性暂且如此
    static std::unordered_set<std::string> fontNamePool;
    auto [it, inserted] = fontNamePool.insert(fontName);
    icon.fontHandle = (void*)it->c_str();

    icon.codepoint = codepoint;
    icon.textureId = "";
    icon.size = {iconSize, iconSize};
    icon.spacing = spacing;

    // 标记布局需要重新计算
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
}

void RemoveIcon(entt::entity entity)
{
    if (!Registry::Valid(entity)) return;
    if (Registry::AnyOf<components::Icon>(entity))
    {
        Registry::Remove<components::Icon>(entity);
        Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
    }
}
} // namespace ui::icon