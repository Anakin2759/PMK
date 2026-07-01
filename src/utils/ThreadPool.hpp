#pragma once

#include "MpmcQueue.hpp"
#include "WorkStealingDeque.hpp"

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
namespace myutils {

/**
 * @brief 基于 C++23 的工作窃取线程池。
 *
 * 特性：
 * - 使用 std::jthread 管理工作线程生命周期；
 * - 外部提交进入无锁 MPMC 全局队列，降低 Submit 的锁竞争；
 * - 工作线程优先执行本地 Chase-Lev 双端队列任务，并可从其他线程窃取任务；
 * - 使用独立可取任务计数减少空闲线程无效唤醒；
 * - Submit 返回 std::future，用于获取任务结果或异常；
 * - 支持 void 和非 void 返回类型；
 * - 支持 Wait() 等待当前已提交任务完成；
 * - 析构时默认优雅停止：不再接收新任务，已入队任务会执行完毕。
 */
class ThreadPool final {
 public:
  explicit ThreadPool(std::size_t thread_count = std::thread::hardware_concurrency(),
                      std::size_t queue_capacity = kDefaultQueueCapacity)
      : global_queue_(NormalizeQueueCapacity(queue_capacity)) {
    Start(thread_count == 0 ? 1 : thread_count);
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  ~ThreadPool() { Shutdown(); }

  /**
   * @brief 提交任务并返回 future。
   * @throws std::runtime_error 线程池已停止时抛出。
   */
  template <class Func, class... Args>
    requires std::invocable<std::decay_t<Func>, std::decay_t<Args>...>
  [[nodiscard]] auto Submit(Func&& func, Args&&... args)
      -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>> {
    using ReturnType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        [callable = std::forward<Func>(func),
         ... captured_args = std::forward<Args>(args)]() mutable -> ReturnType {
          if constexpr (std::is_void_v<ReturnType>) {
            std::invoke(std::move(callable), std::move(captured_args)...);
          } else {
            return std::invoke(std::move(callable), std::move(captured_args)...);
          }
        });

    auto future = task->get_future();

    pending_tasks_.fetch_add(1, std::memory_order_acq_rel);
    if (stopping_.load(std::memory_order_acquire)) {
      pending_tasks_.fetch_sub(1, std::memory_order_acq_rel);
      tasks_finished_.notify_all();
      throw std::runtime_error("ThreadPool has been stopped");
    }

    Task wrapper = [this, task = std::move(task)] {
      (*task)();
      FinishTask();
    };

    available_tasks_.fetch_add(1, std::memory_order_release);
    if (!EnqueueTask(std::move(wrapper))) {
      available_tasks_.fetch_sub(1, std::memory_order_acq_rel);  // 回退可取任务计数
      pending_tasks_.fetch_sub(1, std::memory_order_acq_rel);    // 回退待完成任务计数
      tasks_finished_.notify_all();
      throw std::runtime_error("ThreadPool task queue is full");
    }

    task_available_.notify_one();
    return future;
  }

  /**
   * @brief 等待已提交任务全部完成。
   *
   * 等待期间其他线程仍可继续 Submit；本函数会等待到队列为空且正在执行的任务数为 0。
   */
  void Wait() {
    std::unique_lock lock(mutex_);
    tasks_finished_.wait(lock,
                         [this] { return pending_tasks_.load(std::memory_order_acquire) == 0; });
  }

  /**
   * @brief 停止线程池，执行完已入队任务后退出所有工作线程。
   */
  void Shutdown() {
    const bool kAlreadyStopping = stopping_.exchange(true, std::memory_order_acq_rel);
    if (kAlreadyStopping) {
      return;
    }

    task_available_.notify_all();
    workers_.clear();
  }

  /**
   * @brief 当前工作线程数量。
   */
  [[nodiscard]] std::size_t ThreadCount() const noexcept { return workers_.size(); }

  /**
   * @brief 当前尚未完成的任务数量，包括排队和正在执行的任务。
   */
  [[nodiscard]] std::size_t PendingCount() const {
    return pending_tasks_.load(std::memory_order_acquire);
  }

 private:
  using Task = std::move_only_function<void()>;
  using LocalQueue = WorkStealingDeque<Task>;

  struct WorkerState {
    explicit WorkerState(std::size_t capacity) : local_queue(capacity) {}

