# VMP-ui 架构锐评与优化路线

> 日期：2026-08-13
> 性质：只读评审，不改动任何源码/测试/CMake/既有文档
> 依据：`.github/copilot-instructions.md`、`src/**`、`include/ui/**`、`docs/todo/**`、`docs/architecture/**`、`docs/pm/**`、`CMakeLists.txt`、`src/CMakeLists.txt`、`tests/**`、`logs/**` 的实读
> 关联：`UI_FRAMEWORK_GAP_ANALYSIS_2026-05.md`、`THREADPOOL_DECISION_2026-08-09.md`、`run-20260809-P0-threadpool-retain-hardening.md`、`CPU_RENDER_ISSUES.md`

---

## 0. 一句话结论

**架构方向是对的（ECS + DI + 自包含发行 + 现代 C++23），但当前处于"demo 级能力 + 工程级骨架"的割裂状态：骨架过度设计、文档与代码脱节、渲染双路径和并发路径埋着被"自愈"掩盖的债；下一步不应再堆新控件，而应先把正确性、测试与文档对齐这三条线收口，再以 Theme/Overlay/Focus 三项根能力承接后续控件。**

---

## 1. 总体评级

| 维度 | 评分（1-5） | 一句话评语 |
|---|---|---|
| 架构方向 | ★★★★☆ | ECS + `UiRuntime` 注入 + CRTP System 注册，方向正确且克制 |
| 公共 API 设计 | ★★★★☆ | 管道 DSL + `Result<T>` + 自包含公共头，是整库最亮眼的部分 |
| 自包含发行 | ★★★★★ | 单 `VMPUI.lib`、消费者零第三方依赖，方案成熟 |
| 渲染管线 | ★★★☆☆ | GPU SDF 路径精致，CPU fallback 路径 4 处静默降级 BUG |
| 并发/事件循环 | ★★☆☆☆ | 无锁 MPSC + 帧调度器尚可，但 lost-wakeup 被节流自愈掩盖，历史反复 |
| 组件/能力覆盖 | ★★☆☆☆ | 基础控件可用，Theme/Overlay/Focus 根能力缺失，多个半成品悬空 |
| 测试 | ★★☆☆☆ | 有 209 项，但发现机制、环境属性、回归覆盖均有缺口 |
| 文档-代码一致性 | ★☆☆☆☆ | 多处注释/指引与实现直接矛盾（ASIO、单例、文件路径） |
| 工程卫生 | ★★☆☆☆ | `logs/` 堆积 30+ 调试产物，残留 PestManKill 品牌，引用不存在的文档 |

---

## 2. 锐评：架构债务与风险

### 2.1 渲染双路径割裂：GPU 精致、CPU 静默降级（最优先）

GPU 路径用 HLSL SDF 着色器在片元级计算圆角/阴影/描边/纹理，`Vertex` 携带完整 SDF 参数，设计是好的。但 `FallbackBackendRenderer`（`src/renderers/FallbackBackendRenderer.hpp`）走 SDL_Renderer 时：

- **BUG-1** 圆角：只读顶点 AABB + 首顶点颜色，`radius[4]` 从不读取 → 所有圆角退化为直角。
- **BUG-2** 图片：`batch.texture != whiteTextureTag` 时直接 `return` 丢弃整批，**无任何警告日志**。
- **BUG-3** Canvas 圆形：`pushConstant.radius`（批次级）被忽略，圆形退化为矩形。
- **BUG-4** 抗锯齿：线段用矩形近似，斜线明显锯齿。

核心问题不是"CPU 路径比 GPU 差"——CPU 降级本身合理——而是**"静默丢弃参数 + 无日志 + 无告警"的反模式**。用户在无 GPU 机器上会看到"控件还在但样式全错"，且无法从日志定位。这属于"silent degradation"，比直接崩溃更难排查。（详见 `docs/todo/CPU_RENDER_ISSUES.md`，方案 A~D 已起草，尚未落地。）

### 2.2 并发/事件循环：正确性被节流"自愈"掩盖

