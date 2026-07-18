#include <ui/api/Utils.hpp>

#include <type_traits>

static_assert(std::is_same_v<ui::utils::TaskHandle, std::uint32_t>);
static_assert(std::is_same_v<decltype(&ui::utils::InvokeTask),
                             void (*)(ui::UiRuntime&, std::function<void()>)>);
static_assert(std::is_same_v<decltype(&ui::utils::TimerCallback),
                             ui::utils::TaskHandle (*)(ui::UiRuntime&, std::uint32_t, std::function<void()>)>);