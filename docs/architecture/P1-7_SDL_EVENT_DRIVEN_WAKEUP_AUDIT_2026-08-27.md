# P1-7 SDL EVENT_DRIVEN 唤醒现状审计与集成测试规划

- 日期：2026-08-27
- 输入来源：用户对 `d:\code\VMP-ui` 当前 P1-7 实现的只读分析请求；`docs/pm/run-20260827-0000-p1-7-sdl-event-driven-wakeup.md`；`docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md`；当前生产源码与测试配置
- 作用范围：SDL event watch → `EventLoop` 唤醒 → `FrameTick` → `InteractionSystem::pollSdlEvents()` → `Dispatcher` 的线程归属、生命周期和最小真实集成测试
- 本文仅为架构与修改规划，不修改生产源码、测试源码或 CMake。

## 1. 现状与结论

当前链路是正确的最小方向，但真实 SDL 队列矩阵尚无证据：

```text
SDL_PushEvent
    ├─ SDL event watch（SDL 所在线程，当前在 Application.cpp 私有安装）
    │      └─ EventLoop::notifyExternalEvent()
    │             └─ epoch++ + condition_variable.notify_all()
    └─ SDL 事件队列
           └─ EventLoop 投递 default handler
                  └─ FrameTick（UI 消费线程）
                         └─ SystemManager::pollInput()
                                └─ InteractionSystem::pollSdlEvents()
                                       └─ Dispatcher 转换/入队/触发
```

已确认的生产事实：

- `ApplicationImpl` 在 SDL 初始化、系统注册和默认帧处理器建立后调用 `SDL_AddEventWatch`；析构时先移除 watch，再注销系统并关闭 SDL。
- `WakeEventLoop` 只把 `userdata` 转成 `EventLoop*` 并调用 `notifyExternalEvent()`，不访问 `Registry`、`Dispatcher`、日志、SDL poll 或事件转换。
- `EventLoop` 的 `EVENT_DRIVEN` 等待使用持久 `observedEpoch`；外部事件、`invoke()`、模式切换和退出均可改变 epoch 并通知等待者。
- `FrameTick` 是唯一生产帧入口；它先在 `POLL_INPUT` 阶段调用 `InteractionSystem`，随后按内部/公开队列和 ECS 阶段处理事件。
- `InteractionSystem` 在 `SDL_PollEvent` 所在线程直接完成 SDL→内部事件转换：pointer/keyboard/resize/expose 分别进入 buffered 或 immediate Dispatcher 路径。
- 当前 `tests/unittest/test_CoreEventLoopScheduling.cpp` 只验证抽象 `notifyExternalEvent()`/`invoke()` 和单向 `EVENT_DRIVEN → FIXED_RATE`，不能证明 SDL watch、事件字段转换或 `ApplicationImpl` 的安装。
- `Application` 公共头没有帧调度模式 setter/getter；测试若直接创建 `Application`，无法把真实应用切到 `EVENT_DRIVEN`，也无法直接观察其私有帧计数。

**判断：** P1-7 的唤醒实现可作为基础，但“每种真实 SDL 事件在无 `invoke()` 时推进一帧”仍是未验证的集成验收项；不能用现有 EventLoop 单元测试替代。

## 2. 线程与所有权契约

| 阶段 | 所在线程 | 允许操作 | 明确禁止 |
|---|---|---|---|
| `SDL_AddEventWatch` 回调 | SDL 可能调用的线程，不能假定是 UI 消费线程 | 对绑定 `EventLoop` 执行 `notifyExternalEvent()` | `SDL_PollEvent`、Registry、Dispatcher、SystemManager、日志、分配复杂对象、抛异常 |
| `EventLoop` 调度线程 | 当前 `EventLoop::exec()` 所在线程；测试中应作为 UI 消费线程 | 等待 epoch，投递 default handler | 让 watch 回调直接执行 `FrameTick` |
| `FrameTick`/`InteractionSystem` | `exec()` 消费线程 | `SDL_PollEvent`、SDL 事件字段读取和转换、Registry/Dispatcher 操作 | 从 watch 或生产者线程触碰 ECS |
| 测试生产者 | 测试线程 | 构造并调用 `SDL_PushEvent`，只读同步观测量 | 直接调用 `pollSdlEvents()` 代替真实唤醒路径 |

