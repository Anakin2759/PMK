/**
 * @file Callback.hpp
 * @brief UI 公开 move-only 回调类型。
 */
#pragma once

#include <functional>

namespace ui
{

/**
 * @brief UI 事件和控件 API 使用的单所有者回调。
 * @tparam Args 回调参数类型。
 *
 * Callback 保持 move-only 语义，适合捕获不可复制资源，且不依赖 ECS、组件或数学类型。
 */
template <typename... Args>
using Callback = std::move_only_function<void(Args...)>;

} // namespace ui
