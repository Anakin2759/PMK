#include <ui/api/Factory.hpp>

#include <type_traits>

static_assert(std::is_same_v<decltype(&ui::factory::CreateApplication),
                             ui::Result<std::unique_ptr<ui::Application>> (*)(std::span<char*>)>);
static_assert(std::is_same_v<decltype(ui::EntityHandle::raw), ui::entity>);
static_assert(std::is_same_v<decltype(ui::WindowHandle::windowId), std::uint32_t>);
static_assert(std::is_same_v<decltype(ui::WindowHandle::token), std::uintptr_t>);

void DestroyApplicationResult(std::span<char*> arguments)
{
    auto result = ui::factory::CreateApplication(arguments);
}