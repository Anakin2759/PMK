/**
 * ************************************************************************
 *
 * @file Application.hpp
 * @brief UI 应用生命周期的稳定公共 PImpl 外壳。
 *
 * ************************************************************************
 */
#pragma once

#include <memory>
#include <span>

namespace ui
{
class ApplicationImpl;
class UiRuntime;

namespace events
{
struct QuitRequested;
}

class Application
{
   public:
    explicit Application(std::span<char*> arg);
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    ~Application() noexcept;

    void onQuitRequested([[maybe_unused]] ui::events::QuitRequested& event);
    void exec();

    [[nodiscard]] UiRuntime& runtime() noexcept;
    [[nodiscard]] const UiRuntime& runtime() const noexcept;

   private:
    std::unique_ptr<ApplicationImpl> m_impl;
};
}  // namespace ui
