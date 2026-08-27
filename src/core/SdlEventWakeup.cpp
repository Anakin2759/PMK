#include "SdlEventWakeup.hpp"

#include <stdexcept>
#include <string>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>

#include "EventLoop.hpp"

namespace
{
bool SDLCALL WakeEventLoop(void* userdata, [[maybe_unused]] SDL_Event* event) noexcept
{
    static_cast<ui::EventLoop*>(userdata)->notifyExternalEvent();
    return true;
}
}  // namespace

namespace ui::detail
{

SdlEventWakeup::SdlEventWakeup(EventLoop& eventLoop) : m_eventLoop(&eventLoop)
{
    if (!SDL_AddEventWatch(&WakeEventLoop, m_eventLoop))
    {
        m_eventLoop = nullptr;
        throw std::runtime_error(std::string("SDL_AddEventWatch failed: ") + SDL_GetError());
    }
}

SdlEventWakeup::~SdlEventWakeup() noexcept
{
    reset();
}

void SdlEventWakeup::reset() noexcept
{
    if (m_eventLoop == nullptr)
    {
        return;
    }

    SDL_RemoveEventWatch(&WakeEventLoop, m_eventLoop);
    m_eventLoop = nullptr;
}

}  // namespace ui::detail