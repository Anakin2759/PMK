/**
 * ************************************************************************
 *
 * @file EntityCast.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-06-03
 * @version 0.1
 * @brief 公开 ui::entity 与内部 entt::entity 的边界转换工具。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <type_traits>
#include <utility>

#include "common/EntityTypes.hpp"
#include "common/Result.hpp"
#include "entt/entity/entity.hpp"

namespace ui::detail
{

static_assert(sizeof(ui::entity) == sizeof(entt::entity), "ui::entity must match entt::entity storage size");
static_assert(ui::null_entity == static_cast<ui::entity>(entt::null), "ui::null_entity must match entt::null");

[[nodiscard]] constexpr entt::entity ToInternal(ui::entity entity) noexcept
{
    return static_cast<entt::entity>(entity);
}

[[nodiscard]] constexpr ui::entity ToPublic(entt::entity entity) noexcept
{
    return static_cast<ui::entity>(entity);
}
/**
 * @brief 将内部 entt::entity 的 Result 转换为公开的 ui::entity 的 Result。
 * @param result 内部 entt::entity 的 Result。
 * @return ui::Result<ui::entity> 转换后的公开 ui::entity 的 Result。
 */
[[nodiscard]] inline ui::Result<ui::entity> ToPublic(ui::Result<entt::entity>&& result)
{
    if (!result)
    {
        return std::unexpected(result.error());
    }
    return ToPublic(*result);
}
/**
 * @brief 将内部 entt::entity 的 Result 转换为公开的 ui::entity 的 Result（const 版本）。
 * @param result 内部 entt::entity 的 Result（const 版本）。
 * @return ui::Result<ui::entity> 转换后的公开 ui::entity 的 Result。
 */
[[nodiscard]] inline ui::Result<ui::entity> ToPublic(const ui::Result<entt::entity>& result)
{
    if (!result)
    {
        return std::unexpected(result.error());
    }
    return ToPublic(*result);
}

} // namespace ui::detail
