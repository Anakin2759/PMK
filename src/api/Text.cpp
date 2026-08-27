#include "ui/api/Text.hpp"

#include <string>
#include <utility>

#include "helper/Helper.hpp"

namespace ui::text
{

namespace
{
Registry& CurrentRegistry()
{
    return UiRuntime::current().registry();
}
}  // namespace

void SetText(ui::entity entity, const std::string& content)
{
    detail::text::SetText(CurrentRegistry(), detail::ToInternal(entity), content);
}

void SetButtonEnabled(ui::entity entity, bool enabled)
{
    detail::text::SetButtonEnabled(CurrentRegistry(), detail::ToInternal(entity), enabled);
}

void SetTextContent(ui::entity entity, const std::string& content)
{
    detail::text::SetTextContent(CurrentRegistry(), detail::ToInternal(entity), content);
}

void SetTextWordWrap(ui::entity entity, policies::TextWrap mode)
{
    detail::text::SetTextWordWrap(CurrentRegistry(), detail::ToInternal(entity), mode);
}

void SetTextAlignment(ui::entity entity, policies::Alignment alignment)
{
    detail::text::SetTextAlignment(CurrentRegistry(), detail::ToInternal(entity), alignment);
}

void SetTextColor(ui::entity entity, const Color& color)
{
    detail::text::SetTextColor(CurrentRegistry(), detail::ToInternal(entity), color);
}

std::string GetTextEditContent(ui::entity entity)
{
    return detail::text::GetTextEditContent(CurrentRegistry(), detail::ToInternal(entity));
}

void SetTextEditContent(ui::entity entity, const std::string& content)
{
    detail::text::SetTextEditContent(CurrentRegistry(), detail::ToInternal(entity), content);
}

void SetPasswordMode(ui::entity entity, policies::TextFlag enabled)
{
    detail::text::SetPasswordMode(CurrentRegistry(), detail::ToInternal(entity), enabled);
}

void SetClickCallback(ui::entity entity, Callback<> callback)
{
    detail::text::SetClickCallback(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnSubmit(ui::entity entity, Callback<> callback)
{
    detail::text::SetOnSubmit(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnTextChanged(ui::entity entity, Callback<const std::string&> callback)
{
    detail::text::SetOnTextChanged(CurrentRegistry(), detail::ToInternal(entity), std::move(callback));
}

void SetLineHeight(ui::entity entity, float height)
{
    detail::text::SetLineHeight(CurrentRegistry(), detail::ToInternal(entity), height);
}

void SetCharacterSpacing(ui::entity entity, float spacing)
{
    detail::text::SetCharacterSpacing(CurrentRegistry(), detail::ToInternal(entity), spacing);
}

void SetTextWrapWidth(ui::entity entity, float width)
{
    detail::text::SetTextWrapWidth(CurrentRegistry(), detail::ToInternal(entity), width);
}

void SetFontSize(ui::entity entity, float size)
{
    detail::text::SetFontSize(CurrentRegistry(), detail::ToInternal(entity), size);
}

}  // namespace ui::text
