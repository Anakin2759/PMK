/**
 * ************************************************************************
 *
 * @file SkillSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-03
 * @version 0.1
 * @brief 技能系统定义 负责技能的创建与管理 注册技能相关事件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <absl/container/flat_hash_map.h>
#include "src/server/Interface/ISkill.h"
class SkillSystem
{
public:
    explicit SkillSystem(GameContext& context) : m_context(&context) {};

    void registerEvents() {}
    void unregisterEvents() {}

private:
    GameContext* m_context;
    absl::flat_hash_map<std::string, entt::poly<ISkill>> m_skillMap;
};