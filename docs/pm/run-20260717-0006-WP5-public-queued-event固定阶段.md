# 项目经理协调记录 - WP5 public queued event 固定阶段

- 时间：2026-07-17
- 输入来源：用户要求依据最新路线图与 2026-07-17 已确认决策，实际实现、测试并回写文档
- 本轮范围：WP5 最小基础批次——仅将 `ui::event::Enqueue()` 接入下一调度帧的固定自动派发阶段；不实施完整 WP5，不引入通用任务图
- 验收标准：公开 queued event 在生产帧路径恰有一个自动派发点；入队当下不触发；下一次调度帧在 Layout/Render 前触发；派发中再次入队延后到再下一帧；多 Runtime 不串队列；定向与全量测试、公开头检查和架构门禁通过；路线图状态与指标回写

## 工作包

| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | 固定阶段契约与最小影响分析 | 主 Agent 接续 | 路线图 WP5、决策门、现有 `TaskChain` 与 public event bridge | `docs/architecture/change-wp5-public-queued-event-stage-20260717.md` | completed |
| 2 | 最小生产接入与契约测试 | 主 Agent 接续 | 工作包 #1 规划条目 | 源码/测试变更及交付报告 | completed |
| 3 | 定向构建、测试与门禁闭环 | CMake Tools / Test Explorer | 工作包 #2 交付报告 | 构建与测试结果 | completed |
| 4 | 路线图与本记录验收回写 | 主 Agent 接续 | 工作包 #2、#3 产物 | 路线图最新状态、指标和残余风险 | completed |

## 选择结论

选择 WP5 的“public queued event 下一帧固定阶段自动派发”作为当前最小批次。理由：

1. 2026-07-17 决策门已明确承诺自动派发，原决策阻塞已解除。
2. 当前 `QueuedTask -> InputTask -> RenderTask` 已提供最小固定顺序，且 `QueuedTask` 已是帧上下文推进与 buffered event 派发阶段。
3. public event queue 已通过 `std::exchange` 获取 pending 批次，天然支持“派发中再次入队延后到下一帧”。
4. 批次可限制为一个生产接入点、契约测试、门禁指标和文档回写；无需改节流策略、唯一 FrameTick 类型或引入任务图。

## 工作包 #1：架构师派发规格

- **目标**：确定 public queued event 在现有轻量固定管线中的唯一自动派发位置，并给出不破坏多 Runtime 隔离的最小调用边界。
- **依据**：`docs/architecture/ARCHITECTURE_REVIEW_AND_ROADMAP_2026-07-11.md` 的 WP5、3.6、3.7、决策门与“明确不做”；`src/core/TaskChain.hpp`；`src/api/Event.cpp`；`src/helper/Helper.hpp`；`tests/unittest/test_TaskChain.cpp`；`tests/unittest/test_PublicEventApi.cpp`；`tools/check_architecture_boundaries.py`。
- **允许文件**：仅新增 `docs/architecture/change-wp5-public-queued-event-stage-20260717.md`。
- **禁止文件**：全部源码、测试、CMake、路线图、PM 记录、第三方目录；禁止执行命令和 Git 操作。
- **验收标准**：文档明确阶段顺序、下一帧定义、递归入队语义、disconnect-during-dispatch 语义、异常策略现状、多 Runtime 隔离方式、最小文件范围、测试矩阵、风险及不做项；不得提出通用任务图。
- **失败后的升级条件**：若最小接入必须依赖隐式 `UiRuntime::current()` 且可能与 `QueuedTask::runtime` 不一致，或需改变公开 API/异常策略/Task 节流，则标记待确认，不替用户决策。

## 工作包 #2：代码工厂派发规格

- **目标**：按工作包 #1 的规划条目，将 public queued event 接入下一调度帧固定阶段并补充最小契约测试与指标门禁。
- **依据**：`docs/architecture/change-wp5-public-queued-event-stage-20260717.md` 的实施条目。
- **只允许触达**：
  - `src/core/TaskChain.hpp`
  - `src/api/Event.cpp`
  - `src/helper/Helper.hpp`
  - `include/ui/api/Event.hpp`（仅当规划要求澄清既有 API 注释；禁止新增 API）
  - `tests/unittest/test_TaskChain.cpp`
  - `tests/unittest/test_PublicEventApi.cpp`
  - `tools/check_architecture_boundaries.py`