- `src/utils/EventLoop.hpp` 是无锁 MPSC 有界队列 + condition_variable 通知，`src/core/EventLoop.hpp` 再包一层 `std::jthread` 帧调度器。
- `THREADPOOL_DECISION_2026-08-09.md` 已证实：`Post/WaitForWork` 存在同类 lost-wakeup，但被 16ms 帧节流自愈掩盖，用户从未在可观测层面复现。
- 历史反复：先"理论风险 → 加锁通知 → 违背无锁承诺 → 用户要求还原 → 确立'无实测不改变同步结构'门槛"。
- 结论是理性的：**并发正确性目前靠"16ms 兜底轮询"而非"事件驱动唤醒"保证**，这本身是一种隐性耦合——一旦未来帧循环改事件驱动（见 P1-4），lost-wakeup 会立刻浮出水面。两条路必须绑定推进。

已消化的历史债（记录在案，防止复发）：~1000 行无人消费的 `ThreadPool/MpmcQueue/WorkStealingDeque/Singleton` 死代码、`test_ThreadPool.cpp` 悬空引用导致 `ENABLE_BUILD_TESTS=ON` 时 configure 失败、`UI_ENABLE_MULTITHREAD` 宏名不副实。这些已移除，是好结果，但**揭示了一个系统性倾向：先建基础设施、后找使用场景**。

### 2.3 文档-代码严重脱节（P0-F 已点名，仍未收敛）

| 位置 | 文档/注释写的 | 实际 |
|---|---|---|
| `.github/copilot-instructions.md` | Chains 定义在 `src/api/Chains.hpp` | 实际在 `include/ui/api/Chains.hpp` |
| `.github/copilot-instructions.md` | "不使用 ASIO" | `src/core/EventLoop.hpp` 头注释仍写"基于ASIO实现" |
| `src/core/EventLoop.hpp` 头注释 | "1ms间隔轮询事件" | 实际 16ms 节流投递帧回调 |
| `src/utils/Registry.hpp` 头注释 | "全局实体注册表单例" | 实际已改 DI 注入、非全局单例 |
| `run-20260809-...md` | 引用 `docs/architecture/ARCHITECTURE_REVIEW_2026-08-09.md` | **该文件不存在**（workspace 内检索为空） |

文档是给下一个维护者（和 Copilot 自身）的第一印象。指引文件里"ASIO/单例/路径"三处硬伤，会直接误导代码生成与新人上手。这是 P0-F 工作包明列、却因"调度失败×2"而挂起的项，优先级应上调。

### 2.4 能力缺口：根能力缺失导致控件各自为战

`UI_FRAMEWORK_GAP_ANALYSIS_2026-05.md` 已系统盘点，这里只强调**结构性结论**：

- **根能力缺三块**：Theme/Style（`ThemeSystem` 是 stub）、Overlay/Popup（DropDown 自己造 popup）、Focus/键盘导航（无 Tab 序/焦点环/焦点陷阱）。
- **半成品悬空**：`ListArea`、`Menu`、`Calendar`、`Dialog`、`Draggable/Droppable` 有组件/tag 但缺工厂/renderer/契约，长期悬空比不建更危险（误导 + 维护税）。
- 这会导致后续每个新控件（Tooltip/Menu/Tab/Modal/ComboBox）都各自实现一套浮层/焦点逻辑，债滚债。

### 2.5 测试债务：数量够、质量与机制有缺口

- 209 项中 1 项失败：`FallbackWindowLifecycleTest` 因 `gtest_discover_tests` 未生成 `ENVIRONMENT=SDL_VIDEODRIVER=offscreen` 属性而用真实 windows driver（`problem-log-wp4a`）。这是 CMake 边界外问题，被记为"待人工"。
- `test_ThreadPool.cpp` 悬空引用曾致 configure 失败（已随 ThreadPool 移除修复，但暴露了"引用不存在文件"无前置校验）。
- VS Code 测试适配器无法发现 GTest 用例（`problem-log-wp4b`），只能靠 CTest 清单。
- 缺截图回归、交互录制、性能基准（`TEST_MODULE_REVIEW_PLAN`、gap analysis §5 均点名）。

### 2.6 组织/命名/边界残留

