#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "src/utils/EventLoop.hpp"
#include "src/utils/MpscQueue.hpp"

namespace ui::tests
{
namespace
{

using Clock = std::chrono::steady_clock;

// 观测型压测参数：仅用于复现/记录 lost-wakeup，不改变 EventLoop 的无锁同步结构。
// 单任务 ping 在正常情况下应在几十微秒内完成；超过阈值视为潜在丢失唤醒或调度异常。
inline constexpr int kPingIterations = 1000;
inline constexpr int kPingTimeoutMs = 200;

// 多线程突发排空：总任务数、生产者线程数。
inline constexpr int kBurstTaskCount = 100000;
inline constexpr int kBurstProducerThreads = 4;
inline constexpr int kBurstTasksPerThread = kBurstTaskCount / kBurstProducerThreads;

// 队列容量（独立常量，避免 magic number）。
inline constexpr std::size_t kPingQueueCapacity = 64;
inline constexpr std::size_t kBurstQueueCapacity = 4096;

/**
 * @brief 测试本地的完成观测门闩：仅用于限时等待任务执行，不触碰 EventLoop 同步结构。
 */
class CompletionGate
{
   public:
    void Signal()
    {
        {
            std::lock_guard lock(mutex_);
            done_ = true;
        }
        cv_.notify_one();
    }

    [[nodiscard]] bool WaitForMs(int timeoutMs)
    {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return done_; });
    }

   private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool done_{false};
};

struct BlockingMoveState
{
    std::mutex mutex;
    std::condition_variable cv;
    bool moveEntered{false};
    bool allowMove{false};
    bool blockNextMove{true};
};

class BlockingMoveTask
{
   public:
    BlockingMoveTask(std::shared_ptr<BlockingMoveState> state, std::atomic_bool& executed)
        : state_(std::move(state)), executed_(&executed)
    {
    }

    BlockingMoveTask(BlockingMoveTask&& other) noexcept : state_(std::move(other.state_)), executed_(other.executed_)
    {
        std::unique_lock lock(state_->mutex);
        if (state_->blockNextMove)
        {
            state_->blockNextMove = false;
            state_->moveEntered = true;
            state_->cv.notify_all();
            state_->cv.wait(lock, [this] { return state_->allowMove; });
        }
    }

    BlockingMoveTask(const BlockingMoveTask&) = delete;
    BlockingMoveTask& operator=(const BlockingMoveTask&) = delete;
    BlockingMoveTask& operator=(BlockingMoveTask&&) = delete;
    ~BlockingMoveTask() = default;

    void operator()() const
    {
        executed_->store(true, std::memory_order_release);
    }

   private:
    std::shared_ptr<BlockingMoveState> state_;
    std::atomic_bool* executed_;
};

TEST(MpscQueuePublicationTest, ConsumerCannotObserveSlotBeforeAccountingCommit)
{
    ui::utils::MpscQueue<int> queue{2};
    std::atomic_bool committed{false};

    ASSERT_TRUE(queue.TryEmplaceBeforePublish(
        [&committed]() noexcept { committed.store(true, std::memory_order_release); }, 42));
    auto value = queue.TryDequeue();

    ASSERT_TRUE(value.has_value());
    EXPECT_TRUE(committed.load(std::memory_order_acquire));
    EXPECT_EQ(*value, 42);
}

TEST(EventLoopStressTest, DrainWaitsForProducerAlreadyInsidePost)
{
    ui::utils::EventLoop loop{kPingQueueCapacity};
    std::thread consumer([&loop] { loop.Exec(); });
    auto state = std::make_shared<BlockingMoveState>();
    std::atomic_bool executed{false};
    bool posted = false;

    std::thread producer([&]
                         { posted = loop.Post(BlockingMoveTask{state, executed}); });
    {
        std::unique_lock lock(state->mutex);
        ASSERT_TRUE(state->cv.wait_for(lock, std::chrono::milliseconds(kPingTimeoutMs),
                                       [&state] { return state->moveEntered; }));
    }

    loop.Exit(0, true);
    EXPECT_TRUE(loop.IsRunning());

    {
        std::lock_guard lock(state->mutex);
        state->allowMove = true;
    }
    state->cv.notify_all();

    producer.join();
    consumer.join();
    EXPECT_TRUE(posted);
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
    EXPECT_EQ(loop.PendingCount(), 0U);
}

