/**
 * ************************************************************************
 *
 * @file WorkerMailbox.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-05-26
 * @version 0.1
 * @brief WorkerMailbox::flush() 实现（从 .hpp 拆出，减少编译时间膨胀）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "WorkerMailbox.hpp"

#include <exception>

#include "RuntimeFacade.hpp"
#include "utils/Logger.hpp"
#include "utils/Registry.hpp"

namespace ui
{

void WorkerMailbox::enqueue(worker::WorkerCommand cmd)
{
    if constexpr (!utils::ThreadPool::isMultithreaded())
    {
        // 单线程模式：命令在调用栈直接执行，绕过双缓冲
        // 通过 RuntimeFacade 而非弃用的 Registry::current() 访问
        std::get<worker::RegistryCommand>(cmd).apply(RuntimeFacade::current().registry());
        return;
    }
    std::lock_guard lock(m_mutex);
    m_writeBuffer.push_back(std::move(cmd));
}

void WorkerMailbox::flush()
{
    // Phase 1: 移动排干。不能 swap 两个使用不同 memory_resource 的 pmr::vector，
    // MSVC Debug STL 会因 allocator 不兼容触发 containers incompatible for swap 断言。
    {
        std::lock_guard lock(m_mutex);
        if (m_writeBuffer.empty())
        {
            return;
        }

        m_readBuffer.reserve(m_writeBuffer.size());
        for (auto& cmd : m_writeBuffer)
        {
            m_readBuffer.push_back(std::move(cmd));
        }

        m_writeBuffer = std::pmr::vector<worker::WorkerCommand>(&m_writeResource);
        m_writeResource.release();
    }

    // Phase 2: 无锁执行，m_readBuffer 由主线程独占
    auto& registry = RuntimeFacade::current().registry();
    for (auto& cmd : m_readBuffer)
    {
        try
        {
            std::get<worker::RegistryCommand>(cmd).apply(registry);
        }
        catch (const std::exception& ex)
        {
            Logger::error("[WorkerMailbox] Command threw: {}", ex.what());
        }
        catch (...)
        {
            Logger::error("[WorkerMailbox] Command threw unknown exception.");
        }
    }

    // Phase 3: 释放读端刚消费完的 resource。栈缓冲复位，堆溢出回收，Worker 端不受影响。
    m_readBuffer.clear();
    m_readBuffer = std::pmr::vector<worker::WorkerCommand>(&m_readResource);
    m_readResource.release();
}

} // namespace ui