- **模块归属未决**：`Registry/Dispatcher/Logger` 在 `src/utils/`，但语义属 core，`UiRuntime` 头注释还残留"utils"心智（pm run Q8 未决）。`utils/` 当前同时装着通用队列（`MpscQueue/EventLoop`）和 UI 领域对象（`Registry/Dispatcher/Logger`），边界不清。
- **品牌/耦合残留**：源码头注释与 `logs/*.log` 仍带 "PestManKill" 前缀（`ThreadPool/SystemManager/FontManager` 等日志行），独立拆分不彻底。
- **工程卫生**：`logs/` 下堆积 30+ 个 `hang-*` / `table-drag-*` / `auto-repro-*` 调试文件，未清理、未见 `.gitignore` 治理策略。`hang-fixed.err` 里还有 "Window already unclaimed!" 这类窗口生命周期告警，与 WP4 GPU 生命周期线相关。

### 2.7 CMake 认知负荷高（虽设计合理）

7 个 OBJECT 库 + 模块级 `UI_POOL_*` + 双资源后端（CMRC/STD_EMBED）+ DXC 三态（AUTO/ON/OFF）+ LTO 约束（合并库禁 LTO）——**每一条都有正当理由**（模板编译内存、自包含发行、着色器可复现），但叠加后 configure 面相当复杂。`compile_commands.json`、`build_build_log.txt` 等散落仓库根目录，也说明构建产物/日志管理边界不清。

---

## 3. 应保留的亮点（不要因锐评而误伤）

1. **自包含发行**：单 `VMPUI.lib` 合并 SDL3/yoga/freetype/harfbuzz，消费者 `find_package + link` 即插即用，平台库经 `$<LINK_ONLY>` 带出。这是整个项目最有工程价值的资产。
2. **管道 DSL + `AnyChain` 类型擦除**：`entity | Size(...) | BackgroundColor(...) | OnClick(...)` 就地内联零开销，跨模块用 concept-model 擦除，设计干净。
3. **`Result<T>` 统一错误处理**：`Err/Ok/TRY/TRY_VOID` + `std::source_location` 自动捕获，20 个错误码段位预留，克制且一致。
4. **ECS + DI**：`Registry` 内部持 `entt::registry`，经 `UiRuntime` 注入 System，非全局单例；`EnableRegister<Derived>` CRTP 注册。方向正确。
5. **架构门禁**：`check_architecture_boundaries.py` 与公共头自包含检查作为默认构建目标，边界债务会直接 build fail。
6. **`UiRuntime::current()` 硬化**：debug `assert` + release `std::terminate`，替代此前 251 处裸解引用 UB 扩散面（P0-D 已落地）。

---

## 4. 优化路线

原则：**先正确性，再根能力，后控件，最后生态**。每阶段可独立验收、独立合入。

```mermaid
flowchart LR
  P0[P0 止血与正确性<br/>1-2周] --> P1[P1 根能力<br/>3-6周]
  P1 --> P2[P2 控件与数据<br/>1-2月]
  P2 --> P3[P3 生态与规模化<br/>持续]
  P0 -.并发实测证据.-> P1
  P1 -.Overlay/Focus 就绪.-> P2
```

### P0 — 正确性与止血（1-2 周）

| 工作包 | 目标 | 关键产物/验收 | 风险 |
|---|---|---|---|
| **P0-1** CPU fallback 渲染修复 | 关闭 4 个静默降级 BUG | 圆角/图片/Canvas 圆/抗锯齿在 `UI_FORCE_CPU_RENDER=ON` 下可回归；丢弃参数时必打 warning | 低；`CPU_RENDER_ISSUES.md` 方案 A~D 已起草 |
| **P0-2** 测试发现机制修复 | 恢复标准 CTest 全绿 | `gtest_discover_tests` 补 `ENVIRONMENT/LABELS/RUN_SERIAL/TIMEOUT`；`ENABLE_BUILD_TESTS=ON` configure 通过；加入"引用文件存在性"前置校验 | 低 |
| **P0-3** 文档-代码对齐 | 消除 §2.3 全部矛盾 | 修 `EventLoop.hpp`/`Registry.hpp` 头注释、copilot-instructions 的 Chains 路径；补建或删除 `ARCHITECTURE_REVIEW_2026-08-09.md` 引用 | 低；属 P0-F 收口 |
| **P0-4** EventLoop lost-wakeup 实测 | 只加观测，不改同步结构 | 压力测试（线程数/任务数/超时/复现日志）落盘；未复现则仅记录 | 中；严守"无实测不改无锁"门槛 |
| **P0-5** 工程卫生 | 清账 | `logs/` 调试产物清理 + `.gitignore` 治理；PestManKill 品牌残留清单 | 低 |
| **P0-6** 待决 Q 项收敛 | 消除悬空决策 | Q1.1(Singleton)、Q5(守卫脚本)、Q8(utils→core) 出结论 | 低；需用户拍板 |

