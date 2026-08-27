# 项目经理协调记录 - P1-2 显式 Runtime 依赖迁移

- 时间：2026-08-27
- 输入来源：用户要求端到端完成 P1-2，并以当前工作区重新审计为准
- 本轮范围：审计、规划、分批实现、测试与文档闭环
- 验收标准：生产源码中的 `UiRuntime::current()` 债务归零；若公共 convenience/兼容边界确需保留，则仅保留逐调用点、带理由与生命周期约束的最小白名单；架构门禁、Debug 全量构建、unit CTest 和编辑器诊断全部通过

## 当前事实快照

- 当前架构门禁 baseline 覆盖 20 个生产文件、合计 336 处 `UiRuntime::current()`。
- 债务类型初步分布：Registry（API/helper）、Dispatcher（API/事件入口）、Logger（manager/render/system）、Runtime/context（少量上下文与公共 convenience API）。
- `docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md` 与 `docs/todo/README.md` 均明确 P1-2 仍为 ACTIVE，且禁止将其他 PLANNED/NEEDS-REVIEW 项误标 DONE。
- 现有门禁是按文件计数 baseline，不是逐调用点可审计白名单；若最终不能归零，必须升级为严格白名单结构。

## 工作包

| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | 当前调用分类、调用链与迁移规划 | 架构师 | 本协调记录；当前 20 个 baseline 文件；相关构造/创建调用点 | `docs/architecture/P1-2_EXPLICIT_RUNTIME_DI_AUDIT_2026-08-27.md` | 阻塞：当前会话无子 agent 调度能力 |
| 2 | Registry/helper/API 边界迁移 | 代码工厂 | 工作包 1 中 Registry 条目 | 源码变更与交付报告 | 待办 |
| 3 | Logger/manager/render 边界迁移 | 代码工厂 | 工作包 1 中 Logger 条目 | 源码变更与交付报告 | 待办 |
| 4 | Dispatcher/Runtime/context 与兼容边界迁移 | 代码工厂 | 工作包 1 中 Dispatcher/Runtime 条目 | 源码变更与交付报告 | 待办 |
| 5 | 测试、门禁 baseline/白名单及状态文档同步 | 代码工厂 | 工作包 1-4 的交付报告 | 测试与文档变更报告 | 待办 |
| 6 | 诊断、架构门禁、Debug all、unit CTest 闭环 | 测试构建闭环 | target=all；tests=unit；报告要求见下 | 测试报告与问题日志 | 待办 |

## 工作包闸门

### WP1：架构师审计

- 目标：逐调用点按 Registry / Dispatcher / Logger / Runtime 分类，追踪构造、工厂、SystemManager、RenderSystem/manager 和公共 API 调用链，形成可分批实施的显式文件清单。
- 依据：`tools/check_architecture_boundaries.py`、当前源码、`docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md`、`docs/todo/README.md`。
- 允许读取：`src/**`、`include/**`、`tests/**`、`example/**`、`tools/check_architecture_boundaries.py`、上述两份状态文档及相关 CMake 文件。
- 只允许写入：`docs/architecture/P1-2_EXPLICIT_RUNTIME_DI_AUDIT_2026-08-27.md`。
- 禁止：修改源码、测试、构建配置、门禁 baseline、功能规划状态；不得作批量文本替换方案。
- 验收：必须列出每处/每组调用的分类、上游创建点、拟注入类型、允许文件、测试影响、迁移顺序、风险；确需保留项必须逐调用点说明公共/兼容理由和生命周期约束。
- 升级条件：无法确定对象所有者、注入生命周期或公共 ABI 影响时标为“待确认”，不得自行决策。

### WP2-WP4：代码工厂分批实现

- 目标：严格按 WP1 条目分批迁移，不进行跨批次批量替换。
- 依据：`docs/architecture/P1-2_EXPLICIT_RUNTIME_DI_AUDIT_2026-08-27.md` 的对应条目号。
- 允许文件：必须由 WP1 对每个条目显式列出；当前候选仅限 baseline 中的 20 个生产文件及其直接声明头、构造调用点和直接单元测试。
- 禁止：无关功能规划、无关重命名/风格润色、新依赖、新抽象、修改 PLANNED/NEEDS-REVIEW 状态、未授权 Git 操作。
- 验收：每批静态诊断无新增 error/warning；交付报告列出变更文件、剩余调用和阻塞原因；不得以降低 baseline 计数掩盖实际调用。
- 升级条件：公共 ABI 变化、对象生命周期不清、跨线程 Logger/Dispatcher 所有权不清、需要范围外文件时停止并回到 WP1/用户。

### WP5：门禁、测试与文档同步

- 目标：更新直接受影响测试；将 runtime-current baseline 清零，或改为逐调用点最小可审计白名单；同步两份指定状态文档。
- 允许文件：`tests/**` 中 WP1 明确的直接测试；`tools/check_architecture_boundaries.py`；`docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md`；`docs/todo/README.md`。
- 禁止：修改其他 `docs/todo/**` 功能规划，不得把 PLANNED/NEEDS-REVIEW 标为 DONE。
- 验收：文档记录真实剩余数、白名单理由和验证证据；baseline 不含 stale allowance；若归零则门禁默认禁止所有新增调用。
- 升级条件：任何保留调用无法给出逐点理由、owner、生命周期和删除条件时，不得加入白名单。

### WP6：测试构建闭环

- 目标：依次执行编辑器诊断、架构门禁、Debug 全量构建、unit CTest；并行构建内存不足时以 `--parallel 1` 重试。
- target/tests：架构门禁 `ui_architecture_boundary_check`（并确认公共头门禁）；Debug target=`all`；CTest label=`unit`，`--output-on-failure`。
- 报告期望：报告路径、每阶段命令/退出码/通过数、问题日志新增条数、最终剩余 `UiRuntime::current()` 调用清单。
- 失败策略：仅可修复本工作包引入的构建脚本或测试笔误；业务/接口/生命周期问题写问题日志并退回对应代码工作包。
- 验收：全部阶段全绿；否则不得报告完成。

## 调度时间线

- 2026-08-27：重新读取架构门禁、架构评审和 TODO 索引，并搜索当前生产/测试调用。
- 2026-08-27：确认属于 Full PM Path；建立本协调记录。
- 2026-08-27：当前会话未提供子 agent 调度能力，WP1 无法派发，后续实现与验证按权限边界暂停。

## 待用户决策

- [ ] 启用可调度的“架构师、代码工厂、测试构建闭环”子 agent 后继续本轮编排。

## 结论

- 状态：阻塞
- 关键产物：`docs/pm/run-20260827-0000-p1-2-explicit-runtime-di.md`
- 下一工作包：WP1 当前调用分类、调用链与迁移规划
