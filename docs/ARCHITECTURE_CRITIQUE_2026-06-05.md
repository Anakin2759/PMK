# VMP-ui 当前架构再锐评（2026-06-05）

> 结论先行：项目的架构止血动作已经有效，`api` 反向依赖、`common` 运行时污染、系统层 deprecated 静态入口这几条最危险的线已经被门禁压住；但现在暴露出另一个更现实的问题：**边界迁移已经超过构建系统和测试体系的承载能力，代码在“架构方向变好”和“构建闭环变脆”之间拉扯**。继续优化前，必须先把“能稳定构建、能稳定验证、能稳定表达 public/internal 边界”这三件事补上。

## 评价口径

本次重新评估以 2026-06-05 当前工作区状态为准，重点观察：

- P0 架构门禁是否仍有效；
- `api` / `detail` / `common` / `core` / `systems` / `renderers` 的依赖方向是否继续收敛；
- 旧 `Registry::Xxx` / `Dispatcher::Xxx` 静态路径在生产代码和测试代码中的残留；
- public SDK 边界是否仍被 `src` include 泄露；
- CMake 源文件清单是否跟得上 `api` → `detail` 下沉；
- 测试是否支持运行时隔离和类型边界迁移。

## 当前正向进展

### 1. P0 架构门禁已经从“文档建议”变成“构建事实”

`tools/check_architecture_boundaries.py` 当前可通过，且 `ALLOWED_API_INCLUDE_COUNTS`、`ALLOWED_STATIC_RUNTIME_COUNTS`、`ALLOWED_COMMON_RUNTIME_INCLUDE_COUNTS` 均为空。也就是说，新增以下债务会直接被挡住：

- `core/detail/systems/renderers/services` 反向 include `api/*`；
- `systems/**` 新增 `Registry::Xxx` / `Dispatcher::Xxx` 静态入口；
- `common/**` 新增 `RuntimeFacade` / `Registry` 运行时依赖。

锐评：**这是目前最值得保留的成果。没有这个门禁，后面所有“重构”都会被新债抵消。**

### 2. `common` 运行时污染已经基本止血

此前 `WindowEntityLookup` / `WindowSync` 放在 `common` 下，直接把运行时服务逻辑塞进公共底座。当前已经迁到 `core`：

- `src/core/WindowEntityLookup.hpp`
- `src/core/WindowSync.hpp`

测试侧也开始跟进：

- `tests/unittest/test_Visibility.cpp` 已改用 `src/core/WindowSync.hpp`；
- `tests/support/ThemeSystemTest.hpp` / `UiTestRuntime.hpp` / `ComponentAssertions.hpp` 已移除测试支撑层旧 `Registry::Xxx` / `Dispatcher::Update` 路径，改走 `RuntimeFacade`；
- `tests/unittest/test_Utils.cpp`、`test_MainWindow.cpp` 已移除 `src/singleton/Registry.hpp` 直接引用并迁移到实例 registry。

锐评：**底座不再知道 runtime，这一步很关键。否则 `common` 会变成“谁都能依赖、也谁都能污染”的垃圾场。**

### 3. 内部层反向依赖 `api` 已经清零

当前扫描显示 `src/core/**`、`src/detail/**`、`src/systems/**`、`src/renderers/**`、`src/services/**` 中 `api/*` 反向依赖 baseline 为 0。

已完成的关键下沉包括：

- `api/Utils.hpp` 的内部使用改为 `detail/Utils.hpp`；
- `api/Theme.hpp` 数据下沉到 `common/Theme.hpp`；
- `api/Table.hpp` 内部列宽计算下沉到 `common/Table.hpp`；
- `api/Scale.hpp` / `api/Shortcut.hpp` / `api/Animation.hpp` / `api/Event.hpp` 的内部复用点下沉到 `common` 或 `detail`。

锐评：**public facade 不再被系统层当工具箱用，这是从“能跑的 demo”走向“可维护库”的分水岭。**

### 4. 运行时隔离路线已经形成，但还没完全落地

`Registry` / `Dispatcher` 已有 `thread_local activeInstance`、`UiRuntimeScope`、`RuntimeFacade`。系统构造也开始使用注入式 `Registry&` / `Dispatcher&`，`SystemManager` 已抽出 `registerBuiltInSystems()` 并提供 `addSystemBeforeRegister()`。

锐评：**运行时隔离已经不是口号，但现在还处于“新路修好了，旧路还很多人走”的阶段。**

