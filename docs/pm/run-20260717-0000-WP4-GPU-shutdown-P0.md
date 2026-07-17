# 项目经理协调记录 - WP4 GPU shutdown 最小 P0

- 时间：2026-07-17
- 输入来源：用户指定批次；`docs/architecture/ARCHITECTURE_REVIEW_AND_ROADMAP_2026-07-11.md` 的 WP4 GPU shutdown
- 本轮范围：仅编排 Application 销毁顺序、SDL 初始化后的构造失败回滚及可测试契约；不实施源码，不执行构建
- 验收标准：`SystemManager`（含 `RenderSystem`/GPU 资源）在 `SDL_Quit()` 前被确定性销毁；`SDL_Init()` 成功后任一后续构造步骤抛异常时恰好执行一次 SDL 回滚；测试可稳定观测顺序与次数

## 工作包
| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | 风险与最小范围核验 | 项目经理（只读） | 已核验风险、路线图 WP4、Application/SystemManager 生命周期 | 本记录的实施规格 | 完成 |
| 2 | Application 生命周期 P0 落地 | 主 Agent | 本记录“工作包 #2” | 私有生命周期协调器与 Application 销毁顺序修复 | 完成 |
| 3 | 定向测试闭环 | 主 Agent | 工作包 #2 交付报告 | Debug 构建与定向/全量测试结果 | 完成 |

## 工作包 #2：实施规格

- **目标**：在 `ApplicationImpl` 内建立显式、异常安全且可测试的 SDL 所有权边界；正常析构先断开事件/注销处理器，再销毁 `m_systems`，最后调用 `SDL_Quit()`；构造阶段仅在 `SDL_Init()` 成功后取得 SDL 所有权，后续任意异常自动回滚。
- **依据**：当前 `ApplicationImpl::~ApplicationImpl()` 在析构体内调用 `SDL_Quit()`，而 `m_systems` 是成员，析构体结束后才自动析构；当前 `SDL_Init()` 后的上下文创建、handler 注册和 event-loop 配置均可能抛出，但无 SDL 回滚。
- **只允许触达（最小集合）**：
  - `src/core/Application.cpp`
  - `tests/unittest/test_ApplicationLifecycle.cpp`（新增）
  - `tests/unittest/CMakeLists.txt`
- **条件允许文件**：若生产代码无法在不暴露测试钩子的前提下稳定验证 SDL 次数/顺序，可新增一个仅供内部生命周期实现使用的私有头 `src/core/ApplicationLifecycle.hpp`；不得成为 public API。
- **禁止文件/事项**：`src/systems/render/**`、`src/systems/RenderSystem.hpp`、renderer/managers、`SystemManager` 实现、公开 `include/`、根/`src` CMake、完整 GPU API 注入、SDL 全局封装重构、WP7 拆分、新第三方依赖、无关格式化或重命名。
- **实现约束**：
  1. SDL 所有权必须使用局部/成员 RAII 表达；仅 `SDL_Init()` 成功后 armed，释放后 disarm，避免构造失败与正常析构双重 `SDL_Quit()`。
  2. 正常销毁顺序必须是：停止 Application 自有回调连接 → `unregisterAllHandlers()` → 显式销毁 `m_systems`（触发 `RenderSystem`/GPU cleanup）→ `SDL_Quit()`；`m_runtimeScope` 与 `m_runtime` 在上述过程保持有效。
  3. `m_systems` 销毁后不得再解引用；析构保持 `noexcept`。单个清理步骤异常不得导致 SDL 回滚丢失；不得以 catch 包围全部步骤后直接跳过后续关键清理。
  4. 不改变 Application 公共 API，不把 SDL 函数表或 GPU backend 注入到业务公共接口。
  5. 测试缝应最窄化：优先测试私有生命周期 guard/cleanup 协调契约，不启动真实窗口/GPU；禁止为测试引入完整 SDL/GPU mock 框架。
