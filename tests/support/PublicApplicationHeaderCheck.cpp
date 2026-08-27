#include <ui/Application.hpp>

#include <type_traits>

static_assert(!std::is_copy_constructible_v<ui::Application>);
static_assert(!std::is_move_constructible_v<ui::Application>);
