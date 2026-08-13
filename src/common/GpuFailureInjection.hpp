/**
 * ************************************************************************
 *
 * @file GpuFailureInjection.hpp
 * @brief 内部线程局部 GPU 失败注入 gate
 *
 * ************************************************************************
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

namespace ui::detail
{

/**
 * @brief 内部 GPU 事务的可注入失败阶段
 * @note 仅供 src 内部测试 seam 使用，不属于公开配置或安装 API。
 */
enum class GpuFaultPoint : std::uint8_t
{
    DEVICE_CREATE,
    RESOURCE_CREATE,
    TRANSFER_CREATE,
    MAP,
    COMMAND_ACQUIRE,
    SWAPCHAIN_ACQUIRE,
    COPY_PASS_BEGIN,
    RENDER_PASS_BEGIN,
    SUBMIT
};

namespace gpu_failure_injection
{
struct Rule final
{
    GpuFaultPoint point;
    std::size_t failOnHit;
    bool failSubsequentHits;
    std::size_t hitCount = 0;
};

[[nodiscard]] inline Rule*& ActiveRule() noexcept
{
    static thread_local Rule* activeRule = nullptr;
    return activeRule;
}
} // namespace gpu_failure_injection

/**
 * @brief 在当前线程临时安装内部 GPU 失败规则
 *
 * 规则仅影响当前线程，析构时恢复上一条规则，支持嵌套作用域。未安装规则时失败注入
 * 默认关闭；该机制不读取外部配置，也不属于公开 API。
 */
class ScopedGpuFault final
{
public:
    /**
     * @brief 在线程局部作用域安装一条失败规则
     * @param point 需要观测的失败阶段
     * @param failOnHit 从 1 开始计数的触发次数；为 0 时不触发
     * @param failSubsequentHits 是否在首次触发后继续使同阶段失败
     * @note 默认没有活动规则，gate 恒为关闭；规则不读取环境变量、命令行或用户配置。
     */
    ScopedGpuFault(GpuFaultPoint point, std::size_t failOnHit, bool failSubsequentHits = false) noexcept
        : m_rule{point, failOnHit, failSubsequentHits},
          m_previous(std::exchange(gpu_failure_injection::ActiveRule(), &m_rule))
    {
    }

    ~ScopedGpuFault() { gpu_failure_injection::ActiveRule() = m_previous; }

    ScopedGpuFault(const ScopedGpuFault&) = delete;
    ScopedGpuFault& operator=(const ScopedGpuFault&) = delete;
    ScopedGpuFault(ScopedGpuFault&&) = delete;
    ScopedGpuFault& operator=(ScopedGpuFault&&) = delete;

    [[nodiscard]] std::size_t HitCount() const noexcept { return m_rule.hitCount; }

private:
    gpu_failure_injection::Rule m_rule;
    gpu_failure_injection::Rule* m_previous;
};

/**
 * @brief 检查当前线程的指定阶段是否应注入失败
 * @return 当前线程规则在本次命中触发时返回 true；未安装规则时返回 false
 */
[[nodiscard]] inline bool ShouldInjectGpuFailure(GpuFaultPoint point) noexcept
{
    auto* rule = gpu_failure_injection::ActiveRule();
    if (rule == nullptr || rule->point != point)
    {
        return false;
    }
    ++rule->hitCount;
    return rule->failOnHit != 0
        && (rule->hitCount == rule->failOnHit
            || (rule->failSubsequentHits && rule->hitCount > rule->failOnHit));
}

} // namespace ui::detail
