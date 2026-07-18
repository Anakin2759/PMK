# WP3 Factory / Application 公共头边界最小批次规划

- 日期：2026-07-18
- 输入来源：用户指定审计范围；`src/api/Factory.hpp/.cpp`、`src/core/Application.hpp/.cpp`、`src/core/UiRuntime.hpp`、`include/ui.hpp`、根与 `src/` CMake、`tests/unittest`、`tests/support`、`example/ui_demo`
- 作用范围：Factory 物理公共头迁移、Application PImpl 声明公共化、公共头独立编译门禁、相关 include/CMake/test/example 收口
- 改动类型：公开头路径迁移 + 公开类型出口补齐；不改变运行时实现，不收紧整个 target 的依赖传播，不推进 WP8 handle 生命周期重构

## 1. 审计结论与推荐决策

### 1.1 当前阻塞链

当前 umbrella 的实际链路为：

`include/ui.hpp` → `api/Factory.hpp`（依赖 `${CMAKE_SOURCE_DIR}/src` 的 PUBLIC 搜索路径）→ `core/UiRuntime.hpp` + `core/Application.hpp`。

其中：

- `UiRuntime.hpp` 是内部服务容器完整定义，直接包含 `ThreadPool`、`Dispatcher`、`Logger`、`Registry`，继续向下带入 EnTT/spdlog 等实现依赖；它不能迁入公共 SDK。
- `Application.hpp` 已是标准 PImpl 外壳：数据成员只有 `std::unique_ptr<ApplicationImpl>`，析构函数已在 `.cpp` 中 out-of-line 定义，公开头无需知道 SDL、EnTT、spdlog、EventLoop 或 SystemManager。
- `Factory.hpp` 的 runtime-bound 重载只使用 `UiRuntime&`，因此只需 `class UiRuntime;`，不需要 `UiRuntime` 完整定义。
- `CreateApplication()` 返回 `Result<std::unique_ptr<Application>>`。Factory 仅前置声明 `Application` 虽可声明函数，但调用方销毁返回的 `expected<unique_ptr<Application>, Error>` 时 `default_delete<Application>` 需要完整 `Application`。因此，若 `<ui/api/Factory.hpp>` 要独立可用，必须同时提供稳定且完整的公共 `Application` 类声明。

### 1.2 明确设计决策

1. **本批必须新增稳定 `include/ui/Application.hpp`。** 迁移现有 PImpl 外壳，不迁移 `ApplicationImpl`；这是使 `CreateApplication()` 可被公共 Factory 消费的必要条件，不是额外分层。
2. **新增权威头 `include/ui/api/Factory.hpp`。** 它只包含标准库头、`ui/Application.hpp`、`ui/MathTypes.hpp`、`ui/Result.hpp`、`ui/api/Entity.hpp`，并前置声明 `UiRuntime`。
3. **不新增 public `UiRuntime.hpp`。** Factory 的 `UiRuntime&` 参数、`Application::runtime()` 返回引用以及 `app->runtime()` 直接传给其他公开函数均可在不完整类型下编译。需要调用 `registry()/dispatcher()/logger()` 的代码属于内部/测试代码，不是本批公共契约。
4. **Application 继续使用 PImpl，且析构保持 out-of-line。** 不允许在公共头内 `= default` 析构，否则会把 `ApplicationImpl` 完整类型要求推回消费者。
5. **本批不改变 Factory 函数签名、行为和 handle 布局。** `EntityHandle`、`WindowHandle`、`MakeEntityHandle`、`MakeWindowHandle` 原样迁移，避免把路径迁移与 WP8 语义变更耦合。
6. **本批不把 CMake 的 EnTT/Eigen/spdlog 或内部 include 路径整体改为 PRIVATE。** 目前 umbrella 仍含 `api/Shortcut.hpp`，且 `Event/Scale/Theme` 等公共头仍通过内部头工作；立即收紧会把 Factory 独立批次扩散为全 SDK 闭包修复。Factory/Application 的独立 header-check 必须不给这些第三方路径，以证明本批边界本身已闭合。
7. **`src/api/Factory.hpp` 删除，不保留同名转发头。** 它是最后一个待迁移 API 物理头，保留会延续双权威路径。直接包含旧源码路径的仓外代码发生源码不兼容；`<ui.hpp>` 用户和函数 ABI 不变。
8. **`src/core/Application.hpp` 建议保留一轮纯转发兼容头。** 权威定义迁到 `include/ui/Application.hpp`；旧 core 路径只允许包含公共头，不得保留另一份类定义。这样不会暴露 core 实现，同时降低已存在直接 include 的破坏。待 install/export consumer 落地后再删除。
9. **`Application::onQuitRequested` 本批保持现状。** 它看起来是内部事件适配方法且当前无外部调用，但此批目标是路径和依赖闭包，不同时做 API 缩减。后续单独评估移为 private/删除。

