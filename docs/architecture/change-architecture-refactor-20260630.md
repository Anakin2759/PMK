# VMP-ui 当前架构锐评与重构方案

> 日期：2026-06-30  
> 输入来源：用户请求“锐评当前架构，给出重构方案”；结合 `README.md`、`CMakeLists.txt`、`src/CMakeLists.txt`、边界检查脚本、既有规划文档与关键源码抽样。  
> 用户确认：目标是沉淀成可复用 UI 库；暂时无需顾虑 API 破坏；滞后的过渡层、旧入口、废弃实现可直接移除；需要支持同线程多 runtime、跨线程多 runtime、多窗口并行；需要跨平台截图回归。  
> 作用范围：`src/api`、`src/detail`、`src/common`、`src/core`、`src/systems`、`src/renderers`、`src/services`、`tests/unittest`、`tools/check_architecture_boundaries.py`、`docs/*PLAN.md`。

## 1. 一句话结论

当前架构不是“坏”，而是典型的练手项目进入框架化阶段后的中间态：功能堆得很快，边界开始补票，运行时正在从全局单例迁移到上下文门面，但抽象层仍混杂，部分模块名义上分层、实际互相知道太多。

由于目标已明确为“可复用 UI 库”，且需要同线程多 runtime、跨线程多 runtime、多窗口并行与跨平台截图回归，`RuntimeFacade::current()` 不能继续作为主设计。既然暂时无需顾虑 API 破坏，滞后的过渡层、旧入口、废弃实现应直接移除，而不是继续保留兼容包袱。

## 2. 架构锐评

### 2.1 好的部分

| 方面 | 评价 |
|---|---|
| 技术路线 | EnTT + SDL3 GPU + Yoga 的组合适合自研 UI 框架，方向清晰。 |
| API 体验 | Chain DSL 易用，和 ECS 内部模型解耦的目标正确。 |
| 构建治理 | CMake 选项、资源后端、shader 编译、架构边界检查已开始成体系。 |
| 错误处理 | `Result<T>` 与统一错误码方向正确，避免热路径异常扩散。 |
| 规划意识 | 已有事件边界、EnTT 私有化、主题、HiDPI、SVG、Rich Text 等规划文档，说明技术债可见。 |

### 2.2 尖锐问题

| 问题 | 当前表现 | 后果 |
|---|---|---|
| 分层还不稳 | `src/api/*.cpp`、`systems`、`renderers`、`services` 仍有大量 `RuntimeFacade::current()`、`entt::entity`、`raw()` 或内部实现直达。 | 新功能容易绕过边界，测试需要启动过多运行时上下文。 |
| `RuntimeFacade` 变成新全局点 | 它替代了旧 `Registry::current()`，但仍是 thread-local 全局入口。 | 多 runtime、多窗口、多线程测试隔离都会被隐式当前上下文拖累。 |
| `common` 语义过宽 | `common` 同时放公开类型、内部事件、组件、渲染类型、全局上下文。 | 公开/内部边界不直观，任何头文件都可能变成依赖扩散源。 |
| `detail` 过载 | `detail` 同时承担 API 桥接、实体转换、控件实现、查询、主题、事件等。 | 它会逐渐变成“第二个 src”，不是清晰的内部边界。 |
| System 职责偏胖 | `StateSystem`、`LayoutSystem`、`ThemeSystem` 已有明显多职责趋势。 | 后续加 Tooltip、Menu、Modal、焦点导航时会继续膨胀。 |
| Renderer 与 ECS 绑定重 | `IRenderer`/各 renderer 直接吃 `entt::entity` 和 registry。 | 渲染层难以做离屏测试、截图回归、不同后端替换。 |
| 控件能力与根能力倒挂 | DropDown、Table、TextEdit、DragDrop 等先有局部实现，Overlay、Focus、Command、Style 根能力后补。 | 每个控件会长出自己的小机制，后续统一成本增加。 |
| 架构门禁是“软门禁” | `tools/check_architecture_boundaries.py` 已有 baseline 思路，但仍允许不少关键债。 | 能防止继续变坏，但不能自然导向变好。 |

## 3. 改动类型判断

这是一次**跨模块架构重构规划**，但不建议做破坏性大重写。推荐拆成 4 类改动，并将 API 边界与多 runtime 作为硬约束：

