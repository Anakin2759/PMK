# 项目经理协调记录 - Runtime 所有权与注入边界纠偏

- 时间：2026-08-27
- 输入来源：用户否定提交 `20f0ced2` 引入的公开 Runtime 设计；依据 `docs/todo/想法.md`
- 基线：HEAD `ec439eb3`；用户确认工作区起始 `git status` 干净
- 本轮范围：规划、分批实施、架构门禁与窄验证；禁止提交或推送
- 验收标准：公共头和示例不暴露 `UiRuntime`；公开 Factory/Chains/API 无 Runtime 参数；Application 唯一持有 Runtime；仅 System 接收 Runtime，并向内部依赖传具体依赖；非 System 不保存或反查完整 Runtime；保留 P1-7/event-driven/render-dirty 等无关修复

## 需求判定

本任务跨公共 API、Application 生命周期、System 注入、Registry/内部依赖、示例、测试和架构门禁，且用户明确要求 PM 全流程，采用 Full PM Path。不得整体 revert `20f0ced2`。

## 工作包

| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | Runtime 所有权、调用链及差异规划 | 架构师 | 本记录、`docs/todo/想法.md`、`20f0ced2^..ec439eb3`、当前源码 | `docs/architecture/RUNTIME_OWNERSHIP_CORRECTION_PLAN_2026-08-27.md` | 阻塞：当前会话无子 agent 调度入口 |
| 2 | 公共 API、Chains/Factory 与示例恢复 | 代码工厂 | WP1 规划条目与显式文件清单 | 源码变更和交付报告 | 待办 |
| 3 | Application/System 注入和内部依赖去 Runtime 化 | 代码工厂 | WP1 规划条目与显式文件清单 | 源码变更和交付报告 | 待办 |
| 4 | 架构门禁与必要回归测试 | 代码工厂 | WP1、WP2、WP3 交付报告 | 门禁/测试变更和交付报告 | 待办 |
| 5 | 窄构建与测试闭环 | 测试构建闭环 | WP2-WP4 交付报告 | 验证报告和问题日志 | 待办 |

## 工作包闸门

### WP1：架构规划

- 目标：逐项区分 `20f0ced2` 的 Runtime 暴露改动与必须保留的 P1-7/event-driven/render-dirty 改动；给出可实施的文件级规划。
- 依据：`docs/todo/想法.md`；用户列出的 10 项硬目标；提交范围 `20f0ced2^..ec439eb3`；当前调用点。
- 允许读取：`include/ui/**`、`src/**`、`example/**`、`tests/**`、`tools/check_architecture_boundaries.py`、相关 CMake 文件、上述提交差异和现有架构/PM 文档。
- 只允许写入：`docs/architecture/RUNTIME_OWNERSHIP_CORRECTION_PLAN_2026-08-27.md`。
- 禁止：修改源码、测试、构建配置；不得建议整体 revert；不得以 public getter、组件注入或 `Registry::runtime()` 替代。
- 验收：列出条目号、现状调用链、目标调用链、允许文件、禁止触达、保留改动、测试影响、风险；明确 internal-only context/具体服务集合的所有权和线程/生命周期边界；明确 Application 唯一拥有 Runtime 且外部不可调用。
- 升级条件：无法区分无关修复、无法确定具体服务集合或需要新增公开类型时，标为待用户确认。

### WP2：公共边界与示例

- 目标：`include/ui/**` 和 `example/**` 清除 `UiRuntime`/`WithRuntime`/`app->runtime`，恢复无 Runtime 参数的 Factory/Chains/API 调用形态。
- 依据：WP1 中“公共边界”条目。
- 只允许触达：WP1 对每条列出的 `include/ui/**`、对应 `src/api/**`、`example/**` 和直接编译检查文件。
- 禁止：System/Registry 内部重构、无关 P1-7/event-driven/render-dirty 代码、公开 Runtime getter、组件注入、新第三方依赖、未授权 Git 操作。
- 验收：文本门禁目标 1-3 全满足；公共头保持自包含；交付报告列出实际修改文件及残留调用。
- 升级条件：自由 public API 无法通过 internal-only 入口定位具体服务，或需把任何 Runtime 类型放入 `include/ui`。