## 2. 影响摘要

| 范围 | 当前状态 | 本批结果 |
|---|---|---|
| Factory 公共路径 | 权威头在 `src/api`，umbrella 依赖源码搜索路径 | 权威头位于 `include/ui/api` |
| Application 完整类型 | 只能从 `src/core` 获得 | PImpl 外壳位于 `include/ui/Application.hpp` |
| UiRuntime | Factory 直接包含内部完整定义 | Factory/Application 仅前置声明 |
| 第三方头闭包 | Factory 经 UiRuntime 间接带入内部/第三方 | Factory/Application 叶子闭包不含 EnTT/SDL/spdlog |
| Factory 实现 | 同目录相对 include，另重复包含 Application/UiRuntime | 显式包含公共 Factory；内部实现继续按需包含 core/第三方 |
| umbrella | 使用 `"api/Factory.hpp"`，依赖 src PUBLIC path | 使用 `"ui/api/Factory.hpp"`；Application 由 Factory 导出 |
| tests/example | 生命周期测试直接 include 旧 Factory；example 额外拿到 `src` include | 生命周期测试切换公共路径；example 移除显式 src include |
| CMake 全局边界 | 仍传播源码目录及 EnTT/Eigen/spdlog | 本批只登记新公共头并增加孤立头检查；全局收紧延后 |

### 2.1 SOLID / YAGNI 检查

- SRP：`Application.hpp` 只声明应用生命周期外壳；`Factory.hpp` 只声明创建 API 和公开 DTO。
- OCP：Factory 继续通过实现文件接入内部 runtime/SDL，不为新增控件修改 Application 边界。
- LSP：无新增继承层次。
- ISP：不公开完整 UiRuntime 服务接口，避免消费者依赖 registry/logger/dispatcher。
- DIP：公共创建 API 依赖自有值类型和 PImpl 外壳；内部实现依赖 SDL/EnTT/spdlog。
- YAGNI：不引入 `IApplication`、自定义 deleter、抽象 Runtime 接口或 DI 容器。

引入 public Application 外壳的理由：`unique_ptr<Application>` 的默认删除器要求调用点可见完整 `Application` 类；不引入的代价是 Factory 头只能“声明可见、实际不可安全消费”，或必须破坏性改成自定义 handle/deleter。

## 3. 精确文件变更规划

### 3.1 P0：公共权威头

| 文件 | 操作 | 精确内容 |
|---|---|---|
| `include/ui/Application.hpp` | 新增 | 从 `src/core/Application.hpp` 迁入 `Application` PImpl 外壳；仅保留 `<memory>`、`<span>`、`ApplicationImpl/UiRuntime/events::QuitRequested` 前置声明；析构仅声明、不得 inline default；保持现有 ctor、deleted copy/move、`exec()`、`runtime()`、`onQuitRequested()` 签名 |
| `src/core/Application.hpp` | 改为纯转发 | 仅 `#pragma once` + `#include "ui/Application.hpp"`，不得继续定义类或包含 core/第三方头 |
| `include/ui/api/Factory.hpp` | 新增 | 迁入所有公开声明与 `EntityHandle/WindowHandle`；删除 `core/UiRuntime.hpp` 和 `core/Application.hpp` include；增加 `class UiRuntime;`；包含 `ui/Application.hpp`；补齐标准头 `<concepts>`（`std::same_as`）、`<cstdint>`、`<memory>`、`<span>`、`<string>`、`<string_view>`、`<type_traits>`、`<vector>` |
| `src/api/Factory.hpp` | 删除 | 删除旧权威头，不保留 API 双路径 |

