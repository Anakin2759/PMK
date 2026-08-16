# P1-4 帧循环双模式调度 — 交付说明

- 日期：2026-08-16
- 前置：P0-4 EventLoop lost-wakeup 修复（方案 B `atomic_wait/notify`）已落地
- 用户决策：保留固定帧数路径，后续提供锁帧接口

## 设计

`src/core/EventLoop` 增加双模式帧调度（`FrameScheduleMode`）：

| 模式 | 行为 | 用途 |
|---|---|---|
| `FIXED_RATE`（默认） | 以 `setTargetFrameRate`（默认 60fps）精确节流，`sleep_until` 避免累积漂移；兼容历史 16ms 行为 | 动画/渲染固定节奏，向后兼容 |
| `EVENT_DRIVEN` | 空闲时挂起（CPU 归零），`invoke()` 投递任务或 `quit()` 时唤醒调度器并投递默认处理器 | 输入响应式 UI，低功耗 |

锁帧接口：

- `setTargetFrameRate(fps)` / `targetFrameRate()` — 0 表示不锁帧
- `setFrameScheduleMode(mode)` / `frameScheduleMode()` — 运行期可切换

## 关键实现

- `frameSchedulerLoop(stop_token)`：双分支调度循环。
- `m_scheduleEpoch` 唤醒纪元：事件驱动模式等待谓词检测纪元变化，消除「notify 先于 wait 注册」的丢失窗口（与 `utils::EventLoop` 方案 B 同原理——首次测试即复现并修复）。
- `invoke()` 在事件驱动模式下 `fetch_add(epoch) + notify_all`。
- `quit()` 递增纪元并唤醒，保证调度器线程可退出。

## 验证

| 项 | 结果 |
|---|---|
| `FixedRateDeliversFramesOnInterval` | 100fps 下 60ms 约 5–7 帧 ✅ |
| `EventDrivenWakesOnInvoke` | 空闲 1 帧，invoke 后 2 帧（唤醒生效）✅ |
| `TargetFrameRateCanBeChanged` | 帧率/模式运行期切换 ✅ |
| stress（含 2 项 lost-wakeup 压测） | 5/5 ✅ |
| unit | 95/95 ✅ |
| Ninja all + 两道门禁 | 退出码 0 ✅ |

新增测试：`tests/unittest/test_CoreEventLoopScheduling.cpp`（3 项）。
