# VMP-ui 当前架构锐评（2026-06-04）

> 结论先行：这个项目已经有“像样的 UI 引擎骨架”，但现在最危险的不是功能不全，而是**边界正在被旧全局访问、API 反向依赖和 public include 泄露慢慢吃穿**。如果继续堆功能，后面会从“自研 ECS UI”滑向“到处能调、到处都耦合、哪里都不好测”的大泥球。

## 评价口径

本次只评架构，不评代码风格细枝末节。重点观察：

- ECS 运行时边界是否清楚；
- `api` / `common` / `core` / `systems` / `managers` / `renderers` 是否单向依赖；
- public API 是否真的有边界；
- 构建、测试、资源、渲染是否支持长期演进；
- 当前技术债是否会阻碍多 runtime、测试隔离、平台扩展。

## 值得保留的优点

### 1. 大方向是对的

UI 元素用 EnTT entity 表达，组件基本是纯数据，布局、交互、渲染、主题、动画分别由 System 处理。这比传统“控件类继承树”更适合做高性能、自定义渲染的 UI 框架。

证据：

- `src/common/components/*`
- `src/systems/*`
- `src/interface/ISystem.hpp`

### 2. API 体验有亮点

`operator|` 链式 DSL 是目前最有辨识度的部分。`Chain<F>` 做模块内零开销组合，`AnyChain` 做跨模块类型擦除，设计思路清楚，不是随便包一层 `std::function` 糊弄。

证据：

- `src/api/Chains.hpp`
- `README.md` 的 Quick Start / Chain DSL 示例

### 3. 运行时隔离已经开始补课

`Registry` / `Dispatcher` 不再只是粗暴全局单例，已经出现 `thread_local activeInstance`、`UiRuntimeScope`、`RuntimeFacade` 这类运行时上下文设施。这说明项目已经意识到多 runtime、测试隔离和生命周期问题。

证据：

- `src/singleton/Registry.hpp`
- `src/singleton/Dispatcher.hpp`
- `src/core/RuntimeFacade.hpp`

### 4. 渲染层有拆分意识

`RenderSystem` 用 PIMPL，把 GPU 资源、渲染上下文、批处理、字体纹理等重状态藏到实现层，头文件没有膨胀到不可控。

证据：

- `src/systems/RenderSystem.hpp`
- `src/systems/render/RenderSystemImpl.hpp`
- `src/systems/render/*`
- `src/renderers/*`

### 5. CMake 可配置性够用

测试、示例、LTO、clang-tidy、ASAN、shader 编译、资源后端、CPU fallback、平台缩放、多线程都有选项。对于练手项目来说，构建面已经明显超过普通 demo。

证据：

- `CMakeLists.txt`
- `src/CMakeLists.txt`

## 严重问题与技术债

### S1：旧静态 `Registry` / `Dispatcher` 入口仍在核心系统里横行（`LayoutSystem` 已收敛）

项目表面上已经有注入式 `Registry&` / `Dispatcher&`，此前核心系统仍大量使用 deprecated 静态入口，例如 `Registry::TryGet`、`Registry::AnyOf`、`Registry::View`、`Registry::Clear`。P1 已完成 `LayoutSystem.cpp` 的静态 `Registry::Xxx` 迁移，当前 P0 检查中系统层 deprecated 静态入口 baseline 已清空。

这会直接伤害：

- 多 runtime；
- 测试隔离；
- 依赖注入；
- 并发边界；
- 将来把系统拆出去复用的可能性。

此前最典型的重灾区是 `LayoutSystem.cpp`。它名义上是系统对象，实际大量逻辑还在读“当前全局 Registry”。该问题已通过向匿名 namespace helper 显式传入 `Registry&`、成员函数统一使用 `m_reg` 的方式完成收敛。

证据：

- `src/singleton/Registry.hpp`：静态 PascalCase API 已标记 deprecated；
- `src/singleton/Dispatcher.hpp`：静态事件 API 已标记 deprecated；
- `src/systems/LayoutSystem.cpp`：已移除 `Registry::TryGet` / `Registry::AnyOf` / `Registry::View` / `Registry::Clear` 静态调用，改用 `Registry&` 实例 API；
- `src/common/WindowEntityLookup.hpp`：`common` 层直接调用 `Registry::Valid` / `Registry::Get` / `Registry::View`。

锐评：**这是“全局单例换皮成 RuntimeFacade”的典型半迁移状态。最怕的是大家误以为已经完成解耦，实际只是多包了一层。**

建议：

