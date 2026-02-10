/**
 * ************************************************************************
 *
 * @file Skill.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-18
 * @version 0.1
 * @brief
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <string>
#include <functional>
#include <entt/entt.hpp>
#include <array>
#include "src/shared/common/Common.h"

struct MetaSkillInfo
{
    absl::string name;
    absl::string description; // 新增描述字段
    bool needTarget = true;
    uint8_t maxTargets = 1;
    uint8_t minTargets = 1;
};