测试证明线程归属的最低成本方式是：在生产 `FrameTick` handler 内记录 `std::thread::id`，在各 Dispatcher 监听器内记录同一 ID；watch seam 的回调只接受 `EventLoop&`，不能获得 `UiRuntime&`。不要把“监听器被调用”误写成 watch 线程执行证明；它只能证明转换发生在消费线程。

## 3. 推荐生产 seam

### 3.1 抽取唯一 watch 组件，而不是在测试复制 callback

新增内部组件：

- `src/core/SdlEventWakeup.hpp`
- `src/core/SdlEventWakeup.cpp`（若平台/SDL 回调实现不适合保持 header-only）

建议契约：`ui::detail::SdlEventWakeup` 为不可拷贝的 RAII 对象，构造时接收 `EventLoop&`，调用 `SDL_AddEventWatch`；析构/显式 `reset()` 调用 `SDL_RemoveEventWatch`。静态 callback 只执行 `EventLoop::notifyExternalEvent() noexcept` 并返回 `true`。安装失败通过现有 `Result` 或构造阶段异常风格返回，保持与 `ApplicationImpl` 当前错误处理一致。

`Application.cpp` 只保留：

- 在现有正确时机创建 `SdlEventWakeup` 成员；
- 在系统注销和 SDL shutdown 前销毁该成员。

测试直接复用这个生产组件安装 watch，不复制 `SDL_AddEventWatch`、userdata 和 callback 逻辑。该 seam 的理由是消除当前私有自由函数的复制风险，并使“watch 只唤醒”的契约有唯一实现；不引入事件类型过滤器、事件总线适配层或新的线程抽象。

### 3.2 测试控制 EVENT_DRIVEN 的 seam

推荐增加稳定的、非破坏性 `Application` 转发接口：

- `include/ui/Application.hpp`：`setFrameScheduleMode(FrameScheduleMode)` 与 `frameScheduleMode()`；必要时在公共头前置声明 enum 或放入现有公共调度 API。
- `src/core/Application.cpp`：转发到 `m_eventLoop`。

这是运行时已有能力的正式入口，不是测试专用后门，也支持计划中“运行中双向切换”验收。若本轮不接受公共 API 扩展，则使用仅测试目标可见的内部 `ApplicationTestAccess`/构建宏作为备选，但不推荐：它会把私有实现暴露给测试并增加构建分支。

### 3.3 事件观测不新增业务抽象

测试通过 `application->runtime().dispatcher().sink<...>()` 安装短生命周期监听器，并在 `FrameTick` 之后读取原子观测量。不要给 `InteractionSystem` 增加测试回调或复制一个“测试版 InteractionSystem”；现有依赖注入构造函数已经足够复用真实系统。

## 4. 最小真实 SDL 集成测试矩阵

建议新增 `tests/unittest/test_SdlEventDrivenIntegration.cpp`，加入现有 `ui_eventloop_stress_tests` 或新建 `ui_sdl_event_driven_tests`。推荐新建独立目标，避免把 SDL 全局状态与纯 EventLoop 压测混在一起；该目标使用 `RUN_SERIAL TRUE` 和 `RESOURCE_LOCK ui_sdl_video`。

每个参数化用例都遵循同一协议：