1. `LayoutSystem.cpp` 已完成迁移，匿名 namespace 工具函数显式传入 `Registry&`，成员函数使用 `m_reg`；
2. 禁止 `systems/*` 新增 `Registry::Xxx` / `Dispatcher::Xxx` 静态调用；
3. CI 增加 grep 检查，把 deprecated 静态入口限制在兼容层、测试辅助层或极少数 public facade。

### S1：内部层反向依赖 `api`，架构方向被打穿

`api` 应该是对外门面：做参数校验、entity 包装、链式 DSL、调用内部 detail/service。现在不少内部层反过来 include `api/*`，导致依赖方向变成双向。

证据：

- `src/systems/HitTestSystem.cpp`、`src/systems/LayoutSystem.cpp`、`src/systems/TimerSystem.cpp`、`src/systems/TweenSystem.hpp`、`src/systems/StateSystem.cpp`、`src/systems/ThemeSystem.cpp`、`src/systems/render/RenderFrame.cpp`、`src/services/TextEditingService.cpp` 已从 `api/Utils.hpp` 切换到 `detail/Utils.hpp`；
- `src/systems/StateSystem.cpp` 已从 `api/Table.hpp` 切换到 `common/Table.hpp`；
- `src/systems/ThemeSystem.cpp` 已从 `api/Theme.hpp` 切换到 `common/Theme.hpp`；
- `src/renderers/TableRenderer.cpp` 已从 `api/Table.hpp` 切换到 `common/Table.hpp`，仍 include `api/Scale.hpp`；
- `src/renderers/ShapeRenderer.hpp` 已从 `api/Theme.hpp` 切换到 `common/Theme.hpp`，并通过 `RuntimeFacade` 读取 `ThemeContext`；
- `src/renderers/TextRenderer.hpp` 已移除 `api/Utils.hpp`，改用 `core/TextUtils.hpp` 与 `policies::HasFlag`。

锐评：**`api` 被系统层当工具箱用，是 public facade 最常见的腐烂方式。短期方便，长期会让“用户 API”和“内部实现工具”完全混在一起。**

建议：

1. `api/Utils.hpp` 反向依赖已完成第一轮收敛：内部系统/服务/文本渲染器不再 include `api/Utils.hpp`，改用 `detail/Utils.hpp` / `core/TextUtils.hpp` / `policies::HasFlag`；
2. `api/Theme.hpp` 的主题数据已下沉到 `common/Theme.hpp`，`api/Table.hpp` 中被内部复用的列宽计算已下沉到 `common/Table.hpp`；下一步处理 `api/Scale.hpp` / `api/Shortcut.hpp` / `api/Animation.hpp` / `api/Hierarchy.hpp` 等剩余内部反向依赖；
3. 建立规则：`systems`、`renderers`、`services` 默认不得 include `api/*`，例外必须说明原因。

### S2：`common` 层不够纯，已经混入运行时服务逻辑（已清零 common 运行时依赖）

`common` 理想上应放纯数据结构、事件、错误码、轻量纯函数。此前 `WindowEntityLookup.hpp` 与 `WindowSync.hpp` 放在 `common` 下，直接或间接访问 `RuntimeFacade` / `Registry`。P1 已将这两处运行时服务逻辑迁移到 `core`，当前 `src/common/**` 不再 include `core/RuntimeFacade.hpp` 或 `singleton/Registry.hpp`。

证据：

- `src/core/WindowEntityLookup.hpp`：窗口 ID ↔ entity 查找缓存已从 `common` 移至 `core`，并改用注入式小写 `Registry` 实例 API；
- `src/core/WindowSync.hpp`：窗口组件与 `SDL_Window` 同步逻辑已从 `common` 移至 `core`；
- `src/core/RuntimeFacade.hpp` 又声明了 `WindowLookupService`。

锐评：**`common` 一旦开始知道 runtime，分层就基本失效。common 应该是所有人能依赖的底座，不该反过来知道谁在运行。**

建议：

1. `WindowEntityLookup` 已从 `common` 迁到 `core`；
2. `WindowSync` 已从 `common` 迁到 `core`；
3. 统一通过 `RuntimeFacade::windowLookup()` 或注入服务访问窗口查找能力。

### S2：public API 边界是假边界

`include/ui.hpp` 是 public umbrella header，但它直接 include `../src/...`。同时 `ui` target 把 `${CMAKE_SOURCE_DIR}` 和 `${CMAKE_CURRENT_SOURCE_DIR}` 作为 `PUBLIC` include 目录暴露给用户。

证据：

- `include/ui.hpp` 直接 include `../src/common/...`、`../src/api/...`；
- `src/CMakeLists.txt` 中 `target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})`。

影响：

