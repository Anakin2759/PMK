# EventLoop lost-wakeup 同步方案评审

> 日期：2026-08-14
> 前置：`THREADPOOL_DECISION_2026-08-09.md` §6.1「无实测不改无锁路径」门槛已满足（P0-4 可重复复现）
> 证据：`docs/test-reports/run-20260814-eventloop-lost-wakeup.md`
> 性质：方案评审文档，**本阶段不改动任何同步结构**

---

## 1. 问题定义与复现证据

### 现象

`ui::utils::EventLoop`（`src/utils/EventLoop.hpp`）单任务 ping 在 200ms 超时阈值下可重复复现任务未被唤醒：

| 运行 | 迭代 | 结果 |
|---|---|---|
| 首次 | 第 5 次 | 超时 1 次 |
| 重复 | 第 36 次 | 超时 1 次 |

多生产者突发（4×25000 任务、队列容量 4096、`Exit(drain=true)`）全部执行完成，未发现丢任务。

### 影响

- 当前帧调度器（`src/core/EventLoop.hpp`）以 16ms 节流兜底轮询，lost-wakeup 被掩盖。
- 一旦 P1-4 改为事件驱动唤醒，丢失唤醒会直接导致任务延迟或挂起。
- 因此 P1-4 必须在本方案评审完成并落地后才能实施。

---

## 2. 根因分析

### 当前同步结构

```
Post():                              WaitForWork()（消费者线程）:
  accepting_.load()                    std::unique_lock lock(wait_mutex_);
  queue_.TryEnqueue(task)  // 无锁 CAS  work_available_.wait(lock, [this] {
  pending_tasks_.fetch_add(1, release)     return exit_requested_ ||
  WakeUpOne() = notify_one()                pending_tasks_ > 0;
                                              });
```

`notify_one()` **不持有 `wait_mutex_`**。

### 精确竞态窗口

C++ 标准 `condition_variable::wait(lock, pred)` 等价于：

```cpp
while (!pred()) {
    wait(lock);   // 原子地：释放 lock + 注册睡眠
}
```

谓词检查（持锁）与「释放锁 + 进入睡眠」之间存在顺序依赖，但 `notify` 若**不持锁**，可插入该窗口：

1. 消费者持锁检查谓词：`pending_tasks_ == 0` → 决定睡眠。
2. 生产者（无锁路径）：`TryEnqueue` 成功 → `pending_tasks_.fetch_add(1)` → `notify_one()`。
3. 此刻消费者尚未注册到等待队列 → **通知丢失**（`notify` 无等待者时直接丢弃）。
4. 消费者随后「释放锁 + 睡眠」，谓词不再重查 → **永久等待**。

这是经典「无锁 notify + 谓词竞态」lost-wakeup，与 200ms 超时观测完全吻合。

### 为什么多生产者突发不丢

高吞吐场景下队列几乎始终非空，消费者极少真正进入 `wait`，因此竞态窗口几乎不会被命中；只有「单任务、队列空、精确时序」才暴露。

---

## 3. 方案比较

| # | 方案 | 热路径成本 | 无锁语义 | 正确性 | 风险 |
|---|---|---|---|---|---|
| A | **持锁 notify**：`Post` 在 `wait_mutex_` 保护下 `fetch_add` + `notify_one` | 每次入队加锁/解锁 | ❌ 违背「无锁提交/通知」承诺（历史反复点） | ✅ 标准 CV 规则保证 | 高（回归 2026-08-10 已还原方案） |
| B | **C++20 `atomic_wait/notify`**：消费者在计数原子（或专门唤醒原子）上 `wait(0)`；生产者 `fetch_add` 后 `notify_one()` | 无锁；`notify` 一次系统调用 | ✅ 队列仍无锁 | ✅ **`atomic::wait` 无「检查后睡眠」窗口**：`wait(expect)` 是「load==expect 才睡」，`notify` 修改值后任何 wait 都会看到新值立即返回 | 中（平台实现差异；Windows `WaitOnAddress` 成熟） |
| C | **Event Count / 计数信号量**（自研） | 无锁 | ✅ | ✅ 理论最优 | 高（新同步原语，需要大量测试） |
| D | **保持现状 + 观测** | 0 | ✅ | ❌ 已知缺陷 | 阻塞 P1-4 |

### 方案 B 正确性论证

```cpp
// 消费者
void WaitForWork() {
    pending_tasks_.wait(0);   // 若 load()!=0 立即返回；否则阻塞
    // 唤醒后 DrainReadyTasks 重查队列
}

// 生产者
pending_tasks_.fetch_add(1, release);
pending_tasks_.notify_one();
```

