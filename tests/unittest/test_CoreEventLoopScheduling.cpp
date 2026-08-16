#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "src/core/EventLoop.hpp"

namespace ui::tests
{
namespace
{

using Clock = std::chrono::steady_clock;

/// 帧回调执行计数观测
class FrameCounter
{
   public:
    void Record()
    {
        ++count_;
    }

    [[nodiscard]] std::uint32_t Count() const noexcept
    {
        return count_.load(std::memory_order_acquire);
    }

   private:
    std::atomic<std::uint32_t> count_{0};
};

/**
 * @brief 固定帧率模式：默认 60fps 下帧回调以约 16ms 间隔执行。
 *
 * P1-4 双模式：FIXED_RATE（默认）保持历史行为，帧调度器按目标帧率节流投递默认处理器。
 */
TEST(CoreEventLoopSchedulingTest, FixedRateDeliversFramesOnInterval)
{
    ui::EventLoop loop;
    FrameCounter counter;

    loop.registerDefaultHandler([&counter] { counter.Record(); });
    loop.setTargetFrameRate(100);  // 100fps → 10ms 间隔，缩短测试时间

    std::thread loopThread([&loop] { loop.exec(); });

    // 等待约 60ms，应产生约 5~7 帧（10ms 间隔）
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    loop.quit();
    loopThread.join();

    const auto frames = counter.Count();
    GTEST_LOG_(INFO) << "FixedRate: target=100fps elapsed=60ms frames=" << frames;
    EXPECT_GE(frames, 3U) << "固定帧率模式下 60ms 内应至少产生 3 帧";
    EXPECT_LE(frames, 15U) << "固定帧率模式不应远超目标帧率（防止忙循环）";
}

/**
 * @brief 事件驱动模式：空闲时 CPU 归零，invoke 投递后立即唤醒执行。
 *
 * P1-4 双模式：EVENT_DRIVEN 下帧调度器空闲挂起，外部 invoke 唤醒调度器并投递默认处理器。
 */
TEST(CoreEventLoopSchedulingTest, EventDrivenWakesOnInvoke)
{
    ui::EventLoop loop;
    FrameCounter counter;

    loop.registerDefaultHandler([&counter] { counter.Record(); });
    loop.setFrameScheduleMode(ui::FrameScheduleMode::EVENT_DRIVEN);

    std::thread loopThread([&loop] { loop.exec(); });

    // 预热：确保事件循环已运行
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 空闲等待不应产生帧
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const auto idleFrames = counter.Count();

    // invoke 外部任务应唤醒调度器（随后投递默认处理器）
    loop.invoke([] {});
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    const auto afterInvokeFrames = counter.Count();
    GTEST_LOG_(INFO) << "EventDriven: idle=" << idleFrames << " afterInvoke=" << afterInvokeFrames;

    loop.quit();
    loopThread.join();

    // 空闲期应几乎无帧（允许少量预热帧）；invoke 后应出现新帧
    EXPECT_LE(idleFrames, 2U) << "事件驱动模式空闲期不应持续产生帧（CPU 应归零）";
    EXPECT_GT(afterInvokeFrames, idleFrames) << "invoke 应唤醒事件驱动调度器并产生新帧";
}

/**
 * @brief 锁帧接口：运行中切换帧率应改变帧间隔。
 */
TEST(CoreEventLoopSchedulingTest, TargetFrameRateCanBeChanged)
{
    ui::EventLoop loop;
    FrameCounter counter;

    loop.registerDefaultHandler([&counter] { counter.Record(); });

    // 初始默认值
    EXPECT_EQ(loop.targetFrameRate(), 60U);

    // 切换帧率
    loop.setTargetFrameRate(120);
    EXPECT_EQ(loop.targetFrameRate(), 120U);

    // 0 = 不锁帧
    loop.setTargetFrameRate(0);
    EXPECT_EQ(loop.targetFrameRate(), 0U);

    // 模式切换往返
    loop.setFrameScheduleMode(ui::FrameScheduleMode::EVENT_DRIVEN);
    EXPECT_EQ(loop.frameScheduleMode(), ui::FrameScheduleMode::EVENT_DRIVEN);
    loop.setFrameScheduleMode(ui::FrameScheduleMode::FIXED_RATE);
    EXPECT_EQ(loop.frameScheduleMode(), ui::FrameScheduleMode::FIXED_RATE);

    // 简短运行验证不崩溃
    std::thread loopThread([&loop] { loop.exec(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    loop.quit();
    loopThread.join();
}

}  // namespace
}  // namespace ui::tests