## 当前严重问题与技术债

### S1：构建闭环曾被 `api` → `detail` 下沉打断（本轮已恢复）

本轮优化前，CMake 构建失败的主因之一不是语法错误，而是链接缺符号：

- `ui::factory::CloseDropDownPopup(entt::entity)`；
- `ui::utils::GetAbsolutePosition(entt::entity)`；
- `ui::utils::GetEntityRect(entt::entity)`；
- `ui::utils::GetScrollViewportRect(entt::entity)`；
- `ui::utils::GetScrollViewportLength(entt::entity, bool)`；
- `ui::utils::GetScrollContentLength(entt::entity, bool)`；
- `ui::utils::GetScrollMaxOffset(entt::entity, bool)`；
- `ui::utils::GetVerticalScrollbarGeometry(entt::entity)`。

这些调用来自内部 entt 路径，而 public `api/Utils.cpp` / `api/Factory.cpp` 只实现了 `ui::entity` 版本，导致链接阶段找不到 `entt::entity` 兼容符号。本轮已在 public facade 实现文件中补齐 entt 兼容转发，恢复构建闭环。

证据：

- `src/api/Utils.cpp`：已补齐 `GetAbsolutePosition(entt::entity)`、`GetEntityRect(entt::entity)`、滚动相关 entt 兼容重载，统一转发到 `ui::entity` 实现；
- `src/api/Factory.cpp`：已补齐 `CloseDropDownPopup(entt::entity)`，转发到 `CloseDropDownPopup(ui::entity)`；
- `src/systems/HitTestSystem.cpp`、`src/systems/StateSystem.cpp`、`src/systems/render/RenderFrame.cpp` 已调用 `ui::utils::*` 内部版本；
- `src/core/Application.cpp` 调用 `ui::factory::CloseDropDownPopup(event.entity)`；
- 2026-06-05 当前 `Build_CMakeTools` 构建已通过，`example_ui_demo` 链接缺符号问题已解除。

锐评：**构建闭环恢复了，但这次问题说明实体类型半迁移期间必须保留清晰的兼容层；否则每个内部 entt 调用点都可能变成链接雷。**

建议：

1. 已补齐当前链接缺失的 entt 兼容符号，并确认构建通过；
2. 后续继续把系统/detail 调用点逐步统一到明确的 `ui::entity` / `entt::entity` 转换工具；
3. 系统性核对所有 `detail/*.hpp` 是否存在声明但未链接实现的问题；
4. 中期考虑给 entity 兼容重载加集中测试，避免再次出现“编译过、链接炸”。

### S1：测试代码仍大量固化旧静态单例路径（ThemeSystem / Hierarchy / DragDrop 已清理）

生产代码 P0 门禁已压住系统层静态入口，但测试中仍残留大量旧路径：

- `tests/unittest/test_TaskChain.cpp` 仍使用 `Dispatcher::Update` / `Dispatcher::Sink` / `Dispatcher::Enqueue`；
- `test_TweenSystem.cpp` 仍使用 `Registry::Get/TryGet/AllOf/Emplace` 与 `Dispatcher::Trigger`。

已清理的测试包括：

- `test_Visibility.cpp`；
- `test_Utils.cpp`；
- `test_MainWindow.cpp`；
- `tests/support/ThemeSystemTest.hpp`；
- `tests/support/UiTestRuntime.hpp`；
- `tests/support/ComponentAssertions.hpp`。
- `test_ThemeSystem_Button.cpp` / `Input.cpp` / `DropDown.cpp` / `Window.cpp` 已通过 `ThemeSystemTest::registry()` 统一迁移到实例 registry，当前 `test_ThemeSystem_*.cpp` 已无 `Registry::` / `Dispatcher::` 匹配。
- `test_Hierarchy.cpp` 已移除 `src/singleton/Registry.hpp` 和旧 `Registry::TryGet` / `Registry::AllOf` / `Registry::EmplaceOrReplace` 静态路径，改用 `RuntimeFacade::current().registry()` 实例入口；涉及 `Hierarchy::parent` / `children` 的断言显式使用 `detail::ToInternal()`。
- `test_DragDrop.cpp` 已移除 `src/singleton/Registry.hpp` / `Dispatcher.hpp` 和旧 `Registry::TryGet` / `Registry::Emplace` / `Dispatcher::Trigger` 静态路径，改用实例 registry 与 `RuntimeFacade::current().trigger()`；拖放事件中的 `ui::entity` / `entt::entity` 边界通过 `detail::ToInternal()` 显式转换。