### P1 — 根能力（3-6 周）

| 工作包 | 目标 | 依赖 | 验收 |
|---|---|---|---|
| **P1-1** Theme/Style System | 主题 token、状态样式、默认皮肤、运行时切主题触发 RenderDirty | 无 | Button/Label/TextEdit/CheckBox/DropDown 能从统一 token 取色取尺寸；切主题有测试 |
| **P1-2** Overlay/Popup Manager | 统一浮层栈、定位、外部点击关闭、焦点恢复、z-order | 无 | DropDown 改走统一 popup；Tooltip/Menu 复用；关闭/焦点有测试 |
| **P1-3** Focus/键盘导航 | Tab 序、方向键、焦点环、焦点陷阱、Disabled 跳过 | 无 | Tab/Shift+Tab 遍历 + TextEdit/Button 焦点行为有测试 |
| **P1-4** 帧循环事件驱动化 | 16ms 轮询 → 事件驱动唤醒 | **P0-4 实测结论** | 帧空闲 CPU 归零；输入延迟不退化；唤醒路径有压测 |
| **P1-5** 测试策略落地 | 截图回归 + 交互录制 + 性能基准骨架 | P0-2 | 基准脚本可跑；视觉回归接 CI |

> 注意：P1-4 必须与 P0-4 绑定。当前 lost-wakeup 靠 16ms 节流掩盖，一旦改事件驱动，丢失唤醒会立刻暴露——顺序反了就制造回归。

### P2 — 控件与数据能力（1-2 月）

| 工作包 | 内容 | 依赖 |
|---|---|---|
| **P2-1** 基础输入控件 | RadioGroup/Switch/TabView/ListView | P1-1/P1-3 |
| **P2-2** 弹出类控件 | Tooltip/ContextMenu/ModalDialog | P1-2 |
| **P2-3** 数据能力 | VirtualList、TreeView、Table 排序/筛选/列调整、数据源绑定 | P2-1 |
| **P2-4** 动画完整化 | timeline/sequence/parallel、暂停恢复、完成回调、取消策略 | 无 |
| **P2-5** 半成品收口 | ListArea/Menu/Calendar/Dialog/拖放 补齐契约与测试 | P1-2/P1-3 |
| **P2-6** 纯 CPU 并发 | 图片/字体/SVG 解码经专用 worker + `EventLoop::Post` 回主线程 | **需先有 benchmark 证明帧预算被 CPU 占用** |

### P3 — 生态与规模化（持续）

Accessibility 语义层 → I18N/RTL/bidi → UI Inspector/实体树/布局边框可视化 → 外部 DSL（以正式模块引入，不再留假入口）→ 视觉回归 CI 常态化。

---

## 5. 风险清单（评审者视角的"如果只改一件事"）

1. **最高优先**：CPU fallback 的静默降级（P0-1）——它是"用户可见、日志无痕"的正确性缺陷，且方案已备好，只欠落地。
2. **最易误判**：并发——不要在没有可重复复现前改动无锁路径；正确动作是加观测，不是加锁。
3. **最影响演进**：文档脱节（P0-3）——它会持续污染 Copilot 与新人判断，成本低、收益高，应优先收口。
4. **最需用户决策**：P0-6 的三个悬空 Q 项，阻塞了 utils/core 边界与守卫脚本的长期走向。

---

## 6. 附：评审期间发现的额外小项（非阻塞）

- 根目录散落 `_fix_mojibake.py`、`build_build_log.txt`，应归入 `tools/` 或 `.gitignore`。
- `src/core/EventLoop.hpp` 文件注释署名 `EventLoop.h`（头注释文件名为 `.h`，实际 `.hpp`），属轻微不一致。
- `std::function`（`EventLoop::ExceptionHandler`）与项目"优先 `std::move_only_function`"约定不一致，可顺手统一。
