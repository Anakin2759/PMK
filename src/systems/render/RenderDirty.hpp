#pragma once

#include <entt/entity/entity.hpp>

#include <unordered_set>
#include <vector>

#include "common/Tags.hpp"
#include "common/components/Layout.hpp"
#include "ui/Result.hpp"
#include "utils/Registry.hpp"

namespace ui::systems::render_detail
{

/**
 * @brief 清除一次成功窗口提交所消费的层级子树脏标记。
 *
 * 仅能在对应窗口 framebuffer 成功提交后调用。失效子节点与异常层级环会被忽略，
 * 脱离该窗口子树的实体不会因其他窗口成功而被清理。
 */
inline void ClearRenderDirtySubtree(Registry& registry, entt::entity windowEntity)
{
    std::vector<entt::entity> pending{windowEntity};
    std::unordered_set<entt::entity> visited;

    while (!pending.empty())
    {
        const auto entity = pending.back();
        pending.pop_back();
        if (!registry.valid(entity) || !visited.insert(entity).second)
        {
            continue;
        }

        registry.remove<components::RenderDirtyTag>(entity);
        if (const auto* hierarchy = registry.try_get<components::Hierarchy>(entity); hierarchy != nullptr)
        {
            pending.insert(pending.end(), hierarchy->children.begin(), hierarchy->children.end());
        }
    }
}

/**
 * @brief 根据窗口本帧提交结果提交 dirty 消费。
 * @return true 表示 framebuffer 已成功提交且 dirty 已清理；失败时保持原状供下一帧重试。
 */
[[nodiscard]] inline bool CommitRenderDirtyOnSuccess(Registry& registry, entt::entity windowEntity,
                                                     const ui::Result<void>& submission)
{
    if (!submission.has_value())
    {
        return false;
    }
    ClearRenderDirtySubtree(registry, windowEntity);
    return true;
}

}  // namespace ui::systems::render_detail
