# 项目经理协调记录 - P1-7 SDL EVENT_DRIVEN 外部事件唤醒

- 时间：2026-08-27
- 输入来源：用户要求端到端复核并完成 `docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md` P1-7
- 本轮范围：现状复核、缺口规划、实现、可重复测试、构建测试闭环、状态文档同步
- 验收标准：EVENT_DRIVEN 下通过 `SDL_PushEvent` 分别注入 pointer、keyboard、resize、expose，均可在无 `invoke()` 情况下唤醒并推进一帧；SDL event watch 只发送 wake signal，SDL poll/事件转换只在 UI 消费线程；运行中 EVENT_DRIVEN↔FIXED_RATE 切换可靠；停止/退出不挂起；空闲不持续产帧；架构门禁、Debug `all --parallel 1`、相关及 `unit` CTest 全绿后方可标记 P1-7 DONE。

## 当前事实快照

- 主计划已于 2026-08-26 将 P1-7 标为 DONE，但同段“后续增强项”明确尚未补齐 `SDL_PushEvent` pointer/keyboard/resize/expose 集成矩阵，因此当前 DONE 证据不足，必须重新验收。
- `src/core/Application.cpp` 已安装 SDL event watch；回调目标为 `EventLoop::notifyExternalEvent()`，需继续核实回调实现确实不访问 Registry/Dispatcher/Logger。
- `src/core/EventLoop.cpp` 已有持久 `observedEpoch`、外部唤醒、模式切换通知和 quit 唤醒逻辑；现有测试只覆盖抽象 `notifyExternalEvent()` 与 EVENT_DRIVEN→FIXED_RATE，未覆盖真实 SDL 入队矩阵、反向切换、停止竞态及 UI 线程转换归属。
- `src/systems/InteractionSystem.hpp` 当前负责在帧内 poll 并转换鼠标、键盘及窗口事件；需通过结构审计和测试确认 watch 不做业务转换。
- `ui_eventloop_stress_tests` 当前标记为 `stress;concurrency`，相关专项测试目标/标签及 SDL 初始化隔离策略需由规划明确。

## 工作包

| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | P1-7 现状审计、线程归属与测试矩阵规划 | 架构师 | 本记录、P1-7 主计划、当前 EventLoop/Application/InteractionSystem/测试 | `docs/architecture/P1-7_SDL_EVENT_DRIVEN_WAKEUP_AUDIT_2026-08-27.md` | DONE |
| 2 | 按规划补齐生产实现缺口 | 代码工厂 | WP1 规划条目 | `SdlEventWakeup` RAII seam 与 Application 接入 | DONE |
| 3 | 补齐 SDL PushEvent 与调度生命周期可重复测试 | 代码工厂 | WP1 测试矩阵条目、WP2 交付报告 | `ui_sdl_event_driven_tests` 真实事件矩阵 | DONE（4/4） |
| 4 | 架构门禁、Debug all、专项与 unit CTest 闭环 | 测试构建闭环 | WP2/WP3 交付报告；目标与命令见下 | 当前工作区验证结果 | DONE |
| 5 | 仅在 WP4 全绿后同步主计划与 TODO 索引 | 代码工厂 | WP4 测试报告 | 主计划与 TODO 索引 | DONE |

## 工作包闸门

### WP1：架构与测试规划

- 目标：复核 P1-7 验收标准，追踪 SDL watch→调度唤醒→帧任务→UI 线程 `SDL_PollEvent`→事件转换完整链路，列出所有实际缺口和可重复测试矩阵。
- 依据：`docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md` P1-7、`src/core/Application.cpp`、`src/core/EventLoop.hpp/.cpp`、`src/systems/InteractionSystem.hpp`、`tests/unittest/test_CoreEventLoopScheduling.cpp`、相关 CMake。
- 允许读取：`src/core/**`、`src/systems/InteractionSystem.hpp` 及其直接依赖、`tests/**`、根及测试 CMake、两份指定状态文档。
- 只允许写入：`docs/architecture/P1-7_SDL_EVENT_DRIVEN_WAKEUP_AUDIT_2026-08-27.md`。
- 禁止：修改源码、测试、构建配置、状态文档；不得假定当前 DONE 结论有效。
- 验收：必须给出线程/所有权边界、SDL watch 回调允许操作清单、pointer/keyboard/resize/expose 的事件字段和断言、EVENT_DRIVEN↔FIXED_RATE 双向切换、停止/退出、空闲不忙循环、重复/批量事件和竞态窗口测试；逐条列明允许修改文件。
- 升级条件：SDL dummy/offscreen 驱动无法稳定表达窗口事件、需改变公共 API/ABI、或测试必须依赖真实桌面窗口时标记待确认。

### WP2：生产实现

