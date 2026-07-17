/**
 * ************************************************************************
 *
 * @file EntityTypes.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-30
 * @version 0.1
 * @brief ui::entity 实体句柄的单一权威定义。
 *
 * 过渡期仍映射到 entt::entity，以保持系统层和服务层既有内部调用可构建。
 * 后续按 `修改规划.md` 完成全量边界迁移后，再切换为 std::uint32_t。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>


namespace ui
{

using entity = uint32_t;

/// @brief 空实体常量，等价于 entt::null。
inline constexpr entity null_entity = std::numeric_limits<entity>::max();



} // namespace ui
