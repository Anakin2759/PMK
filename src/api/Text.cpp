#include "Text.hpp"

#include <string>
#include <utility>

#include "detail/EntityCast.hpp"
#include "detail/Text.hpp"

namespace ui::text
{

void SetText(ui::entity entity, const std::string& content)
{
    detail::text::SetText(detail::ToInternal(entity), content);
}

void SetButtonEnabled(ui::entity entity, bool enabled)
{
    detail::text::SetButtonEnabled(detail::ToInternal(entity), enabled);
}

void SetTextContent(ui::entity entity, const std::string& content)
{
    detail::text::SetTextContent(detail::ToInternal(entity), content);
}

void SetTextWordWrap(ui::entity entity, policies::TextWrap mode)
{
    detail::text::SetTextWordWrap(detail::ToInternal(entity), mode);
}

void SetTextAlignment(ui::entity entity, policies::Alignment alignment)
{
    detail::text::SetTextAlignment(detail::ToInternal(entity), alignment);
}

void SetTextColor(ui::entity entity, const Color& color)
{
    detail::text::SetTextColor(detail::ToInternal(entity), color);
}

std::string GetTextEditContent(ui::entity entity)
{
    return detail::text::GetTextEditContent(detail::ToInternal(entity));
}

void SetTextEditContent(ui::entity entity, const std::string& content)
{
    detail::text::SetTextEditContent(detail::ToInternal(entity), content);
}

void SetPasswordMode(ui::entity entity, policies::TextFlag enabled)
{
    detail::text::SetPasswordMode(detail::ToInternal(entity), enabled);
}

void SetClickCallback(ui::entity entity, components::on_event<> callback)
{
    detail::text::SetClickCallback(detail::ToInternal(entity), std::move(callback));
}

void SetOnSubmit(ui::entity entity, components::on_event<> callback)
{
    detail::text::SetOnSubmit(detail::ToInternal(entity), std::move(callback));
}

void SetOnTextChanged(ui::entity entity, components::on_event<const std::string&> callback)
{
    detail::text::SetOnTextChanged(detail::ToInternal(entity), std::move(callback));
}

void SetLineHeight(ui::entity entity, float height)
{
    detail::text::SetLineHeight(detail::ToInternal(entity), height);
}

void SetCharacterSpacing(ui::entity entity, float spacing)
{
    detail::text::SetCharacterSpacing(detail::ToInternal(entity), spacing);
}

void SetTextWrapWidth(ui::entity entity, float width)
{
    detail::text::SetTextWrapWidth(detail::ToInternal(entity), width);
}

void SetFontSize(ui::entity entity, float size)
{
    detail::text::SetFontSize(detail::ToInternal(entity), size);
}

} // namespace ui::text