1. 在 fixture `SetUp` 设置 `SDL_VIDEODRIVER=offscreen`（CTest 环境优先设置，代码只作诊断）；确认无残留 video subsystem 后 `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)`。
2. 创建 `SDL_WINDOW_HIDDEN` 的 64×64 窗口；不创建 GPU renderer，避免把图形后端成功与否混入唤醒测试。
3. 构造真实 `UiRuntime`、真实 `SystemManager`（built-ins）、真实 `EventLoop`、真实 `FrameTick`，安装生产 `SdlEventWakeup` seam；或者使用 `Application` 加上上述调度模式转发接口。优先后者验证 Application 的真实安装。
4. default handler 每次执行真实 `FrameTick`，并递增原子帧计数、记录消费线程 ID；不要在测试线程手动调用 `invoke()`。
5. 切到 `EVENT_DRIVEN`，等待一个明确的“循环已运行且空闲”的 ready 信号；记录 `idleFrames`，再等待至少 50 ms，断言帧数不增加（允许启动阶段的一帧，不用固定睡眠判断唤醒成功）。
6. 构造事件并断言 `SDL_PushEvent` 返回成功；用条件变量等待 `frameCount >= before + 1`，超时建议 500 ms，并输出事件类型、SDL 错误、前后帧数和线程 ID。
7. 同时等待对应内部事件观测量，断言事件字段和观测线程 ID等于 FrameTick 消费线程 ID；测试线程只检查原子快照。
8. 每个事件完成后清理队列、断开 listener、销毁窗口；fixture 最后停止 loop、移除 watch、销毁应用/系统、调用 `SDL_Quit`，并断言 `SDL_WasInit(SDL_INIT_VIDEO) == 0`。

矩阵如下：

| 用例 | `SDL_Event` 最小字段 | 内部事件/断言 | 额外说明 |
|---|---|---|---|
| pointer | `SDL_EVENT_MOUSE_MOTION`；`windowID`、`x/y`、`xrel/yrel` | `RawPointerMove` 的 position/delta/windowID；监听器线程=消费线程 | 不依赖真实鼠标状态；不使用 `SDL_WarpMouse` |
| keyboard | `SDL_EVENT_KEY_DOWN`；`key`、`mod`、`repeat=false` | `RawKeyInput` 的 key/pressed/repeat/modifiers；监听器线程=消费线程 | 事件是 immediate，但仍必须由 `pollSdlEvents()` 消费；禁止测试线程直接 trigger |
| resize | `SDL_EVENT_WINDOW_RESIZED`；`windowID`、`data1/data2` | `WindowPixelSizeChanged` 的尺寸、source=`RESIZED`、windowID；监听器线程=消费线程 | 使用 `SDL_PushEvent` 的手工字段，不调用 `SDL_SetWindowSize`，避免驱动自行生成额外事件 |
| expose | `SDL_EVENT_WINDOW_EXPOSED`；`windowID` | `WindowExposed` 的 windowID；监听器线程=消费线程 | offscreen 驱动未必自然产生 expose，所以只验证手工 push 的生产路径 |

四个用例均必须有：

- `SDL_PushEvent` 成功断言；
- 从 `beforeEventFrames` 到至少一帧增长的断言；
- 没有任何 `invoke()` 调用的约束（代码审查 + 测试不链接测试辅助唤醒）；
- 事件转换至少一次且字段精确匹配；
- 事件 listener 和 `FrameTick` 使用同一个消费线程 ID；
- 事件注入后只允许预期的一帧或少量由同批事件合并产生的帧，不以“刚好一帧”作为脆弱断言。

## 5. 生命周期与空闲/切换覆盖

在同一独立 fixture 追加 4 个非参数用例：

1. **IdleDoesNotProduceFrames**：ready 后取基线，等待 100 ms；断言无新增帧。不要断言 CPU 时间，帧计数足够且跨平台稳定。
2. **EventDrivenToFixedRate**：事件驱动空闲后切到 10 fps，条件变量等待一帧；再切回事件驱动，确认短窗口内不再连续产帧。覆盖现有单向测试的真实应用路径。
3. **FixedRateToEventDriven**：先以较低 fps 运行，切到事件驱动并等待静止；随后 push 一个矩阵事件，确认仍能唤醒。这样验证切换不会丢失 watch epoch。
4. **StopAndQuitWithPendingWake**：push 事件后立即请求停止/退出，join 有界（例如 500 ms）；重复执行“先停止后 push”仅作为不崩溃/不挂起检查，不能要求停止后处理业务事件。

