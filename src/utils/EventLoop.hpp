#pragma once

#include "MpscQueue.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace ui::utils {

/**
 * @brief 基于有界 MPSC 环形队列的高性能事件循环。
 *
 * 特性：
 * - 多线程可并发 Post 任务；
 * - 单个 Exec 线程独占消费任务，避免消费者侧 CAS 开销；
 * - 使用无锁 MPSC 队列承载热路径任务投递；
 * - 空闲时通过 condition_variable 休眠，投递任务或退出时唤醒；
 * - 支持优雅退出和立即退出；
 * - 支持从事件循环线程内 Dispatch 直接执行任务。
 *
 * 注意：同一个 EventLoop 实例同一时刻只能有一个线程调用 Exec/exec。
 */
class EventLoop final {
 public:
	using Task = std::move_only_function<void()>;
	using ExceptionHandler = std::function<void(std::exception_ptr)>;

	explicit EventLoop(std::size_t queue_capacity = kDefaultQueueCapacity)
			: queue_(queue_capacity) {}

	EventLoop(const EventLoop&) = delete;
	EventLoop& operator=(const EventLoop&) = delete;
	EventLoop(EventLoop&&) = delete;
	EventLoop& operator=(EventLoop&&) = delete;

	~EventLoop() { Exit(); }