`std::atomic::wait(expect)` 语义是 `while (load() == expect) 阻塞`。时序分析：

- 生产者先 `fetch_add`：消费者 `load` 看到新值 ≠ 0 → 不睡。✅
- 消费者先 `wait(0)` 进入阻塞：生产者 `notify_one` 唤醒。✅
- **不存在「检查后睡眠」丢失窗口**——`wait` 内部对值的读取与阻塞注册是原子的。

退出路径类似：`exit_requested_` 用单独原子 + `notify_all`，或在 `pending_tasks_` 上配合。

### 平台注意（Windows）

- clang-cl + MSVC STL：`atomic::wait/notify` 在 VS 2019 16.10+ 落地，底层 `WaitOnAddress`/`WakeByAddressSingle`。本项目 clang-cl + 现代 Windows SDK 可用。
- Linux：`futex`，成熟。
- 需用真实压力测试在两种后端（D3D12/Vulkan 均走同一 EventLoop）验证。

---

## 4. 推荐方案

**推荐 B：C++20 `std::atomic<T>::wait/notify`**，理由：

1. 保留无锁入队/通知路径（不违反 2026-08-10 无锁承诺）。
2. `atomic::wait` 从语义上消除谓词竞态，是 C++20 针对该问题的标准答案。
3. 改动局部（`WaitForWork`/`WakeUpOne`/`WakeUpAll`/`Exit`），不动 `MpscQueue`。

**备用 A（持锁 notify）** 仅当平台 `atomic::wait` 实测不可用或性能不达标时再考虑；该方案需回归 2026-08-10 用户否决点，须再次获得用户确认。

### 实施边界

- **不动**：`MpscQueue`、`Post` 的入队 CAS、`pending_tasks_` 计数语义。
- **改动**：`WaitForWork` 的等待原语、`WakeUpOne/WakeUpAll`、`Exit` 的退出通知。
- 涉及文件：`src/utils/EventLoop.hpp`（+ 必要测试 `tests/unittest/test_EventLoopStress.cpp`）。

---

## 5. 验收标准

1. `test_EventLoopStress` 单任务 ping 在 200ms 阈值下连续 20 轮无超时。
2. 多生产者突发 100000 任务 drain 完整。
3. 新增测量（对比基线）：
   - 吞吐（任务/秒）
   - P95 入队→执行延迟
   - 空闲 CPU（等待时无忙等）
   - 无锁语义：`Post` 热路径无 mutex 锁/unlock 指令
4. 全量构建 + 架构门禁 + 公共头门禁通过。

---

## 6. 与 P1-4 的关系

P1-4（帧循环事件驱动化）依赖本方案落地：

- 本方案完成后，`utils::EventLoop` 的唤醒是事件驱动的。
- P1-4 才能把 16ms 兜底轮询替换为纯事件驱动，且不暴露 lost-wakeup。
- 顺序不可反：先修唤醒，再改调度。

---

## 7. 决策点（待用户确认）

- [x] 是否批准方案 B（`atomic_wait/notify`）作为修复方向？ → **✅ 批准（2026-08-16 用户拍板）**
- [x] 若平台实测不可用，是否允许回退到方案 A（持锁 notify，需再次确认无锁承诺豁免）？ → **✅ 允许**
- [x] 是否授权实施 + 更新压测（基准测量在实施前完成）？ → **✅ 授权**

## 8. 实施记录（2026-08-16）

状态：**✅ 已完成并验证**。

- 实施内容：`src/utils/EventLoop.hpp` 引入 `wake_epoch_`（`std::atomic<std::size_t>` 唤醒事件计数），
  `WaitForWork` 改为「load epoch → 检查谓词 → `wake_epoch_.wait(epoch)`」，`WakeUpOne/WakeUpAll`
  改为 `fetch_add + notify`；移除 `wait_mutex_`/`work_available_`（不再需要 condition_variable）。
- 不动：`MpscQueue`、`Post` 的入队 CAS、`pending_tasks_` 计数语义。
- 验证：单任务 ping 连续 20 轮无超时（修复前第 5/36 次迭代即失败）；1000 次 ping 耗时从
  200ms+ 降至 1–4ms；多生产者 100000 任务 drain 连续 20 轮通过；unit 95/95；全量构建 + 两道门禁通过。
- 报告：`docs/test-reports/run-20260816-eventloop-sync-fix.md`