所有等待使用 `condition_variable` + 原子计数/序列号；固定超时失败，禁止长时间 `sleep_for` 作为同步机制。允许最多一个很短的启动让步，但 ready 必须来自消费线程实际进入 default handler，而不是时间猜测。

## 6. 测试文件与构建修改规划

| 优先级 | 文件 | 修改内容 | 依赖 |
|---|---|---|---|
| P0 | `src/core/SdlEventWakeup.hpp/.cpp` | 抽取唯一 RAII SDL watch seam；callback 仅唤醒 | 无；先于 Application 改造 |
| P0 | `src/core/Application.cpp` | 删除本文件私有 watch callback/状态实现，改持有并管理 seam | P0 seam |
| P1 | `include/ui/Application.hpp`、`src/core/Application.cpp` | 暴露帧模式最小转发接口，供真实 Application 集成测试控制模式 | 需确认公共 API 接受度 |
| P1 | `tests/unittest/test_SdlEventDrivenIntegration.cpp` | 四类 SDL PushEvent 矩阵、线程归属、空闲和生命周期测试 | P0/P1 生产 seam |
| P1 | `tests/unittest/CMakeLists.txt` | 新测试目标、SDL 链接、`gtest_discover_tests` 的 `sdl;integration` 标签、`RUN_SERIAL`、`RESOURCE_LOCK ui_sdl_video`、offscreen 环境和有界 timeout | 测试文件 |
| P2 | `tests/unittest/test_CoreEventLoopScheduling.cpp` | 保留为纯调度单元测试；补充双向切换仅在真实 SDL 集成测试中，不复制矩阵 | 无 |

不应修改第三方 SDL，不应修改 `InteractionSystem` 业务逻辑；只有在实际编译发现 `SDL_Event` 字段初始化需要平台兼容 helper 时，才在测试文件内部使用小型构造函数，不向生产添加事件转换分支。

## 7. 风险与验证建议

- **SDL watch 回调线程未固定**：测试不依赖具体线程；只验证回调不触碰 ECS，业务 listener 必须在消费线程。
- **offscreen 不自然产生窗口事件**：全部使用手工 `SDL_PushEvent`，resize/expose 不依赖窗口驱动副作用。
- **SDL 全局队列污染**：目标串行并锁 video；每个用例先清空/消费残留事件，销毁顺序固定为 loop stop → watch remove → listeners/systems → window → `SDL_Quit`，且不与已有会话并行。
- **一件事件可能导致多个唤醒/帧**：断言“至少一帧 + 精确事件一次”，不断言恰好一帧；在队列排空后再取最终帧快照。
- **初始化失败误报**：fixture 明确报告 video driver、`SDL_GetError()` 和 `SDL_WasInit`；不要求 renderer/GPU 成功。
- **Application 公共 API 扩展争议**：若拒绝扩展，降级为内部测试 harness 复用 `SdlEventWakeup`、`EventLoop`、`SystemManager`、`FrameTick`；另保留一个轻量 Application 构造/析构安装 smoke test。但这会少一层真实 `ApplicationImpl` 路径，不应宣称等价验收。

验证顺序：先编译并运行单独 SDL 集成目标，再运行 `ctest -L integration`（或新标签），随后运行 `ctest -L unit` 与 Debug 全量构建；只有四类矩阵、空闲、双向切换和退出全部有明确通过证据，才可把 P1-7 的“后续增强项”改为完成。

## 8. 待确认问题

1. 是否接受把 `Application::setFrameScheduleMode()`/`frameScheduleMode()` 作为正式非破坏性公共 API？这是直接验证真实 `ApplicationImpl` 的最小 seam。
2. 是否将 SDL PushEvent 集成测试放入独立 `ui_sdl_event_driven_tests`（推荐），还是扩展现有 `ui_eventloop_stress_tests`？前者能避免把业务集成证据归类为压力测试。
3. 当前 CMake/CTest 是否允许新目标始终使用 `SDL_VIDEODRIVER=offscreen`；若某平台没有 offscreen 驱动，是否接受明确跳过并标记 unsupported，而不是回退到人工桌面窗口？
