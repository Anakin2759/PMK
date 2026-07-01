#pragma once

#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>
namespace ui::utils {

/**
 * @brief 固定容量 Chase-Lev 工作窃取双端队列。
 *
 * 适用于工作窃取调度器中的“单拥有者 + 多窃取者”模型：
 * - 拥有者线程从底部 Push/Pop；
 * - 其他线程从顶部 Steal；
 * - Push 在队列已满时立即返回 false；
 * - Pop/Steal 在队列为空或竞争失败时返回 std::nullopt。
 *
 * 注意：该容器不是通用双端队列。只有拥有者线程可以调用 TryPush/TryEmplace/TryPop，
 * 多个窃取者线程可以并发调用 TrySteal。容量会向上取整为 2 的幂。
 */
template <typename T>
class WorkStealingDeque final {
 public:
  explicit WorkStealingDeque(std::size_t capacity)
      : kCapacity(NormalizeCapacity(capacity)),
        kMask(kCapacity - 1),
        buffer_(std::make_unique<Cell[]>(kCapacity)) {  // NOLINT
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);
  }

  WorkStealingDeque(const WorkStealingDeque&) = delete;
  WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;
  WorkStealingDeque(WorkStealingDeque&&) = delete;
  WorkStealingDeque& operator=(WorkStealingDeque&&) = delete;

  ~WorkStealingDeque() = default;

  /**
   * @brief 拥有者线程尝试从底部原地构造任务，队列已满时返回 false。
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  [[nodiscard]] bool TryEmplace(Args&&... args) {
    const auto kBottom = bottom_.load(std::memory_order_relaxed);
    const auto kTop = top_.load(std::memory_order_acquire);

    if (kBottom - kTop >= kCapacity) {
      return false;
    }

    auto item = std::make_shared<T>(std::forward<Args>(args)...);
    buffer_[kBottom & kMask].item.store(std::move(item), std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);  // 确保任务发布在 item 之后可见
    bottom_.store(kBottom + 1, std::memory_order_relaxed);
    return true;
  }

  /**
   * @brief 拥有者线程尝试从底部复制入队，队列已满时返回 false。
   */
  [[nodiscard]] bool TryPush(const T& value) { return TryEmplace(value); }

  /**
   * @brief 拥有者线程尝试从底部移动入队，队列已满时返回 false。
   */
  [[nodiscard]] bool TryPush(T&& value) { return TryEmplace(std::move(value)); }

  /**
   * @brief 拥有者线程尝试从底部弹出元素，队列为空或最后一个元素竞争失败时返回 std::nullopt。
   */
  [[nodiscard]] std::optional<T> TryPop() {
    auto bottom = bottom_.load(std::memory_order_relaxed);
    if (bottom == 0) {
      return std::nullopt;
    }

    --bottom;
    bottom_.store(bottom, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    auto top = top_.load(std::memory_order_relaxed);
    if (top > bottom) {
      bottom_.store(top, std::memory_order_relaxed);
      return std::nullopt;
    }

    auto item = buffer_[bottom & kMask].item.load(std::memory_order_acquire);
    if (top == bottom &&
        !top_.compare_exchange_strong(bottom, bottom + 1, std::memory_order_seq_cst,
                                      std::memory_order_relaxed)) {
      bottom_.store(bottom + 1, std::memory_order_relaxed);
      return std::nullopt;
    }

    if (top == bottom) {
      bottom_.store(bottom + 1, std::memory_order_relaxed);
    }
    if (item == nullptr) {
      return std::nullopt;
    }
    return std::optional<T>(std::move(*item));
  }

  /**
   * @brief 窃取者线程尝试从顶部窃取元素，队列为空或竞争失败时返回 std::nullopt。
   */
  [[nodiscard]] std::optional<T> TrySteal() {
    auto top = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    const auto kBottom = bottom_.load(std::memory_order_acquire);

    if (top >= kBottom) {
      return std::nullopt;
    }

    auto item = buffer_[top & kMask].item.load(std::memory_order_acquire);
    if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst,
                                      std::memory_order_relaxed)) {
      return std::nullopt;
    }

    if (item == nullptr) {
      return std::nullopt;
    }
    return std::optional<T>(std::move(*item));
  }

  /**
   * @brief 队列容量。
   */
  [[nodiscard]] std::size_t Capacity() const noexcept { return kCapacity; }

  /**
   * @brief 估算当前元素数量。并发场景下仅用于观测，不适合作为同步条件。
   */
  [[nodiscard]] std::size_t ApproximateSize() const noexcept {
    const auto kBottom = bottom_.load(std::memory_order_acquire);
    const auto kTop = top_.load(std::memory_order_acquire);
    return kBottom >= kTop ? kBottom - kTop : 0;
  }

  /**
   * @brief 估算队列是否为空。并发场景下仅用于观测。
   */
  [[nodiscard]] bool Empty() const noexcept { return ApproximateSize() == 0; }

 private:
  struct Cell {
    std::atomic<std::shared_ptr<T>> item{};
  };

  static std::size_t NormalizeCapacity(std::size_t capacity) {
    if (capacity == 0) {
      throw std::invalid_argument("WorkStealingDeque capacity must be greater than zero");
    }

    if (capacity > (std::size_t{1} << ((sizeof(std::size_t) * 8) - 1))) {
      throw std::invalid_argument("WorkStealingDeque capacity is too large");
    }

    return std::bit_ceil(capacity);  // 向上取整为 2 的幂
  }

  static constexpr std::size_t kCachelineSize = std::hardware_destructive_interference_size;
  const std::size_t kCapacity;
  const std::size_t kMask;
  std::unique_ptr<Cell[]> buffer_;  // NOLINT
  alignas(kCachelineSize) std::atomic<std::size_t> top_{0};
  alignas(kCachelineSize) std::atomic<std::size_t> bottom_{0};
};

}  // namespace myutils