### WP3：所有权与内部注入

- 目标：Application 继续唯一拥有 Runtime；SystemManager/System 允许构造注入 Runtime；System 仅向内部依赖传具体依赖，消除 `Registry::runtime()` 及非 System 保存/反查完整 Runtime。
- 依据：WP1 中“所有权/内部注入”条目。
- 只允许触达：WP1 显式列出的 `src/core/**`、`src/systems/**`、`src/managers/**`、`src/renderers/**`、`src/utils/**` 及直接单元测试。
- 禁止：公共 getter、Runtime 组件化、范围外重命名/抽象、整体 revert、改动无关 P1-7/event-driven/render-dirty 行为、未授权 Git 操作。
- 验收：目标 4-8 满足；每个非 System 依赖只接收 Registry/Dispatcher/Logger/具体服务等最小依赖；交付报告含所有权图、变更文件和残留风险。
- 升级条件：出现生命周期倒置、跨线程 context 不安全、非 System 无法避免持有 Runtime，或必须修改 WP1 未授权文件。

### WP4：门禁与测试

- 目标：把本轮架构约束固化为默认门禁，并补充最小必要测试；禁止仅靠 baseline 计数掩盖债务。
- 依据：WP1 条目及 WP2/WP3 交付报告。
- 只允许触达：`tools/check_architecture_boundaries.py`、WP1 明确的 `tests/support/**`、`tests/unittest/**`、相关测试 CMake 文件。
- 禁止：生产逻辑、降低现有门禁、删除无关回归、未授权 Git 操作。
- 验收：门禁至少检查 `include/ui/**` 无 `UiRuntime`、`example/**` 无 `UiRuntime|WithRuntime|app->runtime`、非 System 无 `Registry::runtime` 方向；测试覆盖 Application 所有权、System 构造注入及无 Runtime 公共调用形态。
- 升级条件：门禁存在高误报且无法用语法/路径白名单准确表达，或测试需要新增生产接口。

### WP5：窄验证

- 目标：验证公共边界、示例构建、直接受影响 API/System 以及无关关键修复未回归。
- target：`ui_architecture_boundary_check`、`ui_public_headers_self_contained_check`、`example_ui_demo`、受影响的最窄单测 target（由 WP1 明确，优先现有 `ui_unit_tests`/`ui_ecs_tests`，不得先跑全量）。
- tests：WP1/WP4 新增或直接受影响的 Runtime/Application/SystemManager/Factory/Chains 测试过滤集；追加现有 `RenderDirtyTest.*` 与 P1-7 `event-driven` 标签窄回归（若对应 target 已受本轮触达）。
- 报告期望：`docs/validation/runtime-ownership-correction-2026-08-27.md`，包含命令、退出码、通过/失败数、文本扫描结果、问题日志新增条数、未运行项及原因。
- 失败策略：测试/构建脚本笔误可在 WP4 授权范围内修复一次；业务、ABI、生命周期或无关回归失败退回对应代码工作包；同阶段失败两次升级用户。
- 验收：指定门禁与 `example_ui_demo` 构建全绿；直接测试全绿；聊天返回报告路径、阶段结果和问题日志新增条数。

## 调度时间线

- 2026-08-27：读取路由规则，确认 Full PM Path。
- 2026-08-27：读取 `docs/todo/想法.md`、既有 P1-2 PM 记录并扫描 Runtime 暴露；当前扫描显示 86 个文件存在相关命中，公共头、示例、API 和 Registry 方向均受影响。
- 2026-08-27：建立本协调记录；发现当前工具集未提供任何子 agent 调度入口，按 PM 权限边界暂停，未修改源码。

## 待用户决策

- [ ] 启用本会话的“架构师、代码工厂、测试构建闭环”子 agent 调度能力后继续；当前 PM 模式禁止项目经理亲自规划架构、修改源码或运行构建。

## 结论

- 状态：阻塞
- 关键产物：`docs/pm/run-20260827-0001-runtime-ownership-correction.md`
- 实际修改文件：仅本 PM 协调记录
- 下一工作包：WP1 Runtime 所有权、调用链及差异规划
