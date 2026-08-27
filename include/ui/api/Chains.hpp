/**
 * ************************************************************************
 *
 * @file Chains.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-09
 * @version 0.1
 * @brief UI API 的链式调用扩展
 *
 * 实现参数外置链式调用。
 * 允许使用管道操作符 | 对实体进行连续配置。
 * 不需要保存和合并，直接在实体上应用一系列设置。
 *
 * 用法示例（模块内零开销组合，Chain<F> 就地内联）:
 *   using namespace ui::chains;
 *   entity | Size(100, 40) | BackgroundColor(Color::Red()) | Show();
 *
 * 跨模块存储/传递时使用 AnyChain（concept-model 类型擦除，类型稳定）:
 *   AnyChain style = AnyChain{Size(100, 40) | BackgroundColor(Color::Blue())};
 *   AnyChain combined = std::move(style) | AnyChain{Show()};
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "ui/api/Entity.hpp"
#include <concepts>
#include <functional>
#include <memory>
#include <utility>

namespace ui
{
class UiRuntime;
}

namespace ui::actions
{
template <auto Fn>
struct EntityAction;
}  // namespace ui::actions

namespace ui::chains
{
struct ChainActionTag
{
};

template <typename T>
concept Action = std::invocable<T, UiRuntime&, ui::entity>;

template <typename T>
concept TaggedAsChainAction = std::derived_from<std::remove_cvref_t<T>, ChainActionTag> ||
                              requires { typename std::remove_cvref_t<T>::is_chain_action_tag; };

template <typename T>
concept ChainAction = Action<T> && TaggedAsChainAction<T>;

class AnyChain : public ChainActionTag
{
   public:
    using is_chain_action_tag = void;

    struct Concept
    {
        Concept() = default;
        Concept(const Concept&) = delete;
        Concept& operator=(const Concept&) = delete;
        Concept(Concept&&) = delete;
        Concept& operator=(Concept&&) = delete;
        virtual void invoke(UiRuntime&, ui::entity) = 0;
        virtual ~Concept() = default;
    };

    template <ChainAction F>
        requires(!std::same_as<std::decay_t<F>, AnyChain>)
    explicit AnyChain(F&& action) : m_impl(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(action)))
    {
    }

    AnyChain(AnyChain&&) noexcept = default;
    AnyChain& operator=(AnyChain&&) noexcept = default;
    AnyChain(const AnyChain&) = delete;
    AnyChain& operator=(const AnyChain&) = delete;
    ~AnyChain() = default;

    void operator()(UiRuntime& runtime, ui::entity entity)
    {
        m_impl->invoke(runtime, entity);
    }

    friend AnyChain operator|(AnyChain&& lhs, AnyChain&& rhs);

   private:
    template <ChainAction F>
    struct Model final : Concept
    {
        F func;
        explicit Model(F callable) : func(std::move(callable))
        {
        }
        void invoke(UiRuntime& runtime, ui::entity entity) override
        {
            std::invoke(func, runtime, entity);
        }
    };

    explicit AnyChain(std::unique_ptr<Concept> impl) noexcept : m_impl(std::move(impl))
    {
    }

    std::unique_ptr<Concept> m_impl;
};

inline AnyChain operator|(AnyChain&& lhs, AnyChain&& rhs)
{
    struct Combined final : AnyChain::Concept
    {
        AnyChain left, right;
        Combined(AnyChain lhsChain, AnyChain rhsChain) : left(std::move(lhsChain)), right(std::move(rhsChain))
        {
        }
        void invoke(UiRuntime& runtime, ui::entity entity) override
        {
            left(runtime, entity);
            right(runtime, entity);
        }
    };
    return AnyChain{std::make_unique<Combined>(std::move(lhs), std::move(rhs))};
}

template <Action F>
struct Chain : ChainActionTag
{
    using is_chain_action_tag = void;

    F func;

    constexpr explicit Chain(F&& callable) : func(std::move(callable))
    {
    }

    template <typename Self, ChainAction Next>
    auto operator|(this Self&& self, Next&& next)
    {
        auto combined =
            [lhs = std::forward_like<Self>(self.func), rhs = std::forward<Next>(next)](UiRuntime& runtime,
                                                                                         ui::entity entity) mutable
        {
            std::invoke(lhs, runtime, entity);
            std::invoke(rhs, runtime, entity);
        };
        return Chain<decltype(combined)>{std::move(combined)};
    }

    void operator()(this auto&& self, UiRuntime& runtime, ui::entity entity)
    {
        std::invoke(std::forward_like<decltype(self)>(self.func), runtime, entity);
    }
};

template <typename F>
Chain(F&&) -> Chain<std::decay_t<F>>;

template <Action F>
ui::entity WithRuntime(UiRuntime& runtime, ui::entity entity, Chain<F>& chain)
{
    chain(runtime, entity);
    return entity;
}

template <Action F>
ui::entity WithRuntime(UiRuntime& runtime, ui::entity entity, const Chain<F>& chain)
{
    chain(runtime, entity);
    return entity;
}

template <Action F>
ui::entity WithRuntime(UiRuntime& runtime, ui::entity entity, Chain<F>&& chain)
{
    std::move(chain)(runtime, entity);
    return entity;
}

inline ui::entity WithRuntime(UiRuntime& runtime, ui::entity entity, AnyChain& chain)
{
    chain(runtime, entity);
    return entity;
}

inline ui::entity WithRuntime(UiRuntime& runtime, ui::entity entity, AnyChain&& chain)
{
    auto owned = std::move(chain);
    owned(runtime, entity);
    return entity;
}

template <auto Func, typename... Args>
auto Call(Args&&... args)
{
    return ui::actions::EntityAction<Func>{}.bind(std::forward<Args>(args)...);
}

}  // namespace ui::chains

namespace ui::actions
{
template <auto Fn>
struct EntityAction
{
    template <typename... Args>
    void operator()(UiRuntime& runtime, ui::entity entity, Args&&... args) const
    {
        Fn(runtime, entity, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto bind(Args&&... args) const
    {
        return ui::chains::Chain{[... args = std::forward<Args>(args)](UiRuntime& runtime, ui::entity entity) mutable
                     { Fn(runtime, entity, std::move(args)...); }};
    }
};
}  // namespace ui::actions