- 目标：仅实现 WP1 认定的生产缺口，保证 watch 只调用无抛出轻量唤醒原语，Registry/Dispatcher/Logger 访问及 SDL poll/转换只发生在 UI 消费线程。
- 依据：`docs/architecture/P1-7_SDL_EVENT_DRIVEN_WAKEUP_AUDIT_2026-08-27.md` 的生产条目号。
- 允许文件：由 WP1 显式列出的 `src/core/Application.cpp`、`src/core/EventLoop.hpp/.cpp`、`src/systems/InteractionSystem.hpp` 及必要的直接声明文件；未列出者禁止触达。
- 禁止：范围外文件、公共 API/ABI 变更、无关重命名/润色、新依赖、新抽象、状态文档提前标 DONE、未授权 Git 操作。
- 验收：运行中双向模式切换、外部事件、停止/退出均不会丢唤醒或挂起；空闲无持续帧；静态诊断零新增 error/warning；交付报告列出变更文件及残留风险。
- 升级条件：需要跨模块生命周期重构、改变 Application 公共配置、或 SDL 回调安全契约无法在现有结构内满足。

### WP3：专项测试

- 目标：补齐真实 `SDL_PushEvent` 类型矩阵和调度生命周期测试，并保证可重复、可独立清理 SDL 全局状态。
- 依据：WP1 测试条目、WP2 交付报告。
- 允许文件：WP1 明确的 `tests/unittest/**` 或新 P1-7 专项目标文件，以及 `tests/unittest/CMakeLists.txt`；生产文件只读。
- 禁止：通过长时间 sleep 掩盖竞态、依赖人工窗口交互、修改第三方 SDL、放宽既有断言、把压力测试冒充类型矩阵集成测试。
- 验收：pointer、keyboard、resize、expose 每类均验证 `SDL_PushEvent` 成功、无 `invoke()` 仍推进帧、事件由 UI 线程 poll/转换；覆盖 EVENT_DRIVEN→FIXED_RATE、FIXED_RATE→EVENT_DRIVEN、quit/stop、空闲不忙循环；失败有明确超时而非永久挂起。
- 升级条件：测试因 SDL 全局状态与其他用例冲突、平台事件字段不稳定、或必须引入新的测试 seam 时回到 WP1。

### WP4：测试构建闭环

- 目标：依次执行诊断、架构门禁、Debug 全量串行构建、P1-7 专项/相关测试和全量 unit 标签。
- target/tests：
  - 架构门禁：`ui_architecture_boundary_check`、`ui_public_headers_self_contained_check`；
  - 全量构建：`cmake --build build --config Debug --target all --parallel 1`；
  - 相关测试：WP3 新增/调整的 P1-7 专项目标及 `CoreEventLoopSchedulingTest.*`，`--output-on-failure`；
  - 全量测试：`ctest --test-dir build -C Debug -L unit --output-on-failure`。
- 允许文件：测试报告和问题日志路径由测试 agent 创建；仅可自修本轮引入的测试脚本/CMake 笔误。
- 禁止：修改业务语义、跳过失败测试、并行替代用户指定的 `--parallel 1`、未全绿即宣告 DONE。
- 验收：聊天返回报告路径、各阶段命令/退出码/通过数、问题日志新增条数；所有阶段全绿。
- 升级条件：业务/线程/生命周期失败退回 WP2；测试设计失败退回 WP3；同阶段修复两次仍失败则升级用户。

### WP5：状态同步

- 目标：依据 WP4 的当前工作区全绿证据，同步 `docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md` P1-7 和 `docs/todo/README.md`。
- 依据：WP4 测试报告路径和精确通过数。
- 只允许触达：`docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md`、`docs/todo/README.md`。
- 禁止：修改其他规划状态、复用 2026-08-26 的旧验证结果、测试未全绿时标 DONE。
- 验收：主计划逐项记录 SDL 类型矩阵、线程归属、双向模式切换、退出/停止、空闲不忙循环及命令证据；TODO 索引准确反映 P1-7 DONE 和复核日期。
- 升级条件：WP4 非全绿或存在未关闭的 P1-7 阻塞风险时保持 ACTIVE/BLOCKED。

## 调度时间线

- 2026-08-27：依据路由规则确认该任务涉及运行时生命周期、跨模块实现、测试与状态文档闭环，进入 Full PM Path。
- 2026-08-27：读取 P1-7 主计划、EventLoop/Application/现有测试及 TODO 索引，确认旧 DONE 与缺失 SDL PushEvent 类型矩阵证据矛盾。
- 2026-08-27：建立本协调记录；当前会话未提供子 agent 调度入口，WP1 无法派发。

## 结论

- 状态：DONE
- 关键产物：`docs/pm/run-20260827-0000-p1-7-sdl-event-driven-wakeup.md`
- 验收证据：Debug `all` 串行构建退出码 0；P1-7 CTest 4/4；unit 108/108；架构门禁通过。
