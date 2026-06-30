# VMP-ui 架构评估

> 日期：2026-06-30  
> 输入来源：用户请求“评估架构”；结合 `README.md`、根 `CMakeLists.txt`、`src/CMakeLists.txt`、`tools/check_architecture_boundaries.py`、既有 `docs/architecture/change-architecture-refactor-20260630.md` 与关键源码抽样。  
> 用户确认：可以直接移除旧线路依赖，不考虑兼容；截图回归 Windows/Linux 同步推进；同线程多窗口与跨线程独立 runtime 都要考虑，其中同线程多窗口优先。  
> 作用范围：`include/ui.hpp`、`src/api`、`src/detail`、`src/common`、`src/core`、`src/services`、`src/systems`、`src/renderers`、`tests/unittest`、`tools/check_architecture_boundaries.py`。

## 1. 结论摘要

当前 VMP-ui 架构处在“功能原型已跑通，框架边界正在补课”的阶段。核心技术路线合理：EnTT 负责实体组件、SDL3 GPU 负责渲染、Yoga 负责布局，Chain DSL 提供用户侧声明式体验；但运行时上下文、公开 API、内部服务、System 职责、渲染边界仍未完全稳定。

推荐方向不是推倒重写，而是先把架构边界收紧：公开 API 不泄漏 EnTT 和隐式全局运行时；运行时访问从 `RuntimeFacade::current()` 迁移到显式 `UiRuntime` / `Application` / `Window` 归属；胖 System 按真实变化原因拆分；渲染层沉淀可测试的 RenderItem / DrawCommand 边界。

由于已确认“不考虑兼容”，旧线路、滞后 adapter、废弃入口不应继续做 deprecated 过渡，能确认替代路径的直接移除。多 runtime 验收顺序调整为：先保证同线程多窗口，再扩展跨线程独立 runtime；截图回归 Windows/Linux 同步建设。

## 2. 影响摘要

| 维度 | 当前状态 | 影响 |
|---|---|---|
| 公开 API | `factory` 仍大量返回裸 `ui::entity`，创建入口依赖隐式当前运行时。 | 多 runtime、多窗口和跨线程场景下归属不清，调用错误难以及早发现。 |
| Runtime | `UiRuntime` 已持有独立 `Registry`、`Dispatcher`、`WorkerMailbox`，但 `RuntimeFacade::current()` 仍是 thread-local 门面。 | 比旧全局单例更好，但仍不适合作为长期核心入口。 |
| 架构门禁 | `check_architecture_boundaries.py` 已有 baseline 和软门禁。 | 能防止债务增长，但还没有自然推动债务下降。 |
| System | `StateSystem` 同时处理 hover、active、focus、窗口状态、滚动条、slider、dropdown。 | SRP 风险高，后续新增控件会继续膨胀。 |
| 渲染 | renderer 仍较多依赖 ECS 实体和 registry。 | 截图回归、离屏测试和后端替换成本偏高。 |
| common/detail | `common` 与 `detail` 语义偏宽。 | 公开/内部边界不直观，依赖容易扩散。 |
| 构建治理 | CMake 选项、LTO、资源后端、shader 编译、边界检查较完整。 | 工程化基础较好，适合渐进重构。 |

## 3. 推荐架构方向

```text
include/ui.hpp
  ↓ 稳定公开 API、Result、公开轻量类型、runtime-aware 句柄
src/api
  ↓ 参数校验与用户 API；不暴露 EnTT；不承担复杂 ECS 业务
src/detail
  ↓ API 到内部服务的桥接；只做适配，不变成第二业务层
src/core
  ↓ UiRuntime、Application、EventLoop、SystemManager、线程/窗口归属
src/services
  ↓ 可测试根能力：Focus、Overlay、TextEditing、Theme、EventBridge
src/systems
  ↓ 每帧系统；依赖注入 Registry / Dispatcher / Services
src/renderers
  ↓ 渲染数据收集与后端绘制；逐步改向 RenderItem / DrawCommand
src/common/public 或 api/types
  ↓ 公开轻量类型
src/common/internal
  ↓ 内部组件、内部事件、上下文
```

关键约束：

- 新公开 API 必须能明确绑定 `Application` / `UiRuntime` / `Window`，避免隐式当前 runtime。
- `src/api/*.hpp` 不应 include EnTT、`RuntimeFacade`、`Registry`、`Dispatcher`。
- `systems`、`services`、`renderers` 不新增 `RuntimeFacade::current()`，历史 baseline 只减不增。
- `StateSystem` 后续只保留指针状态协调，focus、overlay、scroll、slider 等按变化原因拆出。
- 渲染层优先为截图回归准备稳定数据边界，而不是直接追求多后端抽象。

## 4. 修改规划表

