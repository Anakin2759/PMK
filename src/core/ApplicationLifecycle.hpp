#pragma once

#include <functional>
#include <utility>

namespace ui::detail
{

/**
 * @brief 协调 Application 拥有的系统资源与 SDL 会话关闭顺序。
 *
 * 该类型必须在 SystemManager 之前构造。构造失败展开成员时，SystemManager
 * 会先销毁，然后本类型才释放 SDL 会话。
 */
class ApplicationLifecycle final
{
public:
    using Cleanup = std::move_only_function<void()>;

    ApplicationLifecycle() = default;
    ApplicationLifecycle(const ApplicationLifecycle&) = delete;
    ApplicationLifecycle& operator=(const ApplicationLifecycle&) = delete;
    ApplicationLifecycle(ApplicationLifecycle&&) = delete;
    ApplicationLifecycle& operator=(ApplicationLifecycle&&) = delete;

    ~ApplicationLifecycle() noexcept { Shutdown([] {}); }

    /**
     * @brief 在 SDL 初始化成功后接管对应的退出操作。
     */
    void ArmSdl(Cleanup quit) { m_quit = std::move(quit); }

    /**
     * @brief 幂等关闭：先销毁系统资源，再释放 SDL 会话。
     *
     * 即使系统销毁步骤报告异常，也会继续执行 SDL 退出操作。所有异常均在
     * noexcept 边界内吸收，调用方可在此前分别记录更细粒度的清理错误。
     */
    template <typename DestroySystems>
    void Shutdown(DestroySystems&& destroySystems) noexcept
    {
        if (m_shutdown)
        {
            return;
        }
        m_shutdown = true;

        try
        {
            std::forward<DestroySystems>(destroySystems)();
        }
        catch (...) // NOLINT(bugprone-empty-catch)
        {
            // noexcept 关闭边界：继续执行 SDL 退出，具体清理错误由调用方记录。
        }

        Cleanup quit = std::move(m_quit);
        m_quit = nullptr;
        if (quit)
        {
            try
            {
                quit();
            }
            catch (...) // NOLINT(bugprone-empty-catch)
            {
                // 析构期间不能传播 SDL 退出回调抛出的异常。
            }
        }
    }

private:
    Cleanup m_quit;
    bool m_shutdown = false;
};

} // namespace ui::detail
