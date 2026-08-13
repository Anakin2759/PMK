/**
 * ************************************************************************
 *
 * @file Query.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-30
 * @version 0.1
 * @brief 实体查询 API
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <string>
#include <string_view>

#include "ui/Result.hpp"
#include "ui/api/Entity.hpp"

namespace ui::query
{
[[nodiscard]] bool IsValid(entity ent) noexcept;
[[nodiscard]] Result<entity> FindByAlias(std::string_view alias);
[[nodiscard]] std::string GetAlias(entity ent);
}  // namespace ui::query
