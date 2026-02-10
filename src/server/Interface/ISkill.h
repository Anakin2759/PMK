/**
 * ************************************************************************
 *
 * @file ISkill.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 技能接口定义
    所有技能均实现该接口
    提供使用技能和筛选目标的方法
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <entt/entt.hpp>
#include <entt/poly/poly.hpp>
#include "src/context/GameContext.h"
#include "absl/container/flat_hash_map.h"
struct ISkill : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void onUse() const { return this->template invoke<0>(*this); }
        absl::flat_hash_map<entt::entity, bool> filterTargets(entt::entity pt1) const
        {
            return this->template invoke<1>(*this, pt1);
        }
    };

    template <typename Type>
    using impl = entt::value_list<&Type::onUse, &Type::filterTargets>;
};