锐评：**测试如果继续走旧路，就会不断把旧架构“合法化”。测试不是债务隔离区，测试是架构契约的放大器。**

建议：

1. 新增测试侧软门禁，先统计 `tests/**` 的 `Registry::Xxx` / `Dispatcher::Xxx`，不立刻 fail；
2. 清理顺序建议更新为：`TweenSystem` → `TaskChain`；
3. 提供统一测试工具，例如 `tests/support/RuntimeAccess.hpp`：`ActiveRegistry()` / `ActiveDispatcher()` / `Trigger()` / `Update()`，避免每个测试重复写 `RuntimeFacade::current()`；
4. 清理完成后，把测试侧门禁从统计升级为 fail。

### S1：public API 边界仍是假边界

`include/ui.hpp` 仍直接 include `../src/...`，并且 `ui` target 仍把 `${CMAKE_SOURCE_DIR}` 与 `${CMAKE_CURRENT_SOURCE_DIR}` 作为 `PUBLIC` include 目录暴露。

证据：

- `include/ui.hpp` 聚合 `../src/common/...` 与 `../src/api/...`；
- `src/CMakeLists.txt`：`target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})`。

影响：

- 外部用户可以直接 include 内部头；
- `src` 目录结构变成事实 API；
- `api` / `detail` 下沉时，外部代码可能已经依赖旧路径；
- public SDK 无法承诺稳定。

锐评：**这仍然是“把 src 目录公开卖票参观”。只要 `src` 是 PUBLIC include，所谓 public/private 边界就只能靠自觉。**

建议：

1. 短期在 README 和 `include/ui.hpp` 注明 `src/*` 暴露是过渡期技术债；
2. 建立 `include/ui/...` 稳定头目录；
3. `include/ui.hpp` 只聚合 `include/ui/...`；
4. 分阶段把 `${CMAKE_SOURCE_DIR}` / `${CMAKE_CURRENT_SOURCE_DIR}` 从 `PUBLIC` 改为 `PRIVATE`；
5. 对内部测试可单独给 test target 添加 `src` include，不要用库的 public include 泄露解决测试便利性。

### S2：`ui::entity` 与 `entt::entity` 仍处于半迁移状态

`src/common/EntityTypes.hpp` 当前定义：

- `using entity = uint32_t;`
- `null_entity = 0xFFFFFFFFU`

但大量内部组件仍使用 `entt::entity`，例如 `components::Hierarchy::parent` / `children`。public API 开始走 `ui::entity`，内部系统和 detail 层仍大量走 `entt::entity`。

影响：

- 测试中频繁需要 `static_cast<entt::entity>(ui::entity)`；
- DSL `operator|` 接收 `ui::entity`，旧测试或内部 helper 若用 `entt::entity` 会直接失配；
- public boundary 和 ECS internal boundary 的转换点还不够集中。

锐评：**这是实体句柄“换壳”的危险期。现在最怕的不是类型不同，而是转换点到处散落，最后谁也不知道哪一层该拿哪种 entity。**

建议：

1. 明确规则：public API / DSL 使用 `ui::entity`，systems/detail/core 内部可使用 `entt::entity`；
2. 提供集中转换工具，例如 `ToInternalEntity(ui::entity)` / `ToPublicEntity(entt::entity)`，不要到处 `static_cast`；
3. `Registry` 已有 `ui::entity` 重载，测试优先走实例 registry 的 public 重载；
4. 待 public SDK 稳定后，再评估组件内部是否继续保留 `entt::entity`。

### S2：`RuntimeFacade` 正在变成“新全局”

旧 `Registry::Xxx` / `Dispatcher::Xxx` 被压下去后，`RuntimeFacade::current()` 成为新的主入口。它比旧单例好，因为有 `UiRuntimeScope` 和 active runtime，但如果所有代码都直接 `RuntimeFacade::current()`，依赖注入的收益会被稀释。

典型现状：

- `api` / `detail` 层大量通过 `RuntimeFacade::current().registry()` 访问 ECS；
- 测试支撑层也开始用 `RuntimeFacade` 替代旧静态单例；
- 系统对象构造已注入 `Registry&` / `Dispatcher&`，但部分 renderer/header 仍直接 include `singleton/Registry.hpp`。