- 外部用户能直接 include 内部头；
- 内部目录结构会变成事实 API；
- 未来重构 `src` 路径会破坏用户；
- `include/ui.hpp` 无法承担稳定边界。

锐评：**现在的 public API 更像“把 src 目录公开卖票参观”，不是严格意义上的 SDK。**

建议：

1. 短期文档标注：`src/*` 公开只是过渡期，不承诺稳定；
2. 中期把稳定头迁到 `include/ui/...`；
3. `include/ui.hpp` 只聚合 `include/ui/...`；
4. 最终让 `src` include directory 变成 `PRIVATE`。

### S2：错误处理口径不一致

README 和项目指引强调 UI 层使用 `Result<T>` / `std::expected`，但 `Application` 构造里仍直接 `throw std::runtime_error`。

证据：

- `src/common/Result.hpp`；
- `src/common/ErrorCodes.hpp` / `src/common/ErrorCodes.cpp`；
- `src/api/Factory.hpp` 中 `CreateApplication` 返回 `Result<std::unique_ptr<Application>>`；
- `src/core/Application.cpp` 中 `SDL_Init` 失败时 `throw std::runtime_error`。

这不是绝对错误，但必须明确边界：构造失败是否允许异常？是否由 factory 捕获并转换？热路径是否完全禁止异常？现在文档口径和实现细节还没完全对齐。

锐评：**不是不能抛异常，是不能一边宣称 Result throughout，一边让用户猜哪里会抛。**

建议：

1. 检查并保证 `factory::CreateApplication` 捕获 `Application` 构造异常并转换为 `Result`；
2. 更理想：把 SDL 初始化拆成 `Application::initialize() -> Result<void>`；
3. 文档明确：public API 返回 `Result`，内部构造可短暂使用异常但不得穿透 public boundary。

### S2：系统装配硬编码，扩展点不足

`SystemManager` 构造函数里手写注册所有系统。这个方式简单直接，但长期会限制可选系统、测试替换、插件式扩展和按平台裁剪。

证据：

- `src/core/SystemManager.cpp` 构造函数连续 `m_systems.emplace_back(...)` 注册 `PlatformWindowSystem`、`InteractionSystem`、`TextInputSystem`、`HitTestSystem`、`TweenSystem`、`LayoutSystem`、`RenderSystem`、`StateSystem`、`ActionSystem`、`TimerSystem`、`ThemeSystem`、`ShortcutSystem`。

锐评：**现在是“系统经理”不是 manager，更像“系统硬编码清单”。这在早期没问题，但再加功能会越来越难裁剪。**

建议：

1. 短期抽出 `registerBuiltInSystems()`，先把构造函数变干净；
2. 提供测试用 `addSystemBeforeRegister()` 或构造注入；
3. 可选系统通过 CMake option 或 runtime config 控制；
4. 暂时不要上复杂插件框架，先把内建系统装配解耦。

### S3：组件文件开始变胖，领域边界会越来越难读

组件保持纯数据是优点，但部分组件头已经承载过多领域概念。例如布局相关文件同时容纳尺寸、位置、层级、滚动、线条、箭头等概念；视觉相关文件同时承载透明度、背景、边框、阴影、主题状态等。

证据：

- `src/common/components/Layout.hpp`
- `src/common/components/Visual.hpp`

锐评：**组件无行为很好，但“无行为”不等于“什么数据都塞一个文件”。再增长下去，编译依赖和认知负担会同时爆。**

建议：

1. 不急着大拆，避免为拆而拆；
2. 当文件继续增长或频繁冲突时，按 `Geometry` / `LayoutTree` / `Scroll` / `Decoration` 拆；
3. 拆分时保持组件仍然是纯数据，不要把行为塞回组件。

### S3：测试覆盖还偏“状态/API”，渲染与平台边界偏弱

测试已有基础，尤其有 public header check 和 ECS/API 分组。但复杂的 GPU 渲染、CPU fallback、窗口生命周期、shader/resource 后端缺少更系统的自动化验证。

证据：

- `tests/unittest/CMakeLists.txt`：当前主要目标为 `ui_unit_tests`、`ui_ecs_tests`、`ui_api_tests`；
- `src/systems/render/*`、`src/renderers/*` 复杂度较高，但测试入口有限；
- `src/CMakeLists.txt` 已有 `UI_FORCE_CPU_RENDER`、`UI_RESOURCE_BACKEND`、`UI_ENABLE_SHADER_COMPILATION` 等构建变体。

锐评：**渲染系统复杂度已经上来了，但测试还没跟上。没有 headless/fallback 测试，后面每次改渲染都像开盲盒。**

建议：