- **禁止文件/事项**：`Application.cpp`、其他 System/Task、CMake、路线图与 PM 文档、第三方目录；禁止修改 16/32ms 节流、引入 FrameTick/调度器/任务图、新依赖、新公开 API、无关重命名或格式化、Git 操作。
- **验收标准**：
  1. 生产帧路径恰有一个 public queued event 自动派发点。
  2. 固定顺序可测试：FrameContext 推进及既有 timer/内部 buffered 阶段后，public queued event 派发，再进入 Layout/Render。
  3. `Enqueue()` 同步返回前不触发；下一次 `QueuedTask` 执行时触发一次。
  4. callback 派发中再次 `Enqueue()` 的事件不在同次 drain 中执行，而在再下一调度帧执行。
  5. disconnect-during-dispatch 保持安全且后续 callback 行为由测试固定。
  6. 两个 Runtime 的 event table/queue 不串扰；自动派发必须针对 `QueuedTask::runtime`，不得错误依赖另一个 active current。
  7. 手动 `DispatchQueued()` 保留为显式控制入口，既有 API 兼容。
  8. 架构门禁将 queued-event 生产帧派发点从 0 固化为恰好 1，并能定位真实接入点。
  9. 静态自检零已知 error/warning；交付报告列出全部变更文件。
- **失败后的升级条件**：若需要新增公开 runtime-bound API、修改异常传播策略、变更 Task 节流/阶段数量、触达 `Application.cpp` 或范围外生产文件，立即停止并升级，不自行扩包。

## 工作包 #3：测试构建闭环派发规格

- **任务**：复用现有 `D:/test/VMP-ui/build` Debug 配置执行 build、定向单测、公开头检查、架构门禁及全量 CTest。
- **目标**：
  - build-dir：`D:/test/VMP-ui/build`
  - targets：`ui_architecture_boundary_check`、`ui_public_header_check`、`ui_api_tests`、`ui_ecs_tests`，随后默认 Debug build
  - tests：`PublicEventApiTest.*`、`PublicEventApiIsolationTest.*`、`TaskChainTest.*`，随后全量 CTest
  - 报告期望：命令、阶段结果、通过/失败数量、失败测试名、架构指标（尤其 queued dispatch sites=1）、问题日志新增条数、报告路径
- **失败策略**：仅测试筛选、断言或门禁定位规则的明显笔误可在工作包 #2 允许文件内最小修正一次；业务/接口/多 Runtime/异常问题写问题日志并升级。
- **验收标准**：各目标构建成功；定向与全量测试全绿；架构门禁和公开头检查通过；聊天回执包含报告路径、阶段结果、问题日志新增条数。
- **失败后的升级条件**：同一阶段修正后仍失败，或修复需范围外文件，停止并升级。

## 工作包 #4：文档回写派发规格

- **目标**：依据已通过的实现与测试事实，更新路线图 WP5 状态、完成度、指标和执行快照，并闭合本 PM 记录。
- **依据**：工作包 #2 交付报告与工作包 #3 测试报告；不得依据预期结果回写。
- **允许文件**：
  - `docs/architecture/ARCHITECTURE_REVIEW_AND_ROADMAP_2026-07-11.md`
  - `docs/architecture/change-wp5-public-queued-event-stage-20260717.md`
  - `docs/pm/run-20260717-0006-WP5-public-queued-event固定阶段.md`
- **禁止文件**：源码、测试、CMake、其他 todo/架构/PM 文档、第三方目录、Git 操作。
- **验收标准**：WP5 仍标记 partial/active 而非 completed；明确本批只完成 public queue 固定阶段接入，唯一 FrameTick、Task 节流统一和即时补救白名单仍未完成；量化指标更新为实际值；记录实际测试数量和残余风险。
- **失败后的升级条件**：测试未全绿、报告缺失或实际派发点不为 1 时，不得将路线图写成已完成。

## 调度时间线

- 2026-07-17：核验路线图最新状态、2026-07-17 决策门、现有 TaskChain、public event queue、测试和架构门禁。
- 2026-07-17：确认采用 WP5 最小基础批次，不扩大到完整 WP5/WP7，不引入通用任务图。
- 2026-07-17：完成四阶段工作包、文件边界、验收标准和失败回路定义。
- 2026-07-17：因当前会话未提供子 Agent 调度入口，且项目经理模式禁止亲自修改源码、运行构建或作架构决策，执行阶段阻塞。

## 实际交付

- `src/core/TaskChain.hpp`：在绑定 Runtime scope 中增加唯一公开 queued event 派发点。
- `include/ui/api/Event.hpp`：澄清自动下一调度帧语义及手动派发用途。
- `tests/unittest/test_TaskChain.cpp`：新增自动派发、递归延后一帧和多 Runtime 隔离测试。
- `tools/check_architecture_boundaries.py`：生产派发点必须恰好为 1。
- Debug 构建通过；定向测试 13/13；全量测试 170/170；问题日志新增 0 条。
- 指标：`UiRuntime::current()` 302、PUBLIC include 2、PUBLIC link 3、queued dispatch sites 1。

## 结论

- 状态：completed（仅指本最小批次；WP5 整体仍为 active / partial）
- 关键产物：固定阶段实现、契约测试、机器门禁、架构变更记录和路线图回写
- 残余风险：唯一 FrameTick、16/32 ms Task 内节流统一、即时补救白名单及输入延迟完整验收仍待后续工作包