	/**
	 * @brief 开始事件循环，直到 Exit/Quit 被调用。
	 *
	 * @return 退出码，由 Exit/Quit 设置。
	 * @throws std::logic_error 同一事件循环被重复 Exec 时抛出。
	 */
	int Exec() {
		bool expected = false;
		if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel,
																					std::memory_order_acquire)) {
			throw std::logic_error("EventLoop is already running");
		}

		loop_thread_id_ = std::this_thread::get_id();
		accepting_.store(true, std::memory_order_release);

		try {
			RunLoop();
		} catch (...) {
			loop_thread_id_ = {};
			accepting_.store(false, std::memory_order_release);
			running_.store(false, std::memory_order_release);
			WakeUpAll();
			throw;
		}

		loop_thread_id_ = {};
		accepting_.store(false, std::memory_order_release);
		running_.store(false, std::memory_order_release);
		WakeUpAll();
		return exit_code_.load(std::memory_order_acquire);
	}

	/**
	 * @brief 兼容常见事件循环命名风格的 Exec 别名。
	 */
	int exec() { return Exec(); }  // NOLINT(readability-identifier-naming)

	/**
	 * @brief 从任意线程投递任务。队列已满或事件循环正在立即退出时返回 false。
	 */
	template <class Func>
		requires std::invocable<std::decay_t<Func>&>
	[[nodiscard]] bool Post(Func&& func) {
		if (!CanAcceptNewTask()) {
			return false;
		}

		Task task(std::forward<Func>(func));
		if (!queue_.TryEnqueue(std::move(task))) {
			return false;
		}

		pending_tasks_.fetch_add(1, std::memory_order_release);
		WakeUpOne();
		return true;
	}

	/**
	 * @brief 投递任务，失败时抛出 std::runtime_error。
	 */
	template <class Func>
		requires std::invocable<std::decay_t<Func>&>
	void PostOrThrow(Func&& func) {
		if (!Post(std::forward<Func>(func))) {
			throw std::runtime_error("EventLoop task queue is full or loop is stopping");
		}
	}

	/**
	 * @brief 若当前线程就是事件循环线程则直接执行，否则投递到事件循环。
	 */
	template <class Func>
		requires std::invocable<std::decay_t<Func>&>
	[[nodiscard]] bool Dispatch(Func&& func) {
		if (IsInLoopThread()) {
			std::invoke(std::forward<Func>(func));
			return true;
		}

		return Post(std::forward<Func>(func));
	}

	/**
	 * @brief 请求退出事件循环。
	 *
	 * @param exit_code Exec 的返回值。
	 * @param drain_remaining true 表示执行完已入队任务后退出；false 表示尽快退出。
	 */
	void Exit(int exit_code = 0, bool drain_remaining = true) noexcept {
		exit_code_.store(exit_code, std::memory_order_release);
		drain_on_exit_.store(drain_remaining, std::memory_order_release);
		accepting_.store(drain_remaining, std::memory_order_release);
		exit_requested_.store(true, std::memory_order_release);
		WakeUpAll();
	}

	/**
	 * @brief Exit 的语义别名。
	 */
	void Quit(int exit_code = 0, bool drain_remaining = true) noexcept { Exit(exit_code, drain_remaining); }

	/**
	 * @brief 允许退出后的事件循环再次 Exec。
	 *
	 * 只能在事件循环未运行时调用。未处理任务会保留在队列中。
	 */
	void Reset() {
		if (running_.load(std::memory_order_acquire)) {
			throw std::logic_error("Cannot reset a running EventLoop");
		}

		exit_code_.store(0, std::memory_order_relaxed);
		drain_on_exit_.store(true, std::memory_order_relaxed);
		exit_requested_.store(false, std::memory_order_release);
		accepting_.store(true, std::memory_order_release);
	}

	/**
	 * @brief 设置任务异常处理器。未设置时任务异常会终止 Exec 并向外抛出。
	 */
	void SetExceptionHandler(ExceptionHandler handler) {
		std::scoped_lock lock(handler_mutex_);
		exception_handler_ = std::move(handler);
	}

	/**
	 * @brief 当前线程是否为事件循环线程。
	 */
	[[nodiscard]] bool IsInLoopThread() const noexcept {
		return running_.load(std::memory_order_acquire) && loop_thread_id_ == std::this_thread::get_id();
	}

	/**
	 * @brief 事件循环是否正在运行。
	 */
	[[nodiscard]] bool IsRunning() const noexcept { return running_.load(std::memory_order_acquire); }

	/**
	 * @brief 是否已经请求退出。
	 */
	[[nodiscard]] bool IsExitRequested() const noexcept {
		return exit_requested_.load(std::memory_order_acquire);
	}

	/**
	 * @brief 队列容量。
	 */
	[[nodiscard]] std::size_t Capacity() const noexcept { return queue_.Capacity(); }

	/**
	 * @brief 估算当前尚未执行的任务数量。
	 */
	[[nodiscard]] std::size_t PendingCount() const noexcept {
		return pending_tasks_.load(std::memory_order_acquire);
	}

 private:
	void RunLoop() {
		while (true) {
			DrainReadyTasks();

			if (ShouldStop()) {
				return;
			}

			WaitForWork();
		}
	}

	void DrainReadyTasks() {
		while (true) {
			auto task = queue_.TryDequeue();
			if (!task.has_value()) {
				return;
			}

			pending_tasks_.fetch_sub(1, std::memory_order_acq_rel);
			ExecuteTask(std::move(*task));

			if (ShouldStop()) {
				return;
			}
		}
	}

	void ExecuteTask(Task task) {
		try {
			task();
		} catch (...) {
			HandleException(std::current_exception());
		}
	}

	void HandleException(const std::exception_ptr& exception) {
		ExceptionHandler handler;
		{
			std::scoped_lock lock(handler_mutex_);
			handler = exception_handler_;
		}

		if (handler) {
			handler(exception);
			return;
		}

		std::rethrow_exception(exception);
	}

	[[nodiscard]] bool ShouldStop() const noexcept {
		if (!exit_requested_.load(std::memory_order_acquire)) {
			return false;
		}

		if (!drain_on_exit_.load(std::memory_order_acquire)) {
			return true;
		}

		return pending_tasks_.load(std::memory_order_acquire) == 0;
	}

	[[nodiscard]] bool CanAcceptNewTask() const noexcept {
		if (!accepting_.load(std::memory_order_acquire)) {
			return false;
		}

		return !exit_requested_.load(std::memory_order_acquire) ||
					 drain_on_exit_.load(std::memory_order_acquire);
	}

	void WaitForWork() {
		std::unique_lock lock(wait_mutex_);
		work_available_.wait(lock, [this] {
			return exit_requested_.load(std::memory_order_acquire) ||
						 pending_tasks_.load(std::memory_order_acquire) > 0;
		});
	}

	void WakeUpOne() noexcept { work_available_.notify_one(); }

	void WakeUpAll() noexcept { work_available_.notify_all(); }

	static constexpr std::size_t kDefaultQueueCapacity = 4096;

	MpscQueue<Task> queue_;
	mutable std::mutex wait_mutex_;
	std::condition_variable work_available_;
	mutable std::mutex handler_mutex_;
	ExceptionHandler exception_handler_;
	std::thread::id loop_thread_id_{};
	alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> pending_tasks_{0};
	alignas(std::hardware_destructive_interference_size) std::atomic_bool running_{false};
	alignas(std::hardware_destructive_interference_size) std::atomic_bool accepting_{true};
	alignas(std::hardware_destructive_interference_size) std::atomic_bool exit_requested_{false};
	alignas(std::hardware_destructive_interference_size) std::atomic_bool drain_on_exit_{true};
	alignas(std::hardware_destructive_interference_size) std::atomic_int exit_code_{0};
};

}  // namespace ui::utils
