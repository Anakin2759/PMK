/**
 * ************************************************************************
 *
 * @file Text.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-09
 * @version 0.1
 * @brief 文本操作API
  - 提供对文本组件的便捷操作函数
  - 支持设置文本内容、样式和行为
  - 简化文本相关组件的使用
  - 提供对按钮文本和可编辑文本的专用接口
  - 方便系统和组件对文本进行动态修改
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <entt/entt.hpp>
#include <string>
#include "../common/Policies.hpp"
#include "../common/Components.hpp"
#include "Chains.hpp" // Changed: Include Chains.hpp for DSL

namespace ui::text
{
/**
 * @brief 设置按钮文本内容
 * @param entity 实体ID
 * @param content 文本内容
 */
void SetText(::entt::entity entity, const std::string& content);
/**
 * @brief 设置按钮是否可用
 * @param entity 实体ID
 * @param enabled 是否可用
 */
void SetButtonEnabled(::entt::entity entity, bool enabled);
/**
 * @brief 设置文本内容
 * @param entity 实体ID
 * @param content 文本内容
 */
void SetTextContent(::entt::entity entity, const std::string& content);
/**
 * @brief 设置文本换行模式
 * @param entity 实体ID
 * @param mode 换行模式
 */
void SetTextWordWrap(::entt::entity entity, policies::TextWrap mode);
/**
 * @brief 设置文本对齐方式
 * @param entity 实体ID
 * @param alignment 对齐方式
 */
void SetTextAlignment(::entt::entity entity, policies::Alignment alignment);
/**
 * @brief 设置文本颜色
 * @param entity 实体ID
 * @param color 颜色值
 */
void SetTextColor(::entt::entity entity, const Color& color);
/**
 * @brief 获取可编辑文本内容
 * @param entity 实体ID
 * @return 文本内容
 */
std::string GetTextEditContent(::entt::entity entity);
/**
 * @brief 设置可编辑文本内容
 * @param entity 实体ID
 * @param content 文本内容
 */
void SetTextEditContent(::entt::entity entity, const std::string& content);
/**
 * @brief 设置密码模式
 * @param entity 实体ID
 * @param enabled 是否启用密码模式
 */
void SetPasswordMode(::entt::entity entity, policies::TextFlag enabled);
/**
 * @brief 设置点击回调函数
 * @param entity 实体ID
 * @param callback 回调函数
 */
void SetClickCallback(::entt::entity entity, components::on_event<> callback);
/**
 * @brief 设置文本提交回调（单行模式按 Enter 触发）
 * @param entity 实体ID
 * @param callback 回调函数
 */
void SetOnSubmit(::entt::entity entity, components::on_event<> callback);
/**
 * @brief 设置文本改变回调
 * @param entity 实体ID
 * @param callback 回调函数（参数为新文本内容）
 */
void SetOnTextChanged(::entt::entity entity, components::on_event<const std::string&> callback);
/**
 * @brief 设置行高倍数
 * @param entity 实体ID
 * @param height 行高倍数 (例如 1.2)
 */
void SetLineHeight(::entt::entity entity, float height);
/**
 * @brief 设置字符间距
 * @param entity 实体ID
 * @param spacing 字符间距
 */
void SetCharacterSpacing(::entt::entity entity, float spacing);
/**
 * @brief 设置文本换行宽度 (用于多列或限制宽度)
 * @param entity 实体ID
 * @param width 宽度
 */
void SetTextWrapWidth(::entt::entity entity, float width);
/**
 * @brief 设置字体大小
 * @param entity 实体ID
 * @param size 字体大小
 */
void SetFontSize(::entt::entity entity, float size);

} // namespace ui::text

namespace ui::chains
{
inline auto Text(const std::string& c)
{
    return Call<ui::text::SetText>(c);
}
inline auto ButtonEnabled(bool e)
{
    return Call<ui::text::SetButtonEnabled>(e);
}
inline auto TextContent(const std::string& c)
{
    return Call<ui::text::SetTextContent>(c);
}
inline auto TextWordWrap(policies::TextWrap mode)
{
    return Call<ui::text::SetTextWordWrap>(mode);
}
inline auto TextAlignment(policies::Alignment align)
{
    return Call<ui::text::SetTextAlignment>(align);
}
inline auto TextColor(const Color& c)
{
    return Call<ui::text::SetTextColor>(c);
}
inline auto TextEditContent(const std::string& c)
{
    return Call<ui::text::SetTextEditContent>(c);
}
inline auto PasswordMode(policies::TextFlag e)
{
    return Call<ui::text::SetPasswordMode>(e);
}
inline auto OnClick(ui::components::on_event<> cb)
{
    return Call<ui::text::SetClickCallback>(std::move(cb));
}
inline auto OnSubmit(ui::components::on_event<> cb)
{
    return Call<ui::text::SetOnSubmit>(std::move(cb));
}
inline auto OnTextChanged(ui::components::on_event<const std::string&> cb)
{
    return Call<ui::text::SetOnTextChanged>(std::move(cb));
}
inline auto LineHeight(float h)
{
    return Call<ui::text::SetLineHeight>(h);
}
inline auto CharacterSpacing(float s)
{
    return Call<ui::text::SetCharacterSpacing>(s);
}
inline auto TextWrapWidth(float w)
{
    return Call<ui::text::SetTextWrapWidth>(w);
}
inline auto FontSize(float s)
{
    return Call<ui::text::SetFontSize>(s);
}
} // namespace ui::chains