- **可测试契约**：
  - init 失败：不调用 quit；异常向上传播。
  - init 成功、后续步骤抛出：调用 quit 恰好一次。
  - 正常释放：systems-destroy 发生在 SDL quit 之前，quit 恰好一次。
  - 重复调用内部 cleanup/guard reset（若接口允许）：不重复销毁 systems，不重复 quit。
  - 某个前置注销步骤抛异常（若可注入验证）：仍继续 systems-destroy 与 SDL quit，析构不抛出。
- **验收标准**：
  - 静态检查可明确读出 `m_systems.reset()` 先于 SDL guard/reset；不依赖成员自动析构顺序碰巧正确。
  - 新测试不依赖实际 GPU、显示器或窗口，覆盖上述至少前三项核心契约；若实现支持独立幂等 cleanup，再覆盖重复调用。
  - `ui` 与 `ui_unit_tests` target 可编译；生命周期新增测试通过；既有 `ui_unit_tests` 无回归。
  - 交付报告必须列出实际变更文件，并说明异常路径如何保证 SDL 恰好回滚一次。
- **失败后的升级条件**：若稳定测试必须改公开 `Application` API、修改 `SystemManager`/RenderSystem/renderer、注入完整 SDL/GPU API、改变多 Application 的全局 SDL 引用语义，或发现 `RenderSystem` 清理依赖已销毁的 runtime/scope，则停止扩展并回报用户/上游规划，不在 P0 内自行决策。

## 工作包 #3：验证规格

- **目标**：验证最小生命周期修复，不扩展到 WP4 全量压力验收。
- **target/tests**：Debug；构建 `ui`、`ui_unit_tests`；运行新增 Application lifecycle 测试及全量 `ui_unit_tests`。
- **报告期望**：返回报告路径、各阶段结果、问题日志新增条数；不得把“未运行”写成通过。
- **失败策略**：仅可修复本批测试/CMake 接线笔误；业务生命周期或接口问题写入问题日志并升级。
- **本批明确不验收**：100 次真实窗口创建/销毁、统一 `RenderResourceContext::Shutdown()`、主动泄漏分支、完整 GPU 故障注入；这些仍属于 WP4 后续批次。

## 主要风险

1. `m_systems.reset()` 会触发 `SystemManager::~SystemManager()` 再次注销；应依赖其现有状态机幂等语义，不顺手改 SystemManager。
2. 单一大 `try` 会在前置清理抛出后跳过 systems 销毁和 SDL quit；需逐步 best-effort 清理或等价 RAII 保证。
3. 直接构造完整 `Application` 做异常测试可能启动真实 GPU/窗口，造成 CI 不稳定；测试应聚焦所有权 guard 与顺序协调契约。
4. SDL 是进程全局状态；本批不定义多 Application 并存/引用计数语义，测试必须隔离且不并行干扰。

## 调度时间线
- 2026-07-17：按 Full PM 路径只读核验路线图、`ApplicationImpl`、`SystemManager` 与测试 target；形成最小 P0 文件范围和契约。
- 2026-07-17：未修改源码，未执行构建或测试。
- 2026-07-17：主 Agent 新增 `ApplicationLifecycle.hpp`，SDL 初始化成功后 armed；正常关闭显式执行 systems reset 后 quit，构造失败展开保持同一顺序。
- 2026-07-17：Debug 构建和架构门禁通过；Application 生命周期/Runtime/SystemManager 定向测试 13/13，全量测试 149/149。

## 待用户决策
- [ ] 无；仅在触发升级条件时请求范围或架构决策。

## 结论
- 状态：completed（仅 WP4 最小 P0 批次）
- 关键产物：`src/core/ApplicationLifecycle.hpp`、Application 显式销毁顺序、`test_ApplicationLifecycle.cpp`
- 下一工作包：处理 RenderSystem/device 切换后的旧 device deleter 与 manager 主动泄漏分支；该后续需要窄 GPU API 测试缝