1. **局部修改**：修 CMake 源清单、消除遗留 detail cpp、补测试。
2. **接口重塑**：不为旧 API 保兼容，收口公开事件、主题、实体句柄、创建入口。
3. **内部边界收敛**：把 EnTT、Runtime、Dispatcher、Registry 访问集中在少数边界模块。
4. **架构重构**：拆分胖 System，引入最小服务接口，但不引入大型框架。

硬约束：

- 公开 API 可破坏性调整，以稳定长期边界为优先；滞后接口直接删除，不做长期 deprecated 过渡。
- 所有新增 API 必须能绑定到明确的 `Application` / `UiRuntime` / `Window` 上下文，不依赖隐式全局当前运行时。
- `RuntimeFacade::current()` 从“推荐入口”降级为短期迁移支架；能直接替换的调用点不保留兼容入口。
- 多 runtime 下跨 runtime 操作必须可检测，不能静默落到 thread-local 当前 runtime。
- 同线程多 runtime 与跨线程多 runtime 都是硬需求；运行时归属、线程归属、异步投递边界必须显式建模。
- 跨平台截图回归是硬需求；渲染层需要尽早沉淀可测试的 RenderItem/DrawCommand 边界。

## 4. 推荐目标架构

### 4.1 分层目标

```text
include/ui.hpp
  ↓ 只暴露稳定公开 API、Result、公开事件/样式类型、runtime-aware 句柄
src/api
  ↓ 参数校验 + 用户 API；不暴露 EnTT；不直接写复杂 ECS 逻辑
src/detail
  ↓ API 到内部服务的桥接；只保留边界适配，不承载大业务
src/core
  ↓ UiRuntime、EventLoop、FrameContext、WindowLookup、RuntimeServices
src/services
  ↓ 可测试服务：EventBridge、OverlayManager、FocusManager、TextEditingService 等
src/systems
  ↓ 每帧执行系统；依赖显式注入的 Registry/Dispatcher/服务
src/renderers
  ↓ 渲染收集与绘制；逐步从 entity 直查过渡到 RenderItem/RenderContext
src/common/internal
  ↓ 内部组件、内部事件、上下文
src/api/types 或 src/common/public
  ↓ 公开轻量类型
```

### 4.2 关键原则

- `api/*.hpp` 不包含 EnTT、不暴露内部事件、不包含 `RuntimeFacade.hpp`。
- `api/*.cpp` 可以调用 `detail`，但复杂 ECS 操作下沉到 `detail` 或 `services`。
- `systems` 不再调用 `RuntimeFacade::current()`，构造时注入 `Registry&`、`Dispatcher&` 和必要服务。
- `RuntimeFacade::current()` 仅作为短期内部迁移支架；迁移完成的旧入口直接删除。
- 公开对象从“裸 `ui::entity` + 全局函数”转向“运行时绑定句柄”：例如 `Application`/`Window`/`Entity` 操作携带 runtime 归属。
- `common` 拆语义，不再当万能目录。
- 新控件必须先复用根能力：Style、Overlay、Focus、Command，避免各自造轮子。

### 4.3 Runtime-aware 句柄建议

长期公开 API 不应只依赖 `uint32_t` 实体 ID。建议保留 `ui::entity` 作为底层 ID，同时新增带归属的轻量句柄：

```text
EntityHandle = runtime token + entity id
WindowHandle = runtime token + window entity id + platform window id
Application  = owns UiRuntime / EventLoop / services
```

推荐方向：

- `ui::entity`：底层 ID，主要用于序列化、调试、低层查询。
- `ui::Entity`：公开操作句柄，携带 runtime 归属。
- `ui::Window`：窗口句柄，明确多窗口归属。
- `ui::Application`：运行时所有者，不再只靠 `factory::CreateApplication()` 后全局创建实体。

## 5. 修改规划表