1. 给 `UI_FORCE_CPU_RENDER=ON` 建一套 CI / 本地测试配置；
2. 优先测 `BatchManager`、`ResourceProvider`、`TextTextureCache`、资源后端切换；
3. 平台窗口生命周期通过接口抽象后 mock，不要强依赖真实窗口；
4. 对 runtime scope、多 runtime、window lookup 增加回归测试。

## 建议的整改优先级

### P0：先止血

- 禁止新增系统层 `Registry::Xxx` / `Dispatcher::Xxx` 静态调用；
- 禁止新增 `systems` / `renderers` / `services` 对 `api/*` 的依赖，除非写明豁免；
- 文档声明 `src/*` public include 是过渡技术债。

落地状态：

- 已新增 `tools/check_architecture_boundaries.py` 作为 P0 软门禁；
- 已在 `CMakeLists.txt` 中接入 `ui_architecture_boundary_check`，默认构建会检查“新增边界债”；
- 2026-06-04 之前已经存在的反向依赖、静态入口调用与 `common` 运行时依赖被显式 baseline，后续只能减少，不应增加；
- `src/*` 当前仍通过 CMake `PUBLIC` include 暴露给外部，这是兼容期技术债，不应被视为稳定 SDK。

### P1：迁移最重的耦合点

- 迁移 `LayoutSystem.cpp` 的静态 Registry 访问；
- 拆分 `api/Utils.hpp`，把内部工具下沉；
- 移动 `WindowEntityLookup` 到 `core` 或 `services`；（已完成：`src/core/WindowEntityLookup.hpp`）
- 确认 `CreateApplication` 不让异常穿透 public API。

落地状态：

- 已删除 `src/common/WindowEntityLookup.hpp`；
- 已新增 `src/core/WindowEntityLookup.hpp`；
- 已删除 `src/common/WindowSync.hpp`；
- 已新增 `src/core/WindowSync.hpp`；
- `RuntimeFacade::WindowLookupService` 继续作为窗口查找入口；
- P0 baseline 中已移除 `common` 运行时依赖债，`src/common/**` 不再允许新增 `RuntimeFacade` / `Registry` 依赖。
- 已迁移 `src/systems/LayoutSystem.cpp` 的静态 `Registry::Xxx` 访问，P0 baseline 中系统层 deprecated 静态入口债务已清空。
- 已移除内部层对 `api/Utils.hpp` 的反向依赖，P0 baseline 中不再允许新增 `api/Utils.hpp` include。
- 已下沉主题数据到 `src/common/Theme.hpp`，并移除内部层对 `api/Theme.hpp` 的反向依赖。
- 已下沉表格列宽计算到 `src/common/Table.hpp`，并移除内部层对 `api/Table.hpp` 的反向依赖。

### P2：收窄 SDK 边界

- 建立 `include/ui/...` 公共头目录；
- `include/ui.hpp` 改为只聚合公共头；
- `src` include 目录逐步改为 `PRIVATE`；
- 给内部头增加“不稳定 API”约束。

### P3：补测试与构建矩阵

- 增加 CPU fallback / resource backend / runtime scope 测试；
- 渲染核心模块增加 headless 可测接口；
- 给 deprecated 静态入口和 API 反向依赖加自动扫描。

## 推荐自动扫描规则

可以先用简单 grep 做软门禁：

- `src/systems/**`、`src/renderers/**`、`src/services/**` 不应出现 `#include "api/`；
- `src/systems/**` 不应出现 `Registry::TryGet`、`Registry::Get`、`Registry::View`、`Registry::AnyOf`、`Registry::AllOf`、`Registry::Clear`；
- `src/systems/**` 不应出现 `Dispatcher::Trigger`、`Dispatcher::Enqueue`、`Dispatcher::Sink`、`Dispatcher::Update`；
- `src/common/**` 不应 include `core/RuntimeFacade.hpp` 或 `singleton/Registry.hpp`。

后续再把这些规则升级为 clang-tidy 自定义检查或 CMake lint target。

## 最终判断

VMP-ui 现在不是“架构不行”，而是处在一个很典型的转折点：

- 好的部分：ECS、DSL、运行时上下文、渲染拆分、CMake 选项都已经有雏形；
- 坏的部分：旧全局单例、API 反向依赖、common 污染、public include 泄露正在侵蚀边界；
- 最危险的部分：这些问题不会立刻炸，但会让每个新功能都更容易走捷径，最后把所有层粘死。

一句话锐评：**当前架构像一台已经装上好发动机的样车，但线束还裸露在外、控制器接口还没封壳；继续猛踩油门能跑，真要长期维护就得先整理线束、封住边界。**