/**
 * @brief 单任务 ping 观测：循环投递单个任务并等待其执行。
 *
 * lost-wakeup 的竞态窗口位于 Post 的 pending_tasks_.fetch_add（未持 wait_mutex_）
 * 与消费者 WaitForWork 持锁检查 pending_tasks_==0 之间。本测试不做同步结构改动，
 * 仅通过"投递 → 限时等待执行"复现并记录该窗口。正常情况下每个 ping 应在微秒级完成。
 */
TEST(EventLoopStressTest, SingleTaskPingDoesNotLoseWakeup)
{
    ui::utils::EventLoop loop{kPingQueueCapacity};  // 小容量增加竞争压力，但不影响单任务 ping
    std::thread consumer([&loop] { loop.Exec(); });

    // 预热：确保事件循环已进入运行态
    {
        CompletionGate warmed;
        loop.PostOrThrow([&warmed] { warmed.Signal(); });
        ASSERT_TRUE(warmed.WaitForMs(kPingTimeoutMs)) << "预热任务在阈值内未执行，事件循环可能未启动";
    }

    int timeouts = 0;
    const auto start = Clock::now();

    for (int i = 0; i < kPingIterations; ++i)
    {
        CompletionGate gate;
        loop.PostOrThrow([&gate] { gate.Signal(); });

        if (!gate.WaitForMs(kPingTimeoutMs))
        {
            ++timeouts;
            // 记录首个复现点并提前终止，避免无谓等待
            GTEST_LOG_(INFO) << "lost-wakeup candidate detected at iteration " << i << " (task not executed within "
                             << kPingTimeoutMs << "ms)";
            break;
        }
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();

    loop.Exit(0, true);
    consumer.join();

    GTEST_LOG_(INFO) << "SingleTaskPing: iterations=" << kPingIterations << " timeouts=" << timeouts
                     << " elapsed_ms=" << elapsed;
    EXPECT_EQ(timeouts, 0) << "检测到 " << timeouts << " 次任务在 " << kPingTimeoutMs
                           << "ms 内未执行，疑似 lost-wakeup，需进一步定位";
}

/**
 * @brief 多线程突发排空观测：多生产者并发投递大量任务，验证全部被执行。
 *
 * 用于观测无锁 MPSC 队列在多生产者并发下的吞吐与完整性；配合 Exit(drain=true)
 * 验证优雅退出能排空剩余任务。
 */
TEST(EventLoopStressTest, MultiProducerBurstAllDrain)
{
    ui::utils::EventLoop loop{kBurstQueueCapacity};
    std::thread consumer([&loop] { loop.Exec(); });

    std::atomic<std::uint64_t> executed{0};

    std::vector<std::thread> producers;
    producers.reserve(kBurstProducerThreads);

    for (int t = 0; t < kBurstProducerThreads; ++t)
    {
        producers.emplace_back([&loop, &executed]
                               {
                                   for (int i = 0; i < kBurstTasksPerThread; ++i)
                                   {
                                       // 队列满时返回 false，这里以忙等重试确保所有任务最终入队
                                       while (!loop.Post([&executed]
                                                         { executed.fetch_add(1, std::memory_order_relaxed); }))
                                       {
                                           std::this_thread::yield();
                                       }
                                   }
                               });
    }

    for (auto& producer : producers)
    {
        producer.join();
    }

    // 请求排空退出：等待所有已入队任务执行完毕
    loop.Exit(0, true);
    consumer.join();

    const auto executedCount = executed.load(std::memory_order_acquire);
    GTEST_LOG_(INFO) << "MultiProducerBurst: expected=" << kBurstTaskCount << " executed=" << executedCount;
    EXPECT_EQ(executedCount, static_cast<std::uint64_t>(kBurstTaskCount))
        << "已执行任务数与投递总数不一致，存在丢任务或排空不完整";
}

}  // namespace
}  // namespace ui::tests
