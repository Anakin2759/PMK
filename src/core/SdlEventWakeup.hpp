#pragma once

namespace ui
{
class EventLoop;
}

namespace ui::detail
{

/**
 * @brief 将 SDL 事件到达通知桥接到 EventLoop 的内部 RAII 组件。
 *
 * 构造时安装 SDL event watch，析构或 reset() 时移除。watch 回调只负责
 * 唤醒 EventLoop，不处理 SDL 事件或访问 UI 运行时状态。
 */
class SdlEventWakeup final
{
   public:
    explicit SdlEventWakeup(EventLoop& eventLoop);
    SdlEventWakeup(const SdlEventWakeup&) = delete;
    SdlEventWakeup& operator=(const SdlEventWakeup&) = delete;
    SdlEventWakeup(SdlEventWakeup&&) = delete;
    SdlEventWakeup& operator=(SdlEventWakeup&&) = delete;
    ~SdlEventWakeup() noexcept;

    void reset() noexcept;

   private:
    EventLoop* m_eventLoop;
};

}  // namespace ui::detail