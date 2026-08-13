#include <cstddef>
#include <span>

#include <ui.hpp>

int main(int argc, char** argv)
{
    auto application = ui::factory::CreateApplication(std::span<char*>{argv, static_cast<std::size_t>(argc)});
    return application.has_value() ? 0 : 1;
}