锐评：**`RuntimeFacade` 是过渡桥，不应该变成新的高速公路。系统和服务能注入就注入，只有 public facade 和少量上下文边界才该 current()。**

建议：

1. 系统、服务、renderer 优先接收 `Registry&` / `Dispatcher&` 或明确上下文对象；
2. `RuntimeFacade::current()` 允许用于 public API facade、测试 fixture、运行时 scope 边界；
3. 后续架构门禁可区分“允许 current() 的目录”和“禁止 current() 的目录”。

### S2：系统装配已拆出函数，但还没真正可配置

`SystemManager` 已从构造函数中抽出 `registerBuiltInSystems()`，并提供 `addSystemBeforeRegister()`。这让构造职责变干净，但内建系统清单仍固定。

锐评：**现在只是把硬编码从构造函数搬到私有函数，方向对，但离可裁剪、可测试替换还有一步。**

建议：

1. 引入轻量 `SystemManagerOptions` 或 `BuiltInSystemMask`；
2. 优先支持测试场景禁用 `RenderSystem` / `PlatformWindowSystem`；
3. 暂时不要上插件框架，先把内建系统可裁剪做实。

### S3：组件文件仍在变胖

`Layout.hpp`、`Visual.hpp`、`Data.hpp` 承载概念越来越多。组件是纯数据是优点，但“纯数据”不等于“所有数据塞一个文件”。

锐评：**组件没有行为，不代表组件文件没有边界。继续膨胀下去，编译依赖和认知负担会一起涨。**

建议：

1. 不急着大拆，避免制造无意义 churn；
2. 下一次触碰相关领域时顺手拆：`Geometry` / `LayoutTree` / `Scroll` / `Decoration` / `FormControls`；
3. 拆分时保持组件纯数据，不把行为塞回组件。

### S3：渲染与平台测试仍偏弱

当前测试更多覆盖 API 状态和 ECS 组件状态，GPU / CPU fallback / shader / resource backend / window lifecycle 自动化仍不足。

锐评：**渲染系统复杂度已经进入“不能靠肉眼验”的阶段，但测试还停留在“组件状态对不对”。**

建议：

1. 给 `UI_FORCE_CPU_RENDER=ON` 建本地和 CI 变体；
2. 优先测 `BatchManager`、`ResourceProvider`、`TextTextureCache`、资源后端切换；
3. 平台窗口生命周期抽象后 mock；
4. 给 runtime scope、多 runtime、window lookup 增加回归测试。

## 当前优先级建议

### P0：恢复稳定构建闭环

1. 已补齐当前 `ui::utils::*` 与 `factory::CloseDropDownPopup` 的 entt 兼容链接符号；
2. 已跑通 `ui` 静态库 + `example_ui_demo` 链接；
3. 下一步继续清理测试编译错误与旧单例路径，但不要让测试修复掩盖主库链接缺口。

### P1：继续清理测试旧单例路径

1. `ThemeSystem_*` 已完成迁移，因为它们共享 `ThemeSystemTest` fixture，收益最大；
2. 下一步处理 `TweenSystem`、`TaskChain`；
3. 建立测试侧 runtime access helper；
4. 测试侧旧路径清零后加门禁。

### P2：收窄 public SDK 边界

1. 建立 `include/ui/...` 稳定头；
2. `include/ui.hpp` 改为只聚合稳定头；
3. CMake public include 移除 `${CMAKE_SOURCE_DIR}` 和 `${CMAKE_CURRENT_SOURCE_DIR}`；
4. 内部测试 target 自己 include `src`。

### P3：把 `RuntimeFacade` 限定为过渡桥

1. 给系统和服务继续注入 `Registry&` / `Dispatcher&`；
2. 给 renderer 明确上下文传递方式；
3. 后续门禁统计 `RuntimeFacade::current()` 目录分布。

## 最终判断

VMP-ui 当前不是“架构方向错”，而是进入了更难的阶段：

- 方向正确：ECS、DSL、运行时 scope、`api` 下沉、`common` 纯化、P0 门禁都在正确轨道；
- 风险转移：最危险的问题从“到处反向依赖、到处静态单例”转为“构建清单滞后、测试旧路径太多、public SDK 仍裸奔”；
- 下一步关键：不是继续大拆，而是恢复稳定构建闭环，并把测试也迁到新架构契约上。

一句话锐评：**这台样车的线束已经开始整理，裸线少了，但现在保险盒没接全、仪表盘还报警；别急着加新功能，先让它每天都能稳定点火。**