#include "Image.hpp"

#include <string>

#include "common/components/Data.hpp"
#include "core/UiRuntime.hpp"
#include "detail/EntityCast.hpp"

namespace ui::image
{
namespace
{
[[nodiscard]] Registry& CurrentRegistry()
{
    return UiRuntime::current().registry();
}
} // namespace

void SetImagePath(ui::entity entity, std::string_view path)
{
    auto& reg = CurrentRegistry();
    const auto internal = detail::ToInternal(entity);
    if (!reg.valid(internal)) return;

    auto& src = reg.get_or_emplace<components::ImageSource>(internal);
    src.path = std::string(path);
    src.loaded = false;
    src.loadFailed = false;
}

void SetImageTint(ui::entity entity, Color color)
{
    auto& reg = CurrentRegistry();
    const auto internal = detail::ToInternal(entity);
    if (!reg.valid(internal)) return;

    auto& img = reg.get_or_emplace<components::Image>(internal);
    img.tintColor = color;
}

void SetImageUV(ui::entity entity, Vec2 uvMin, Vec2 uvMax)
{
    auto& reg = CurrentRegistry();
    const auto internal = detail::ToInternal(entity);
    if (!reg.valid(internal)) return;

    auto& img = reg.get_or_emplace<components::Image>(internal);
    img.uvMin = uvMin;
    img.uvMax = uvMax;
}

} // namespace ui::image