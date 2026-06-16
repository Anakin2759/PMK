/**
 * ************************************************************************
 *
 * @file RendererRegistry.hpp
 * @brief 渲染器动态注册表 — 替代 RenderSystem 中硬编码的 12 个 if/else 分支
 *
 * ## 设计目标
 *
 * 将"控件类型 → 渲染器"的映射从 RenderSystem 核心中解耦，
 * 允许新增控件类型时仅需注册渲染器，无需修改 RenderSystem 代码。
 *
 * ## 用法
 *
 * @code
 * RendererRegistry registry(reg);
 *
 * // 按组件组合注册（实体需同时拥有 Text 和 TextContentTag）
 * registry.registerRenderer<TextRenderer>(reg, has<Text, TextContentTag>);
 *
 * // 按单一组件注册
 * registry.registerRenderer<ShapeRenderer>(reg, has<Background>);
 *
 * // 查找
 * if (auto* r = registry.findRenderer(entity)) {
 *     r->submit(entity, ctx);
 * }
 * @endcode
 *
 * ## 注册优先级
 *
 * 按注册顺序匹配（先注册先匹配）。需要更精细控制的渲染器（如 TableCell）
 * 应先注册，通用渲染器（如 ShapeRenderer）后注册。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <vector>

#include <entt/entt.hpp>

#include "interface/IRenderer.hpp"
#include "singleton/Registry.hpp"

namespace ui::renderer_registry
{

// =========================================================================
// § 1 渲染器工厂 concept
// =========================================================================

/// 渲染器必须实现 IRenderer 接口
template <typename T>
concept RegisteredRenderer = std::derived_from<T, core::IRenderer> && std::movable<T>;

// =========================================================================
// § 2 匹配谓词
// =========================================================================

/// 实体匹配谓词：给定实体，返回该渲染器是否能处理
using MatchPredicate = std::function<bool(entt::entity)>;

/// 辅助：检查实体是否拥有指定组件组
template <typename... Components>
[[nodiscard]] MatchPredicate hasComponents(Registry& reg)
{
    return [&reg](entt::entity e) -> bool { return reg.raw().all_of<Components...>(e); };
}

/// 辅助：检查实体是否拥有指定 Tag 组
template <typename... Tags>
[[nodiscard]] MatchPredicate hasTags(Registry& reg)
{
    return [&reg](entt::entity e) -> bool { return reg.raw().any_of<Tags...>(e); };
}

/// 辅助：组合多个谓词（AND 逻辑）
[[nodiscard]] inline MatchPredicate matchAll(std::vector<MatchPredicate> predicates)
{
    return [preds = std::move(predicates)](entt::entity e) -> bool
    {
        for (const auto& p : preds)
        {
            if (!p(e))
            {
                return false;
            }
        }
        return true;
    };
}

// =========================================================================
// § 3 RendererRegistry
// =========================================================================

/**
 * @brief 动态渲染器注册表
 *
 * 持有 (MatchPredicate, unique_ptr<IRenderer>) 对的有序列表。
 * 按注册顺序匹配——更具体的渲染器应先注册。
 */
class RendererRegistry
{
public:
    RendererRegistry() = default;
    ~RendererRegistry() = default;

    RendererRegistry(const RendererRegistry&) = delete;
    RendererRegistry& operator=(const RendererRegistry&) = delete;
    RendererRegistry(RendererRegistry&&) noexcept = default;
    RendererRegistry& operator=(RendererRegistry&&) noexcept = default;

    /**
     * @brief 注册渲染器
     * @tparam Renderer 具体渲染器类型（必须实现 IRenderer）
     * @param predicate 匹配谓词：返回 true 表示该渲染器可处理此实体
     * @param args 转发给 Renderer 构造函数的参数
     * @return 注册的渲染器指针（用于后续配置）
     */
    template <RegisteredRenderer Renderer, typename... Args>
    Renderer* registerRenderer(MatchPredicate predicate, Args&&... args)
    {
        auto renderer = std::make_unique<Renderer>(std::forward<Args>(args)...);
        auto* ptr = renderer.get();
        m_entries.emplace_back(std::move(predicate), std::move(renderer));
        return ptr;
    }

    /**
     * @brief 查找能处理指定实体的渲染器
     * @param entity 实体
     * @return 渲染器指针，或 nullptr（无匹配）
     */
    [[nodiscard]] core::IRenderer* findRenderer(entt::entity entity)
    {
        for (auto& [predicate, renderer] : m_entries)
        {
            if (predicate(entity))
            {
                return renderer.get();
            }
        }
        return nullptr;
    }

    /** @brief 获取已注册渲染器数量 */
    [[nodiscard]] size_t count() const noexcept { return m_entries.size(); }

    /** @brief 清空所有注册 */
    void clear() noexcept { m_entries.clear(); }

private:
    struct Entry
    {
        MatchPredicate predicate;
        std::unique_ptr<core::IRenderer> renderer;

        Entry(MatchPredicate p, std::unique_ptr<core::IRenderer> r)
            : predicate(std::move(p)), renderer(std::move(r))
        {
        }
    };

    std::vector<Entry> m_entries;
};

} // namespace ui::renderer_registry
