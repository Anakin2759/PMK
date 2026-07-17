#pragma once

#include <cstddef>
#include <cstdint>

namespace ui::systems::render
{

/**
 * @brief GPU 管理器固定初始化链的纯逻辑事务状态。
 *
 * 仅记录已提交节点，不持有资源。失败回滚与正常关闭均按固定逆序访问节点，
 * 且每个节点最多访问一次。
 */
class GpuInitializationTransaction final
{
public:
    enum class Node : std::uint8_t
    {
        PIPELINE_CACHE,
        TEXT_TEXTURE_CACHE,
        COMMAND_BUFFER,
    };

    enum class State : std::uint8_t
    {
        IDLE,
        IN_PROGRESS,
        READY,
        FAILED,
        SHUTDOWN,
    };

    [[nodiscard]] bool Begin() noexcept
    {
        if (m_state != State::IDLE)
        {
            return false;
        }
        m_state = State::IN_PROGRESS;
        return true;
    }

    [[nodiscard]] bool Commit(Node node) noexcept
    {
        if (m_state != State::IN_PROGRESS || m_committedCount >= NODE_COUNT
            || expectedNode(m_committedCount) != node)
        {
            return false;
        }
        ++m_committedCount;
        return true;
    }

    [[nodiscard]] bool Complete() noexcept
    {
        if (m_state != State::IN_PROGRESS || m_committedCount != NODE_COUNT)
        {
            return false;
        }
        m_state = State::READY;
        return true;
    }

    template <typename CleanupVisitor>
    void FailAndRollback(CleanupVisitor cleanupVisitor) noexcept
    {
        if (m_state != State::IN_PROGRESS)
        {
            return;
        }
        rollback(cleanupVisitor);
        m_state = State::FAILED;
    }

    template <typename CleanupVisitor>
    void Shutdown(CleanupVisitor cleanupVisitor) noexcept
    {
        if (m_state == State::SHUTDOWN)
        {
            return;
        }
        rollback(cleanupVisitor);
        m_state = State::SHUTDOWN;
    }

    [[nodiscard]] State GetState() const noexcept { return m_state; }
    [[nodiscard]] bool IsReady() const noexcept { return m_state == State::READY; }
    [[nodiscard]] bool IsFailed() const noexcept { return m_state == State::FAILED; }
    [[nodiscard]] std::size_t CommittedCount() const noexcept { return m_committedCount; }
    [[nodiscard]] bool HadCleanupFailure() const noexcept { return m_cleanupFailed; }

private:
    [[nodiscard]] static constexpr Node expectedNode(std::size_t index) noexcept
    {
        switch (index)
        {
        case 0:
            return Node::PIPELINE_CACHE;
        case 1:
            return Node::TEXT_TEXTURE_CACHE;
        default:
            return Node::COMMAND_BUFFER;
        }
    }

    template <typename CleanupVisitor>
    void rollback(CleanupVisitor& cleanupVisitor) noexcept
    {
        while (m_committedCount > 0)
        {
            const Node node = expectedNode(--m_committedCount);
            try
            {
                cleanupVisitor(node);
            }
            catch (...)
            {
                m_cleanupFailed = true;
            }
        }
    }

    static constexpr std::size_t NODE_COUNT = 3;

    State m_state = State::IDLE;
    std::size_t m_committedCount = 0;
    bool m_cleanupFailed = false;
};

} // namespace ui::systems::render