| 阶段 | 优先级 | 工作包 | 主要文件范围 | 验收标准 | 风险 |
|---|---:|---|---|---|---|
| P0 | 最高 | 边界盘点与门禁升级 | `tools/check_architecture_boundaries.py`、`src/api`、`src/common`、`src/core` | 新增规则禁止新增 `RuntimeFacade::current()`、`raw()`、公开头 EnTT；baseline 只减不增；已替换的旧入口直接删除。 | 可能先暴露大量历史债，需要精确 baseline。 |
| P0 | 最高 | 公开/内部类型切分 | `src/common/EntityTypes.hpp`、`src/common/Events.hpp`、`src/detail/EntityCast.hpp`、`src/api/Event.*` | `include/ui.hpp` 独立编译；公开 API 无 EnTT；事件 payload 使用公开句柄或 `ui::entity`。 | 系统层需要大量转换，容易漏。 |
| P0 | 最高 | 多 runtime 句柄契约 | `src/api/Entity.hpp`、`src/api/Factory.*`、`src/core/UiRuntime.*`、`src/detail/EntityCast.hpp` | 实体句柄可校验 runtime 归属；跨 runtime 操作失败而不是误操作；旧裸实体主 API 移除。 | 会破坏现有只传 `uint32_t` 的 API，但已接受。 |
| P0 | 最高 | 跨线程 runtime 边界 | `src/core/UiRuntime.*`、`src/core/WorkerMailbox.*`、`src/core/EventLoop.*`、`src/core/SystemManager.*` | runtime 明确 owner thread；跨线程操作必须经 mailbox/event loop 投递；禁止直接跨线程访问 registry。 | 需要梳理现有 worker 与主线程调用路径。 |
| P0 | 高 | Runtime 访问收敛 | `src/core/RuntimeFacade.*`、`src/core/UiRuntime.*`、`src/systems/*`、`src/services/*` | 新增/改动系统不直接使用 `RuntimeFacade::current()`；SystemManager 统一注入依赖。 | 一次性全改成本高，应分系统迁移。 |
| P1 | 高 | 根能力优先：Style/Theme | `src/common/Theme.hpp`、`src/api/Theme.*`、`src/detail/Theme.*`、`src/systems/ThemeSystem.*` | Button/Label/TextEdit/CheckBox/DropDown/Slider/ProgressBar 默认样式来自 theme token，运行时切主题可触发脏标记。 | 旧链式样式与主题优先级需定契约。 |
| P1 | 高 | 根能力优先：Overlay/Popup | 新增 `src/services/OverlayManager.*` 或 `src/systems/OverlaySystem.*`，改 `DropDown` | DropDown 改用统一浮层；支持 z-order、外部点击关闭、焦点恢复。 | 会影响输入命中和层级树。 |
| P1 | 高 | 根能力优先：Focus/Keyboard | 新增 `FocusManager`/`FocusSystem`，改 `StateSystem`、`TextEditingService`、`ShortcutSystem` | Tab/Shift+Tab、禁用/隐藏跳过、焦点环、TextEdit/Button 行为有测试。 | 现有 StateSystem 逻辑耦合，需要拆。 |
| P1 | 中 | 拆胖 System | `StateSystem.*`、`LayoutSystem.*`、`ThemeSystem.*` | 输入状态、滚动条、拖放、焦点、主题应用各自职责明确。 | 过度拆分会增加跳转成本，按真实变化原因拆。 |
| P1 | 高 | 渲染数据边界与截图回归基础 | `interface/IRenderer.hpp`、`renderers/*`、`systems/render/*`、`tests` | Renderer 收集 `RenderItem`/`DrawCommand`，减少直接 registry 查询；建立跨平台截图回归容忍策略。 | 改动面大，但截图回归已是硬需求，应前置。 |
| P2 | 中 | 控件补齐路线 | `Factory.*`、`Controls.*`、`renderers/*`、`tests/unittest` | Radio/Switch/ListView/Tooltip/ContextMenu/Modal 复用根能力，有工厂、DSL、测试、示例。 | 不能在根能力前抢跑。 |
| P1 | 高 | 测试策略升级 | `tests/unittest`、新增交互/截图回归工具 | 架构边界、公开头、主题、Overlay、Focus、多 runtime、关键交互、截图回归有稳定测试。 | 截图测试在多平台像素差异上需容忍策略。 |

## 6. 建议里程碑

### M1：止血，不让债继续长（1-2 周）

- 升级架构边界脚本，新增规则默认拦截：
  - `systems/renderers/services` 新增 `RuntimeFacade::current()`。
  - `src/api/**/*.hpp` 新增 EnTT 或内部 Runtime include。
  - 新增 `.raw()` 访问。