### 3.2 P0：实现与 umbrella 切换

| 文件 | 修改 |
|---|---|
| `src/api/Factory.cpp` | 首 include 改为 `ui/api/Factory.hpp`；保留 `core/UiRuntime.hpp`、`core/UiRuntimeScope.hpp`、SDL、EnTT、Logger、Registry、组件等为 PRIVATE 实现依赖；删除因新公共头已提供而重复且无直接需要的 include 仅限编译器/IWYU 证明可删，不顺带重排实现 |
| `src/core/Application.cpp` | 首 include 改为 `ui/Application.hpp`；Factory include 改为 `ui/api/Factory.hpp`；其余 core/SDL/Logger/Dispatcher 依赖仍仅存在于 `.cpp` |
| `include/ui.hpp` | `#include "api/Factory.hpp"` 改为 `#include "ui/api/Factory.hpp"`；不额外重复 include Application，Factory 已为其返回值完整类型导出该依赖 |

### 3.3 P0：CMake 与边界门禁

| 文件 | 修改 |
|---|---|
| `src/CMakeLists.txt` | `UI_HEADERS` 新增 `${CMAKE_SOURCE_DIR}/include/ui/Application.hpp` 与 `${CMAKE_SOURCE_DIR}/include/ui/api/Factory.hpp`；`core/Application.hpp` 保留为兼容转发头登记；不在本批修改 `target_link_libraries(ui PUBLIC ...)` 或 PUBLIC include 集合 |
| `tests/unittest/CMakeLists.txt` | 新增不链接 `ui`、仅包含 `${CMAKE_SOURCE_DIR}/include` 的 Factory/Application object header-check，并挂到 `ui_api_tests`；该 target 不显式链接 EnTT、SDL、spdlog、Eigen |
| `tools/check_architecture_boundaries.py` | `migrated_api_headers` 增加 `Factory.hpp`，禁止 `src/api/Factory.hpp` 回归；删除 `Application.cpp -> "api/Factory.hpp"` 的旧债 baseline；如增加公共 Application 头规则，确保公共头禁止 core/SDL/spdlog/EnTT include |

建议新增两个小型检查源而非复用链接 `ui` 的 `ui_public_header_check`：

| 文件 | 目的 |
|---|---|
| `tests/support/PublicApplicationHeaderCheck.cpp` | 只包含 `<ui/Application.hpp>`；静态检查不可复制/移动、析构可用及 `runtime()` 返回引用 |
| `tests/support/PublicFactoryHeaderCheck.cpp` | 只包含 `<ui/api/Factory.hpp>`；静态检查函数签名和 handle 字段类型；定义一个仅编译的函数创建并销毁 `CreateApplication()` 返回对象，以真实触发 `expected<unique_ptr<Application>>` 的析构完整类型要求 |

### 3.4 P1：现有调用方与测试

| 文件 | 修改 |
|---|---|
| `tests/unittest/test_PublicLeafHeaders.cpp` | 直接包含 `<ui/Application.hpp>`、`<ui/api/Factory.hpp>`；增加 Factory/Application 签名与 handle 基础契约静态断言，不访问 Runtime 内部服务 |
| `tests/unittest/test_FallbackWindowLifecycle.cpp` | `src/api/Factory.hpp` 改为 `<ui/api/Factory.hpp>`；保留内部 `UiRuntimeScope`/SDL include，因为该集成测试需要检查 registry、dispatcher 和 SDL 窗口，不应伪装为纯 public consumer |
| `tests/unittest/test_UmbrellaHeader.cpp` | 保持只 include `<ui.hpp>`；可增加最小编译期签名断言，但不承担“无第三方 include path”证明 |
| `example/ui_demo/CMakeLists.txt` | 删除 `${CMAKE_SOURCE_DIR}/src` 显式 include；保留 demo 自身目录和 `ui` 链接 |
| `example/ui_demo/main.cpp` | 无源码行为变更；继续通过 `<ui.hpp>` 创建/销毁 Application，并把不完整 `UiRuntime&` 直接传给 Timer API |
| `example/ui_demo/View/*.h` | 预期无改动；它们只经 `<ui.hpp>` 使用 Factory/Chains，没有内部 include |

