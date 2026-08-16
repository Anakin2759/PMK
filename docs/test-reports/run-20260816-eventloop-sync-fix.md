# EventLoop lost-wakeup 修复验证报告（方案 B）

- 日期：2026-08-16
- 修复：`src/utils/EventLoop.hpp` — `atomic_wait/notify` 事件计数替换 condition_variable 唤醒
- 方案：`docs/architecture/EVENTLOOP_SYNC_FIX_REVIEW_2026-08-14.md` 方案 B（用户 2026-08-16 批准）

## 修改内容

- 新增 `wake_epoch_`（`std::atomic<std::size_t>`）唤醒事件计数。
- `WaitForWork()`：快照 epoch → 检查谓词（exit 或 pending>0）→ `wake_epoch_.wait(epoch)`。
- `WakeUpOne()/WakeUpAll()`：`fetch_add(1, release)` + `notify_one()/notify_all()`。
- 移除 `wait_mutex_` / `work_available_`（不再需要 condition_variable）。
- 未改动：`MpscQueue`、`Post` 入队 CAS、`pending_tasks_` 计数语义。

## 修复前后对比

| 指标 | 修复前（2026-08-14） | 修复后（2026-08-16） |
|---|---|---|
| 单任务 ping 超时 | 第 5 / 第 36 次迭代各 1 次（200ms） | 连续 20 轮 × 1000 次：**0 超时** |
| 1000 次 ping 耗时 | 200–205ms（含超时惩罚） | **1–4ms**（每次 ping 约 1–4µs） |
| 多生产者 100000 任务 drain | 通过 | 连续 20 轮全部通过 |

## 回归验证

- `ui_eventloop_stress_tests`：2/2 通过（ctest --repeat until-fail:20 全绿）
- unit 测试：**95/95 通过**
- Ninja all：退出码 0
- 架构门禁：通过（341 处 `UiRuntime::current()`，无新增债务）
- 公共头门禁：通过（0 violations）
- 合并库 `VMPUI.lib` 与全部 5 个测试 exe、benchmark exe、demo exe 重新链接完整

## 结论

方案 B 落地，lost-wakeup 从"可重复复现"转为"已修复"。P1-4（帧循环事件驱动化）的前置阻塞解除。
