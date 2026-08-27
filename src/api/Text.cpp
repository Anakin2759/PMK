#include "ui/api/Text.hpp"

#include <string>
#include <utility>

#include "helper/Helper.hpp"

namespace ui::text
{

void SetText(UiRuntime& runtime, ui::entity entity, const std::string& content)
{
    detail::text::SetText(runtime.registry(), detail::ToInternal(entity), content);
}

void SetButtonEnabled(UiRuntime& runtime, ui::entity entity, bool enabled)
{
    detail::text::SetButtonEnabled(runtime.registry(), detail::ToInternal(entity), enabled);
}

void SetTextContent(UiRuntime& runtime, ui::entity entity, const std::string& content)
{
    detail::text::SetTextContent(runtime.registry(), detail::ToInternal(entity), content);
}

void SetTextWordWrap(UiRuntime& runtime, ui::entity entity, policies::TextWrap mode)
{
    detail::text::SetTextWordWrap(runtime.registry(), detail::ToInternal(entity), mode);
}

void SetTextAlignment(UiRuntime& runtime, ui::entity entity, policies::Alignment alignment)
{
    detail::text::SetTextAlignment(runtime.registry(), detail::ToInternal(entity), alignment);
}

void SetTextColor(UiRuntime& runtime, ui::entity entity, const Color& color)
{
    detail::text::SetTextColor(runtime.registry(), detail::ToInternal(entity), color);
}

std::string GetTextEditContent(UiRuntime& runtime, ui::entity entity)
{
    return detail::text::GetTextEditContent(runtime.registry(), detail::ToInternal(entity));
}

void SetTextEditContent(UiRuntime& runtime, ui::entity entity, const std::string& content)
{
    detail::text::SetTextEditContent(runtime.registry(), detail::ToInternal(entity), content);
}

void SetPasswordMode(UiRuntime& runtime, ui::entity entity, policies::TextFlag enabled)
{
    detail::text::SetPasswordMode(runtime.registry(), detail::ToInternal(entity), enabled);
}

void SetClickCallback(UiRuntime& runtime, ui::entity entity, Callback<> callback)
{
    detail::text::SetClickCallback(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnSubmit(UiRuntime& runtime, ui::entity entity, Callback<> callback)
{
    detail::text::SetOnSubmit(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetOnTextChanged(UiRuntime& runtime, ui::entity entity, Callback<const std::string&> callback)
{
    detail::text::SetOnTextChanged(runtime.registry(), detail::ToInternal(entity), std::move(callback));
}

void SetLineHeight(UiRuntime& runtime, ui::entity entity, float height)
{
    detail::text::SetLineHeight(runtime.registry(), detail::ToInternal(entity), height);
}

void SetCharacterSpacing(UiRuntime& runtime, ui::entity entity, float spacing)
{
    detail::text::SetCharacterSpacing(runtime.registry(), detail::ToInternal(entity), spacing);
}

void SetTextWrapWidth(UiRuntime& runtime, ui::entity entity, float width)
{
    detail::text::SetTextWrapWidth(runtime.registry(), detail::ToInternal(entity), width);
}

void SetFontSize(UiRuntime& runtime, ui::entity entity, float size)
{
    detail::text::SetFontSize(runtime.registry(), detail::ToInternal(entity), size);
}

}  // namespace ui::text