## 4. 最小批次边界与依赖

推荐批次名称：**WP3-F — Public Factory + PImpl Application Shell**。

该批次必须原子完成以下四项，不能只迁 Factory 文件：

1. public Application PImpl 外壳；
2. public Factory 权威头，UiRuntime 改前置声明；
3. umbrella/实现/CMake 路径切换；
4. 无第三方 include path 的 Application/Factory 独立编译检查。

原因：只做第 2 项会在 `unique_ptr<Application>` 析构处留下不完整类型缺陷；只让 umbrella 构建通过又会被 `ui` 的 PUBLIC 第三方依赖掩盖。

本批不依赖 UiRuntime 公共化，也不依赖 CMake 全局 PRIVATE 收紧，因此可独立验收、独立回滚。

## 5. 兼容性

### 5.1 保持兼容

- 所有 Factory 函数名称、命名空间、参数、默认参数和返回类型不变。
- `Application` 类名、PImpl 布局（单 `unique_ptr` 成员）、构造/析构、`exec()`、`runtime()` 及 deleted copy/move 契约不变。
- `EntityHandle` 仍只有 `raw`；`WindowHandle` 的 `raw/windowId/token` 顺序和类型不变。
- `<ui.hpp>` 消费代码不需要修改。
- `src/core/Application.hpp` 通过纯转发保留一轮源码兼容。

### 5.2 有意不兼容

- 直接包含 `src/api/Factory.hpp` 或依赖 `"api/Factory.hpp"` 的仓外代码必须改为 `<ui/api/Factory.hpp>`。该路径属于源码树实现路径，不作为稳定 SDK 路径继续承诺。
- 不保证静态库跨不同编译器/CRT 的 ABI；本批只保证同工具链下类布局及符号签名不因迁移改变。

### 5.3 不在本批承诺的契约

- `WindowHandle::token` 当前是 Runtime 地址转换值，不代表已验证的稳定 lifetime token。
- `EntityHandle` 当前不保存 token，而 `MakeEntityHandle()` 忽略 token，语义与 WindowHandle 不对称。
- `Make*Handle()` 虽已由 umbrella 暴露，但更像构造 helper；本批为兼容原样保留，是否从 API 删除留给 WP8。
- `Application::onQuitRequested()` 是否属于 public API 尚未决策；本批不改变。
- `UiRuntime` 完整类型及 `registry()/dispatcher()/logger()` 不进入稳定 public API。

## 6. 验收矩阵

| 维度 | 验收项 | 预期结果 |
|---|---|---|
| 物理路径 | `include/ui/api/Factory.hpp`、`include/ui/Application.hpp` 存在 | 权威定义位于 include |
| 旧路径 | `src/api/Factory.hpp` 不存在 | 门禁阻止回归 |
| Factory 闭包 | 仅给 `${repo}/include` 编译 `PublicFactoryHeaderCheck.cpp` | 不需要 EnTT/SDL/spdlog/Eigen/source-root include |
| Application 闭包 | 仅给 `${repo}/include` 编译 `PublicApplicationHeaderCheck.cpp` | PImpl 外壳可独立编译 |
| 完整类型 | header-check 中创建并销毁 `CreateApplication()` 结果 | 无 `default_delete<Application>` incomplete-type 错误 |
| UiRuntime 前置声明 | public Factory/Application 搜索 `core/UiRuntime.hpp` | 0 命中；只有 `class UiRuntime;` |
| 第三方泄漏 | 两个新公共头搜索 `entt/SDL/spdlog` | 0 include、0公开类型命中 |
| umbrella | `ui_public_header_check`、`ui_api_tests` | 构建通过 |
| API 契约 | `test_PublicLeafHeaders` | Factory/Application 签名与 handle 基础断言通过 |
| 行为回归 | `ui_fallback_lifecycle_tests` | offscreen/software 100 次创建关闭通过 |
| 示例消费 | `example_ui_demo` 在不显式添加 `src` include 下 | 编译链接通过 |
| 全量回归 | `ui_unit_tests`、`ui_ecs_tests`、`ui_api_tests` | 全过 |
| 架构门禁 | `ui_architecture_boundary_check` | Factory 旧路径和 Application 内部 API include baseline 清零 |