    WorkerState(const WorkerState&) = delete;
    WorkerState& operator=(const WorkerState&) = delete;
    WorkerState(WorkerState&&) = delete;
    WorkerState& operator=(WorkerState&&) = delete;
    ~WorkerState() = default;

    LocalQueue local_queue;
  };

  void Start(std::size_t thread_count) {
    worker_states_.reserve(thread_count);
    for (std::size_t index = 0; index < thread_count; ++index) {
      worker_states_.push_back(std::make_unique<WorkerState>(global_queue_.Capacity()));
    }

    workers_.reserve(thread_count);
    for (std::size_t index = 0; index < thread_count; ++index) {
      workers_.emplace_back([this, index](const std::stop_token& stop_token) {
        CurrentWorkerIndex() = index;
        WorkerLoop(stop_token, index);
      });
    }
  }

  void WorkerLoop(const std::stop_token& stop_token, std::size_t worker_index) {
    while (true) {
      auto task = TakeTask(stop_token, worker_index);
      if (!task.has_value()) {
        return;
      }

      (*task)();
    }
  }

  [[nodiscard]] std::optional<Task> TakeTask(const std::stop_token& stop_token,
                                             std::size_t worker_index) {
    while (true) {
      if (auto task = worker_states_[worker_index]->local_queue.TryPop(); task.has_value()) {
        MarkTaskTaken();
        return task;
      }

      if (auto task = global_queue_.TryDequeue(); task.has_value()) {
        MarkTaskTaken();
        return task;
      }

      if (auto task = StealTask(worker_index); task.has_value()) {
        MarkTaskTaken();
        return task;
      }

      if (stopping_.load(std::memory_order_acquire) &&
          pending_tasks_.load(std::memory_order_acquire) == 0) {
        return std::nullopt;
      }

      std::unique_lock lock(mutex_);
      task_available_.wait(lock, stop_token, [this] {
        return stopping_.load(std::memory_order_acquire) ||
               available_tasks_.load(std::memory_order_acquire) > 0;
      });
    }
  }

  [[nodiscard]] std::optional<Task> StealTask(std::size_t worker_index) {
    const std::size_t kWorkerCount = worker_states_.size();
    for (std::size_t offset = 1; offset < kWorkerCount; ++offset) {
      const std::size_t kVictimIndex = (worker_index + offset) % kWorkerCount;
      if (auto task = worker_states_[kVictimIndex]->local_queue.TrySteal(); task.has_value()) {
        return task;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] bool EnqueueTask(Task&& task) {
    if (CurrentWorkerIndex() != kNoWorkerIndex) {
      auto& local_queue = worker_states_[CurrentWorkerIndex()]->local_queue;
      if (local_queue.TryPush(std::move(task))) {
        return true;
      }
    }

    return global_queue_.TryEnqueue(std::move(task));
  }

  void MarkTaskTaken() noexcept { available_tasks_.fetch_sub(1, std::memory_order_acq_rel); }

  void FinishTask() {
    const auto kPreviousPending = pending_tasks_.fetch_sub(1, std::memory_order_acq_rel);
    if (kPreviousPending == 1) {
      tasks_finished_.notify_all();
      task_available_.notify_all();
    }
  }

  [[nodiscard]] static std::size_t NormalizeQueueCapacity(std::size_t queue_capacity) {
    if (queue_capacity == 0) {
      throw std::invalid_argument("ThreadPool queue capacity must be greater than zero");
    }
    return queue_capacity;
  }

  [[nodiscard]] static std::size_t& CurrentWorkerIndex() noexcept {
    thread_local std::size_t worker_index = kNoWorkerIndex;
    return worker_index;
  }

  static constexpr std::size_t kDefaultQueueCapacity = 4096;
  static constexpr std::size_t kNoWorkerIndex = static_cast<std::size_t>(-1);

  mutable std::mutex mutex_;
  std::condition_variable_any task_available_;
  std::condition_variable tasks_finished_;
  MpmcQueue<Task> global_queue_;
  std::vector<std::unique_ptr<WorkerState>> worker_states_;
  std::vector<std::jthread> workers_;
  alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> available_tasks_{
      0};  // 可取任务计数
  alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> pending_tasks_{
      0};  // 待完成任务计数
  alignas(std::hardware_destructive_interference_size) std::atomic_bool stopping_{false};
};

}  // namespace myutils
