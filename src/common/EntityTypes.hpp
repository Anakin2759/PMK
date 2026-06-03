/**
 * ************************************************************************
 *
 * @file EntityTypes.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-30
 * @version 0.1
 * @brief entt::entity 别名的单一权威定义。
 *
 * 定义放在 common 层，使 Events.hpp 等公共头文件可以用 entt::entity
 * 而不直接暴露 entt::entity 给外部消费者。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <entt/entt.hpp>

namespace ui
{


using entity = std::uint32_t;

/// @brief 空实体常量，等价于 entt::null。
inline constexpr entity null_entity = 0xffffffff;

} // namespace ui