建议执行顺序：重新配置测试构建 → 单独构建两个 header-check → `ui_api_tests` → fallback lifecycle → example → 全量 Debug 构建/CTest。

## 7. 风险与控制

| 风险 | 等级 | 控制 |
|---|---|---|
| Factory 只前置声明 Application 导致 unique_ptr 析构失败 | 高 | Factory 包含稳定 `ui/Application.hpp`；header-check 实际构造/销毁返回对象 |
| 把完整 UiRuntime 搬进 include，泄漏 EnTT/spdlog | 高 | public 仅前置声明；内部测试按需显式 include core UiRuntime |
| umbrella 测试因链接 `ui` 而掩盖依赖泄漏 | 高 | 新 object check 不链接 ui、不给第三方路径 |
| Application 类定义双份导致 ODR/漂移 | 高 | `src/core/Application.hpp` 只能是纯转发，唯一权威定义在 include |
| 删除旧 Factory 路径影响内部/仓外调用 | 中 | 全仓切换；明确该路径非稳定 SDK；门禁防回归 |
| 同批收紧 PUBLIC link/include 引发无关失败 | 中 | 明确延后，先以孤立叶子头证明本批边界 |
| 将 `onQuitRequested` 意外固化为长期 public API | 中 | 本批标记为兼容保留项，WP8 前单独决策 |
| token/handle 生命周期被误认为已稳定 | 中 | 测试只断言字段/创建行为，不新增有效性承诺 |

## 8. 延后事项

1. **CMake SDK 总边界收紧**：把 `${CMAKE_SOURCE_DIR}`、`${CMAKE_CURRENT_SOURCE_DIR}` 从 ui 的 PUBLIC include 移除，并将 EnTT/Eigen/spdlog 按最终公共闭包改为 PRIVATE；前置是 Shortcut/Event/Scale/Theme 等剩余内部转发依赖清理。
2. **独立 install/export consumer**：从安装树仅用 `find_package` + `<ui.hpp>` 构建，不可访问源码树或 third_party。
3. **public UiRuntime 决策**：若未来确需消费者直接操作 runtime，应设计窄 public façade/操作 API；不得直接搬出当前服务容器完整定义。
4. **WP8 handle 契约**：统一 EntityHandle/WindowHandle token、跨 Runtime 校验、失效行为、裸 entity deprecation 和版本化周期。
5. **Application API 清理**：评估删除或私有化 `onQuitRequested()`，以及是否提供非阻塞 step/run 接口；不与本次迁移绑定。
6. **Factory 职责拆分**：当前同时创建 Application、平台窗口和所有控件，规模较大；在出现真实维护冲突前不拆 `ApplicationFactory/WidgetFactory/WindowFactory`。
7. **旧 `src/core/Application.hpp` 转发删除**：完成 install/export consumer 与迁移说明后再移除。

## 9. 待确认问题

1. 默认把 `src/api/Factory.hpp` 视为非稳定源码路径并直接删除；若项目承诺源码树 include 兼容，可改成一轮纯转发，但会延迟“旧路径清零”验收。
2. 默认 `src/core/Application.hpp` 保留一轮纯转发兼容；若 WP3 要求 `src/core` 公共外壳路径也立即清零，可同步删除并全仓切换。
3. 默认不在本批移除 `Application::onQuitRequested()` 与 `Make*Handle()`；它们应在 WP8 的版本化/兼容策略中处理。
