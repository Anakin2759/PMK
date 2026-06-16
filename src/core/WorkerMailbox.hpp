/**
 * ************************************************************************
 *
 * @file WorkerMailbox.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-25
 * @version 0.1
 * @brief 多写一读 Worker Mailbox — C18 修复 + R2 零延迟 + pmr 零堆分配
 *
 * ## 设计原理
 *
 * Worker 线程只允许向此 Mailbox 投递命令（enqueue），
 * 主线程在每帧 QueuedTask 阶段统一排干（flush）并执行。
 * 采用双缓冲移动排干模式：
 *   - 多写端（Worker 线程）：在互斥锁下向写缓冲区追加命令
 *   - 单读端（主线程）  ：锁内移动到读缓冲区，锁外顺序执行
 *
 * ## 线程安全边界
 *
 * | 方法      | 调用方       | 说明               |
 * |-----------|--------------|--------------------|
 * | enqueue() | 任意线程     | 互斥锁保护写缓冲区 |
 * | flush()   | 仅主线程     | 执行阶段完全无锁   |
 *
 * ## 生命周期约束
 *
 * Worker 任务通过捕获 `WorkerMailbox*` 裸指针投递命令。
 * 调用方必须保证：持有此指针的所有 worker 任务在 UiRuntime 析构前完成。
 * 实践上通过 `m_threadPool.wait()` 在 UiRuntime 析构时同步（见 UiRuntime.hpp）。
 *
 * ## 单线程模式说明（UI_ENABLE_MULTITHREAD=0）
 *
 * submitDetached() 在单线程模式下同步执行 worker lambda；
 * worker 内的 enqueue() 通过 `if constexpr` 编译期分派，
 * 在调用栈内直接 apply 命令到 Registry，无需经过双缓冲，
 * 因此**单线程模式零帧延迟**，与多线程模式行为一致（R2 修复）。
 *
 * ## pmr 双缓冲（R2 内存优化）
 *
 * 每侧缓冲配一个 `monotonic_buffer_resource`（4KB 栈 + new_delete fallback）。
 * flush() 末尾释放本帧用过的 resource，栈缓冲复位、堆溢出回收，
 * 日常帧（≤64 条命令）零堆分配，历史峰值不驻留。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <memory_resource>
#include <mutex>
#include <variant>
#include <vector>

#include "common/ThreadPool.hpp"

// 前向声明，避免引入完整的 Registry 头（通过 RuntimeFacade 间接访问）
namespace ui
{
class Registry;
} // namespace ui

namespace ui::worker
{

/**
 * @brief 向主线程写回 UI Registry 操作的命令
 *
 * Worker 线程在 lambda 中捕获计算结果，lambda 由主线程在 flush() 阶段执行，
 * 可安全地对 UI Registry 进行 emplace / patch / destroy 等写操作。
 *
 * @note 内部严禁调用任何 Dispatcher API（Trigger / Enqueue），
 *       避免在帧时序之外触发事件，破坏 QueuedTask → InputTask → RenderTask 的执行顺序。
 */
struct RegistryCommand
{
    std::move_only_function<void(Registry&)> apply;
};

/// Worker 向主线程投递的命令（variant 仅保留 RegistryCommand）
using WorkerCommand = std::variant<RegistryCommand>;

} // namespace ui::worker

namespace ui
{

/**
 * @brief 多写一读 Worker Mailbox
 *
 * 每个 UiRuntime 实例持有一个 WorkerMailbox。Worker 线程通过捕获其指针投递命令；
 * 主线程在 TaskChain::QueuedTask 帧开始阶段调用 flush() 统一执行，彻底消除
 * Worker 直接访问 UI Registry 的竞争窗口（C18 修复）。
 */
class WorkerMailbox
{
public:
    WorkerMailbox()
                : m_writeResource(m_writeStack.data(), STACK_BUFFER_SIZE, std::pmr::new_delete_resource()),
          m_writeBuffer(&m_writeResource),
                    m_readResource(m_readStack.data(), STACK_BUFFER_SIZE, std::pmr::new_delete_resource()),
          m_readBuffer(&m_readResource)
    { }
    ~WorkerMailbox() = default;
    WorkerMailbox(const WorkerMailbox&) = delete;
    WorkerMailbox& operator=(const WorkerMailbox&) = delete;
    WorkerMailbox(WorkerMailbox&&) = delete;
    WorkerMailbox& operator=(WorkerMailbox&&) = delete;

    /**
     * @brief 从任意线程安全地投递一条命令（多写端）
     *
     * 单线程模式（UI_ENABLE_MULTITHREAD=0）下命令在调用栈内立即执行，
     * 消除多线程与单线程之间"命令延迟一帧"的行为差异（R2 修复）。
     *
     * @note 实现移至 WorkerMailbox.cpp 以使用 RuntimeFacade::registry() 替代弃用的 Registry::current()
     */
    void enqueue(worker::WorkerCommand cmd);

    /**
     * @brief 主线程专用：排干所有待处理命令（实现见 WorkerMailbox.cpp）
     *
     * 分两个阶段：
    *   1. 在锁内把写缓冲移动到读缓冲区，随即释放锁（Worker 可继续 push）
     *   2. 在锁外顺序执行读缓冲区中的所有命令；每条命令独立捕获异常，
     *      保证单条命令失败不影响后续命令的执行。
     *
     */
    void flush();

    /**
     * @brief 调试用：检查写缓冲区是否有待处理命令（仅 Debug 构建有效）
     *
     * @warning Release 构建永远返回 false，仅供开发期诊断，避免 TOCTOU 竞态误用。
     */
    [[nodiscard]] bool hasPending() const
    {
#ifndef NDEBUG
        std::lock_guard lock(m_mutex);
        return !m_writeBuffer.empty();
#else
        return false;
#endif
    }

private:
    static constexpr std::size_t STACK_BUFFER_SIZE = 4096; ///< 栈缓冲 4KB（≈60~85 条命令，日常帧零堆分配）

    mutable std::mutex m_mutex; ///< 保护写缓冲区

    // ── 写缓冲区（多线程，mutex 保护）──
    alignas(64) std::array<std::byte, STACK_BUFFER_SIZE> m_writeStack{};
    std::pmr::monotonic_buffer_resource m_writeResource;
    std::pmr::vector<worker::WorkerCommand> m_writeBuffer;

    // ── 读缓冲区（主线程独占）──
    alignas(64) std::array<std::byte, STACK_BUFFER_SIZE> m_readStack{};
    std::pmr::monotonic_buffer_resource m_readResource;
    std::pmr::vector<worker::WorkerCommand> m_readBuffer;
};

} // namespace ui
