# WP5 public queued event 固定阶段变更记录

- 日期：2026-07-17
- 范围：WP5 最小基础批次
- 状态：completed

## 决策与实现

公开 `ui::event::Enqueue()` 已接入常规帧的 `QueuedTask`，阶段顺序固定为：

1. 推进 `FrameContext`；
2. 触发 Timer；
3. 派发内部 EnTT buffered events；
4. 派发当前 Runtime 的公开 queued events；
5. 后续进入 Input 和 Layout/Render。

`QueuedTask` 在执行期间使用 `UiRuntimeScope` 显式激活其绑定的 Runtime，因此嵌套 Application 或另一个 active current 不会导致跨 Runtime 派发。公开队列继续通过 `std::exchange` 提取本轮 pending 批次，所以 callback 中再次入队的事件会留到下一调度帧。`DispatchQueued()` 保留为显式测试和高级控制入口。

架构门禁现在要求生产帧路径中恰好存在一个 `DispatchQueued()` 接入点。

## 契约测试

新增测试固定以下行为：

- `Enqueue()` 同步返回前不触发 callback；
- 下一次 `QueuedTask` 在内部 Timer/buffered 阶段后自动派发；
- 派发中递归入队延后到再下一调度帧；
- `QueuedTask` 只派发其绑定 Runtime 的队列。

## 验证

- Debug 构建：通过。
- 架构门禁：通过；queued event 生产帧派发点为 1。
- TaskChain/PublicEvent 定向测试：13 passed / 0 failed。
- 全量测试：170 passed / 0 failed。
- 架构指标：`UiRuntime::current()` 302，PUBLIC 内部 include 2，PUBLIC 内部依赖 3，queued event 派发点 1。

## 未包含

本批没有建立唯一 `FrameTick`，没有移除 Task 内 16/32 ms countdown，没有统一 scheduler policy，也没有整理即时 Layout/Render 补救白名单。因此 WP5 仍为 active / partial。
