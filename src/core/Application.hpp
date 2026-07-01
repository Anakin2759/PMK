/**
 * ************************************************************************
 *
 * @file Application.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief ui上下文管理类
 *
 * 负责主循环、输入事件处理、图形上下文管理以及驱动所有ECS系统。
    存在一个单例指针，方便全局访问。
    不是根实体，也不管理根实体
    只负责驱动UiSystem和处理SDL集成
    不负责具体UI逻辑和组件管理
    提供初始化和清理接口
    提供运行主循环的接口
    提供访问根实体的接口
    提供访问图形上下文的接口
    一切修改UI状态的操作均通过ECS系统函数在eventloop中完成，

  - exec 方法启动主循环

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
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
    /**
     * @brief 构造函数：初始化所有外部和内部资源
     * @param title 窗口标题
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    explicit Application(std::span<char*> arg);
    // 阻止拷贝和移动（通常 Application 是独占资源）
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    ~Application() noexcept = default;
    void onQuitRequested([[maybe_unused]] ui::events::QuitRequested& event);

    /**
     * @brief 应用主循环
     */
    void exec();

    /**
     * @brief 获取应用持有的 UI runtime。
     *
     * 新的公开创建入口应显式绑定到该 runtime，避免依赖隐式 thread-local current runtime。
     */
    [[nodiscard]] UiRuntime& runtime() noexcept;

    [[nodiscard]] const UiRuntime& runtime() const noexcept;

private:
    std::unique_ptr<ApplicationImpl> m_impl;
};
} // namespace ui