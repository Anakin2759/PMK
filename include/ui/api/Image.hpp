/**
 * @file Image.hpp
 * @brief Image 组件的路径、着色和 UV 快捷操作 API。
 */
#pragma once

#include <string_view>

#include "ui/Color.hpp"
#include "ui/MathTypes.hpp"
#include "ui/api/Chains.hpp"
#include "ui/api/Entity.hpp"

namespace ui
{
class UiRuntime;
}

namespace ui::image
{
void SetImagePath(UiRuntime& runtime, ui::entity entity, std::string_view path);
void SetImageTint(UiRuntime& runtime, ui::entity entity, Color color);
void SetImageUV(UiRuntime& runtime, ui::entity entity, Vec2 uvMin, Vec2 uvMax);
}  // namespace ui::image

namespace ui::actions::image
{
inline constexpr EntityAction<&ui::image::SetImagePath> SET_IMAGE_PATH_ACTION{};
inline constexpr EntityAction<&ui::image::SetImageTint> SET_IMAGE_TINT_ACTION{};
inline constexpr EntityAction<&ui::image::SetImageUV> SET_IMAGE_UV_ACTION{};
}  // namespace ui::actions::image

namespace ui::chains
{
inline auto ImagePath(std::string_view path)
{
    return ui::actions::image::SET_IMAGE_PATH_ACTION.bind(path);
}

inline auto ImageTint(Color color)
{
    return ui::actions::image::SET_IMAGE_TINT_ACTION.bind(color);
}

inline auto ImageUV(Vec2 uvMin, Vec2 uvMax)
{
    return ui::actions::image::SET_IMAGE_UV_ACTION.bind(uvMin, uvMax);
}
}  // namespace ui::chains