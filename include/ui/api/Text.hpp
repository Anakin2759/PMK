/**
 * @file Text.hpp
 * @brief 文本内容、样式和行为快捷操作 API。
 */
#pragma once

#include <string>
#include <utility>

#include "ui/Callback.hpp"
#include "ui/Color.hpp"
#include "ui/Policies.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui::text
{
void SetText(ui::entity entity, const std::string& content);
void SetButtonEnabled(ui::entity entity, bool enabled);
void SetTextContent(ui::entity entity, const std::string& content);
void SetTextWordWrap(ui::entity entity, policies::TextWrap mode);
void SetTextAlignment(ui::entity entity, policies::Alignment alignment);
void SetTextColor(ui::entity entity, const Color& color);
std::string GetTextEditContent(ui::entity entity);
void SetTextEditContent(ui::entity entity, const std::string& content);
void SetPasswordMode(ui::entity entity, policies::TextFlag enabled);
void SetClickCallback(ui::entity entity, Callback<> callback);
void SetOnSubmit(ui::entity entity, Callback<> callback);
void SetOnTextChanged(ui::entity entity, Callback<const std::string&> callback);
void SetLineHeight(ui::entity entity, float height);
void SetCharacterSpacing(ui::entity entity, float spacing);
void SetTextWrapWidth(ui::entity entity, float width);
void SetFontSize(ui::entity entity, float size);
}  // namespace ui::text

namespace ui::actions::text
{
inline constexpr EntityAction<static_cast<void (*)(ui::entity, const std::string&)>(
    &ui::text::SetText)>
    SET_TEXT_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, bool)>(&ui::text::SetButtonEnabled)>
    SET_BUTTON_ENABLED_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, const std::string&)>(
    &ui::text::SetTextContent)>
    SET_TEXT_CONTENT_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, policies::TextWrap)>(
    &ui::text::SetTextWordWrap)>
    SET_TEXT_WORD_WRAP_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, policies::Alignment)>(
    &ui::text::SetTextAlignment)>
    SET_TEXT_ALIGNMENT_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, const Color&)>(&ui::text::SetTextColor)>
    SET_TEXT_COLOR_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, const std::string&)>(
    &ui::text::SetTextEditContent)>
    SET_TEXT_EDIT_CONTENT_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, policies::TextFlag)>(
    &ui::text::SetPasswordMode)>
    SET_PASSWORD_MODE_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, Callback<>)>(&ui::text::SetClickCallback)>
    SET_CLICK_CALLBACK_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, Callback<>)>(&ui::text::SetOnSubmit)>
    SET_ON_SUBMIT_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, Callback<const std::string&>)>(
    &ui::text::SetOnTextChanged)>
    SET_ON_TEXT_CHANGED_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, float)>(&ui::text::SetLineHeight)>
    SET_LINE_HEIGHT_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, float)>(&ui::text::SetCharacterSpacing)>
    SET_CHARACTER_SPACING_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, float)>(&ui::text::SetTextWrapWidth)>
    SET_TEXT_WRAP_WIDTH_ACTION{};
inline constexpr EntityAction<static_cast<void (*)(ui::entity, float)>(&ui::text::SetFontSize)>
    SET_FONT_SIZE_ACTION{};
}  // namespace ui::actions::text

namespace ui::chains
{
inline auto Text(const std::string& content)
{
    return ui::actions::text::SET_TEXT_ACTION.bind(content);
}
inline auto ButtonEnabled(bool enabled)
{
    return ui::actions::text::SET_BUTTON_ENABLED_ACTION.bind(enabled);
}
inline auto TextContent(const std::string& content)
{
    return ui::actions::text::SET_TEXT_CONTENT_ACTION.bind(content);
}
inline auto TextWordWrap(policies::TextWrap mode)
{
    return ui::actions::text::SET_TEXT_WORD_WRAP_ACTION.bind(mode);
}
inline auto TextAlignment(policies::Alignment align)
{
    return ui::actions::text::SET_TEXT_ALIGNMENT_ACTION.bind(align);
}
inline auto TextColor(const Color& color)
{
    return ui::actions::text::SET_TEXT_COLOR_ACTION.bind(color);
}
inline auto TextEditContent(const std::string& content)
{
    return ui::actions::text::SET_TEXT_EDIT_CONTENT_ACTION.bind(content);
}
inline auto PasswordMode(policies::TextFlag enabled)
{
    return ui::actions::text::SET_PASSWORD_MODE_ACTION.bind(enabled);
}
inline auto OnClick(ui::Callback<> callback)
{
    return ui::actions::text::SET_CLICK_CALLBACK_ACTION.bind(std::move(callback));
}
inline auto OnSubmit(ui::Callback<> callback)
{
    return ui::actions::text::SET_ON_SUBMIT_ACTION.bind(std::move(callback));
}
inline auto OnTextChanged(ui::Callback<const std::string&> callback)
{
    return ui::actions::text::SET_ON_TEXT_CHANGED_ACTION.bind(std::move(callback));
}
inline auto LineHeight(float height)
{
    return ui::actions::text::SET_LINE_HEIGHT_ACTION.bind(height);
}
inline auto CharacterSpacing(float spacing)
{
    return ui::actions::text::SET_CHARACTER_SPACING_ACTION.bind(spacing);
}
inline auto TextWrapWidth(float width)
{
    return ui::actions::text::SET_TEXT_WRAP_WIDTH_ACTION.bind(width);
}
inline auto FontSize(float size)
{
    return ui::actions::text::SET_FONT_SIZE_ACTION.bind(size);
}
}  // namespace ui::chains