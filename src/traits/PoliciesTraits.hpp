/**
 * ************************************************************************
 *
 * @file PoliciesTraits.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-23
 * @version 0.1
 * @brief 对UI策略枚举进行模板特性检测,增加策略enumclass 自动位运算支持
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <utility> // for std::to_underlying (C++23)

#include "Contains.hpp"
#include "ui/Policies.hpp"
#include <cstdint>

namespace ui::policies
{
enum class Size : uint8_t;
enum class LayoutDirection : uint8_t;
enum class Alignment : uint8_t;
enum class Play : uint8_t;
enum class Easing : uint8_t;
enum class WindowFlag : uint8_t;
enum class Position : uint8_t;
enum class TextFlag : uint16_t;
enum class ScrollBar : uint8_t;
enum class IconFlag : uint8_t;
} // namespace ui::policies

namespace ui::traits
{

// ===================== 元编程辅助工具 =====================

// ===================== 组件列表 =====================
using policies = TypeList<ui::policies::Size,
                          ui::policies::Position,
                          ui::policies::LayoutDirection,
                          ui::policies::Alignment,
                          ui::policies::Play,
                          ui::policies::Easing,
                          ui::policies::WindowFlag,
                          ui::policies::TextFlag,
                          ui::policies::TextFlag,
                          ui::policies::ScrollBar,
                          ui::policies::IconFlag>;

// ===================== 策略检测 =====================

/**
 * @brief
 */
template <typename T>
inline constexpr bool is_policies_v = contains_v<T, policies>; // NOLINT

/**
 * @brief 概念 Definitions (C++20)
 * 约束类型必须是策略枚举列表中的一员
 */
template <typename T>
concept Policies = is_policies_v<T>;

} // namespace ui::traits

// 位运算与 HasFlag 的权威实现位于公开头 ui/Policies.hpp。