| 阶段 | 优先级 | 工作项 | 文件范围 | 验收标准 |
|---|---:|---|---|---|
| P0 | 高 | 收紧架构门禁 baseline | `tools/check_architecture_boundaries.py` | 新增 `RuntimeFacade::current()`、`.raw()`、公开头 EnTT 泄漏会失败；baseline 只能减少。 |
| P0 | 高 | 公开 API 与内部类型切分 | `include/ui.hpp`、`src/api`、`src/common` | 公开头独立编译；公开 API 不依赖 EnTT。 |
| P0 | 高 | runtime-aware 句柄契约 | `src/api/Entity.hpp`、`src/api/Factory.*`、`src/core/UiRuntime.*` | 跨 runtime 操作能检测并失败；新创建入口显式绑定 runtime/window。 |
| P0 | 高 | 删除旧线路与兼容入口 | `include/ui.hpp`、`src/api`、`src/detail`、`src/CMakeLists.txt` | 废弃 factory/adapter/未接入 detail cpp 不再保留；示例改用新入口。 |
| P0 | 高 | 同线程多窗口优先验收 | `src/core/Application.*`、`src/core/PlatformWindow.*`、`src/api/Factory.*`、`tests/unittest` | 一个 runtime 内多个窗口实体互不串状态、输入、布局和渲染目标。 |
| P0 | 中 | Runtime 访问显式化 | `src/core/SystemManager.*`、`src/systems/*`、`src/services/*` | 新系统通过构造/注册注入依赖，不调用 thread-local current。 |
| P0 | 中 | 跨线程独立 runtime 约束 | `src/core/UiRuntime.*`、`src/core/WorkerMailbox.*`、`src/core/EventLoop.*` | runtime 明确 owner thread；跨线程写 UI 状态必须投递，不直接访问 registry。 |
| P1 | 高 | 拆分 StateSystem 根能力 | `src/systems/StateSystem.*`、新增 `src/services/*` | Focus、Overlay、Scroll/Slider 逻辑有独立服务或系统，职责可测试。 |
| P1 | 高 | Theme/Style 契约固化 | `src/common/Theme.hpp`、`src/api/Theme.*`、`src/systems/ThemeSystem.*` | 明确优先级：用户显式链式设置 > 局部样式 > 状态样式 > theme 默认。 |
| P1 | 高 | 渲染数据边界 | `src/renderers/*`、`src/systems/render/*`、`src/interface/IRenderer.hpp` | renderer 逐步接收 RenderItem/DrawCommand；截图回归可复用同一输出。 |
| P1 | 中 | Windows/Linux 截图回归基础 | `tests`、`tools`、`example/ui_demo` | 两个平台同步维护基准；固定字体、DPI、窗口尺寸；输出基准图、差异图和阈值判断。 |
| P2 | 中 | 控件补齐 | `src/api/Factory.*`、`src/api/Controls.*`、`src/renderers/*` | Tooltip、ContextMenu、Modal、ListView 等复用 Overlay/Focus/Theme，不自建小机制。 |

## 5. SOLID 检查

| 原则 | 当前风险 | 建议 |
|---|---|---|
| SRP | `StateSystem`、`detail`、`common` 职责偏宽。 | 按输入状态、焦点、浮层、主题、文本编辑、渲染数据分别拆边界。 |
| OCP | 新控件容易修改核心输入/渲染路径。 | 先固定 Overlay、Focus、Theme 扩展点，再补复杂控件。 |
| LSP | System/Renderer 接口目前风险较低。 | 新接口不要让调用方感知具体控件类型。 |
| ISP | `RuntimeFacade` 有胖门面趋势。 | 服务接口按调用方拆，不引入万能 `IUiServices`。 |
| DIP | 系统仍依赖隐式全局门面。 | 通过 `SystemManager` 注入 Registry、Dispatcher 与必要服务。 |

## 6. 风险与验证建议

| 风险 | 缓解 | 验证 |
|---|---|---|
| 一次性迁移 runtime 访问导致大面积编译失败 | 按系统逐个迁移，保留短期 adapter。 | `cmake --build build --config Debug`。 |
| API 破坏影响示例 | 已确认不考虑兼容；集中删除旧入口，示例同步迁移。 | `example/ui_demo` 全量构建与运行冒烟。 |
| 多 runtime 归属错误 | Entity/Window 句柄携带 runtime token。 | 增加跨 runtime AddChild/Show/Query 失败测试。 |
| 跨线程访问 Registry | UI 状态写入必须投递到 owner thread。 | 增加 worker 投递与非法直接访问测试。 |
| 截图回归跨平台不稳定 | Windows/Linux 同步建设；固定字体、DPI、尺寸，使用阈值和差异图。 | Windows/Linux 分平台基准图。 |

## 7. 已确认决策

1. 可以直接移除旧线路依赖，不用考虑兼容。
2. 截图回归 Windows/Linux 同步推进。
3. 同线程多窗口与跨线程独立 runtime 都要考虑；同线程多窗口优先。

## 8. 待确认问题

暂无。
