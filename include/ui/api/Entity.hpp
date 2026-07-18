/**
 * ************************************************************************
 *
 * @file Entity.hpp
 * @brief ui::entity 公开 API 层对实体句柄别名的再导出。
 *
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <limits>

namespace ui
{

using entity = std::uint32_t;

/// @brief 空实体常量。
inline constexpr entity null_entity = std::numeric_limits<entity>::max();

} // namespace ui