- 清理 `src/detail/*.cpp` 未进入 `UI_SOURCES` 的历史残留，确认是废弃、遗漏还是待迁移。
- 对确认废弃的 API/文件不做 deprecated 保留，直接删除或从 umbrella header 移除。
- 给 `include/ui.hpp`、公开事件 API、`ui::entity` 加边界测试。
- 冻结新增裸全局 API：新 API 必须显式说明 runtime/window 归属。

### M2：API 破坏性收口与多 runtime 契约（2-4 周）

- 设计新的公开句柄：`Entity`/`Window`/`Application` 或等价结构，至少携带实体 ID 与 runtime token。
- `ui::entity = uint32_t` 可作为底层 ID 保留，但不再作为长期唯一公开操作载体。
- `Factory` 从隐式当前 runtime 创建，迁移为基于 `Application`/`UiRuntime`/`Window` 的创建入口。
- 移除隐式当前 runtime 的创建入口；示例和测试同步迁移，不提供双轨兼容。
- 跨 runtime `AddChild`、`Show`、`OnClick`、`Query` 等操作必须返回错误或断言失败，不能静默访问当前 runtime。
- runtime 明确 owner thread；跨线程访问通过 `WorkerMailbox` / `EventLoop` 投递，禁止直接跨线程访问 `Registry`。
- 更新 `example/ui_demo`，让示例成为新 API 的规范样板。

### M3：运行时依赖显式化（2-4 周）

- `SystemManager` 统一负责构造并注入 `Registry&`、`Dispatcher&`、必要服务。
- 从低风险系统开始迁移：`TimerSystem`、`ThemeSystem`、`HitTestSystem`。
- 每迁一个系统，边界脚本 baseline 减一项。

### M4：根能力落地（4-8 周）

- 完成最小 Theme/Style 优先级契约：用户链式设置 > 局部样式 > 状态样式 > theme 默认值。
- 引入 OverlayManager，先接管 DropDown，再支撑 Tooltip/ContextMenu/Modal。
- 引入 FocusManager，接入 TextEdit、Button、CheckBox、DropDown、Shortcut。

### M5：截图回归与渲染收敛（8 周+）

- Renderer 逐步改成收集渲染数据，降低对 ECS 的实时查询耦合。
- 建立跨平台截图回归：基准图、容忍阈值、字体/DPI 固定策略、失败差异图输出。
- 增加交互录制和性能基准。

### M6：控件补齐（8 周+）

- 按根能力补控件：Radio/Switch → Tooltip/ContextMenu → Modal → ListView/TreeView。
- 新控件必须纳入截图回归和交互回归。

## 7. SOLID 检查

| 原则 | 当前风险 | 重构约束 |
|---|---|---|
| SRP | `StateSystem`、`detail`、`common` 职责过宽。 | 按变化原因拆：输入状态、焦点、浮层、主题、文本编辑分开。 |
| OCP | 新控件常需改多个稳定核心。 | 提前固定 Style/Overlay/Focus 扩展点，控件按扩展接入。 |
| LSP | 当前继承层少，主要是 renderer/system 接口。 | 新 renderer/system 不应要求调用方知道具体控件类型。 |
| ISP | 服务接口暂少，`RuntimeFacade` 偏胖。 | 服务接口按使用方拆，不造万能 `IUiServices`。 |
| DIP | 系统依赖全局门面而不是抽象/注入。 | 高层系统依赖注入的 Registry/Dispatcher/服务，保留门面作兼容层。 |

## 8. 不建议做的事

- 不建议推倒重写。现有主链路能跑，应该渐进替换边界。
- 不建议引入 DDD、微服务式分层或大型 IoC 容器。项目是 UI 静态库，重型框架收益低。
- 不建议先补一堆控件。没有 Overlay/Focus/Style，控件越多债越多。
- 不建议把所有 EnTT 都藏起来。内部 ECS 层可以用 EnTT，目标是“不泄漏到公开 API”和“访问路径可控”。
- 不建议把 `RuntimeFacade` 作为公开或长期内部入口保留。能同步迁移的调用点应直接替换；仅对无法一次切完的系统短期保留支架。
- 不建议继续扩展只依赖裸 `ui::entity` 的公开 API。既然接受破坏性调整，应尽快引入 runtime 归属语义。

## 9. 可派发给代码工厂的任务清单

| 任务 | 优先级 | 依赖 | 文件范围 |
|---|---:|---|---|
| 增强架构边界脚本并更新 baseline | P0 | 无 | `tools/check_architecture_boundaries.py` |
| 增加公开头与 EnTT 隔离测试 | P0 | 边界规则 | `tests/unittest/test_UmbrellaHeader.cpp`、新增测试 |
| 设计并落地 runtime-aware 公开句柄 | P0 | API 破坏性调整已接受 | `src/api/Entity.hpp`、`src/api/Factory.*`、`src/core/UiRuntime.*` |
| 明确跨线程 runtime 访问模型 | P0 | 多 runtime 两类场景都需要 | `src/core/UiRuntime.*`、`src/core/WorkerMailbox.*`、`src/core/EventLoop.*` |
| 将 Factory 迁移到显式 runtime/window 创建入口 | P0 | runtime-aware 句柄 | `src/api/Factory.*`、`src/detail/Factory.*`、示例 |
| 删除滞后旧 API 与未接入实现 | P0 | 新入口可用或确认废弃 | `src/api`、`src/detail`、`include/ui.hpp`、`src/CMakeLists.txt` |
| 迁移 `TimerSystem` 去 `RuntimeFacade::current()` | P0 | System 注入路径确认 | `src/systems/TimerSystem.*`、`src/core/SystemManager.*` |
| 迁移 `ThemeSystem` 去 `RuntimeFacade::current()` | P0 | 同上 | `src/systems/ThemeSystem.*` |
| 定义 Style 优先级契约 | P1 | Theme 现状确认 | `src/common/Theme.hpp`、`src/api/Theme.hpp`、文档 |
| 实现最小 OverlayManager 并接管 DropDown | P1 | 输入命中规则确认 | `src/services`、`src/systems/StateSystem.*`、`src/api/Factory.*` |
| 实现最小 FocusManager | P1 | Overlay 后或并行 | `src/services`、`src/systems/StateSystem.*`、`TextEditingService.*` |
| 拆分 StateSystem 滚动条/拖放/焦点职责 | P1 | Focus 方案 | `src/systems/StateSystem.*` |
| 建立截图回归基础设施 | P1 | 渲染边界方案 | `tests`、`tools`、`systems/render`、`renderers` |

## 10. 风险与验证建议

| 风险 | 缓解 | 验证 |
|---|---|---|
| 边界收敛导致大量编译错误 | 每次只迁一个系统，保持 adapter。 | `cmake --build build --config Debug`、`ui_architecture_boundary_check`。 |
| 破坏性 API 迁移影响示例和用户代码 | 0.x 阶段集中完成；不保旧入口，只保留迁移说明。 | `example/ui_demo` 全量迁移，删除旧 API 后全量构建。 |
| 多 runtime/thread-local 行为回归 | Runtime 访问迁移期间保留兼容门面，但新增 API 不依赖它。 | `test_UiRuntime.cpp` 扩充多 runtime 隔离、跨 runtime 误用测试。 |
| 跨线程 runtime 误用 | 所有跨线程写 UI 状态必须投递到 owner thread。 | 增加跨线程投递与非法直接访问测试。 |
| 主题优先级破坏旧 DSL 行为 | 明确用户显式链式设置最高优先级。 | Button/TextEdit/DropDown/Slider 主题回归测试。 |
| Overlay 改造破坏 DropDown | 先只接管 DropDown，不同时做 Tooltip/Menu。 | 外部点击关闭、z-order、焦点恢复、销毁时清理测试。 |
| Focus 改造影响 TextEdit | TextEdit 作为焦点系统验收样本优先覆盖。 | Tab、Shift+Tab、输入、快捷键冲突测试。 |
| 截图回归跨平台不稳定 | 固定字体、DPI、窗口尺寸、渲染后端；使用阈值与差异图，不追求逐像素完全一致。 | Windows/Linux 基准图各自维护，CI 输出 diff artifact。 |

## 11. 待确认问题

暂无。当前关键方向已确认：可复用 UI 库、允许破坏性 API、直接移除滞后旧物、同线程与跨线程多 runtime、多窗口并行、跨平台截图回归。
