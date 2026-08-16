# VMP-ui — Copilot 指引

## 项目概览

C++23 自研 ECS UI 静态库（从 PestManKill 独立），基于 EnTT + SDL3 GPU + Yoga Flexbox 布局。使用 CMake 4.0 构建，MSVC / Clang-cl 编译，静态链接所有依赖。

## 架构与模块边界

```
src/          → UI 静态库（ui）源码
example/      → example_ui_demo 可执行示例
tests/        → ui_tests 单元测试
```

**设计要点**：UI 模块使用 `Registry`（内部持有 `entt::registry`，通过 `UiRuntime` 注入 System，非全局单例）；System 均使用 `EnableRegister<Derived>` CRTP，接口方法名为 `registerHandlersImpl`。事件循环为自研 `src/utils/EventLoop.hpp`（有界 MPSC 单消费者）+ `src/core/EventLoop.hpp`（帧调度器，16ms 节流投递帧回调）。

## 构建命令

```bash
# 配置（Ninja + clang-cl，Debug）
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
# 构建
cmake --build build --config Debug
# 启用测试
cmake -B build -DENABLE_BUILD_TESTS=ON
# 启用 clang-tidy
cmake -B build -DENABLE_CLANG_TIDY=ON
```

VS Code 任务 `build Debug (CMake)` 可直接运行构建。

## 编译结构与优化

- ui 源码拆分为 7 个 **OBJECT 库**（`ui_core/systems/render/managers/renderers/api/resources_objects`），再聚合成静态库 `ui`（`VMPUI::ui`），对外行为不变。
- PCH 仅对重型模板模块（core/systems/render/managers）开启；api/renderers/resources 不开，避免重复生成 PCH。`-DUI_ENABLE_PCH=OFF` 可整体关闭。
- 模块级并行度：`UI_POOL_<MODULE>`（如 `-DUI_POOL_API=6 -DUI_POOL_RENDER=1`）覆盖全局 `UI_COMPILE_JOB_LIMIT`；空值继承全局。
- 架构门禁：`ui_architecture_boundary_check`（`tools/check_architecture_boundaries.py`）与公共头自包含门禁 `ui_public_headers_self_contained_check`（`tools/check_public_headers_self_contained.py`）均为默认构建（ALL）目标，新增边界债务或第三方 include 会导致构建失败。

## 核心模式与约定

### 管道 DSL（UI 构建）

UI 实体通过 `operator|` 链式配置，定义在 `include/ui/api/Chains.hpp`：

```cpp
using namespace ui::chains;
auto btn = ui::factory::CreateButton();
btn | Size(100, 40) | BackgroundColor(Color::Blue()) | Text(U"开始") | OnClick([]{...}) | Show();
parent | AddChild(btn);
```

组合样式可复用：`auto style = Size(100,40) | BackgroundColor(Color::Red());`

### View 层是纯函数

客户端视图声明在 `src/client/View/` 下，每个视图是 `inline void Create*()` 自由函数（非类）。通过检查 `BaseInfo.alias` 防止重复创建。

### 组件设计

组件是纯数据结构（无行为），内部标记 `using is_component_tag = void;`。Tag 用 `is_tags_tag`，Event 用 `is_event_tag`。配合 `Component` / `UiTag` concept 做编译期约束。

### 消息协议

`src/shared/messages/` 下定义所有网络消息。每条消息继承 `MessageBase<Derived>` CRTP，必须：

- 定义 `static constexpr uint16_t CMD_ID`
- 实现 `serializeImpl()` / `deserializeImpl()`（使用 `PacketWriter`/`PacketReader`）
- 实现 `toJsonImpl()` 用于调试

`MessageDispatcher` 按 `CMD_ID` 路由到类型化处理器。

### 事件系统

- `[IMMEDIATE]` 事件用 `Dispatcher::Trigger()` 立即执行
- `[BUFFERED]` 事件用 `Dispatcher::Enqueue()` 延迟到下一帧处理

## 命名约定

| 类别          | 规则                             | 示例                                       |
| ------------- | -------------------------------- | ------------------------------------------ |
| 文件名        | PascalCase；`.hpp` 扩展名      | `Components.hpp`, `RenderSystem.hpp`   |
| 类/结构体     | PascalCase                       | `LayoutSystem`, `RenderSystem`         |
| 命名空间      | 小写                             | `ui::factory`, `ui::chains`            |
| 公共函数      | PascalCase                       | `CreateButton()`, `EmplaceOrReplace()` |
| 私有/实现函数 | camelCase                        | `registerHandlersImpl()`                 |
| 成员变量      | `m_` 前缀 + camelCase          | `m_registry`, `m_yogaConfig`           |
| 常量          | `static constexpr` UPPER_SNAKE | `CMD_ID`, `MAX_LOG_FILE_SIZE`          |

## 错误处理约定

UI 层使用统一 `Result<T>` 基础设施（`src/common/Result.hpp` + `src/common/ErrorCodes.hpp/.cpp`）：

```cpp
// 错误载体：错误码 + 上下文 + 自动捕获的调用点
struct Error {
    UiErrc code;                 // 错误枚举（20 个码，段位预留 100）
    std::string context;         // 可选上下文（路径、别名、SDL_GetError() 等）
    std::source_location origin; // 构造点自动捕获
    std::string ToString() const; // "asset_not_found (ctx) @ File.cpp:88"
};

template <typename T>
using Result = std::expected<T, Error>;   // Result<void> 直接使用

// 工厂函数
Err(UiErrc::xxx)              // → std::unexpected<Error>，自动捕获 source_location
Err(UiErrc::xxx, "context")   // 附带上下文字符串
Err(other.error())            // 传播已有 Error（保留原始 origin）
Ok() / Ok(value)              // 成功值

// 传播宏
TRY(auto x, expr);            // 失败即 return，成功则声明变量 x
TRY_VOID(expr);               // Result<void> 版本
```

- `Error` 支持与 `UiErrc` 直接比较：`if (result.error() != UiErrc::INVALID_ENTITY)`。
- `std::formatter<Error>` / `std::formatter<UiErrc>` 已落地；但 spdlog 日志用内置 fmt，日志点须显式 `error.ToString()`。

## C++23 特性使用

项目广泛使用现代 C++ 特性，生成代码时应优先：

- `std::expected<T, E>` 替代异常做错误处理（UI 层统一用 `ui::Result<T>`）
- `std::move_only_function` 替代 `std::function`（事件回调）
- Concepts 做模板约束（`Component`, `UiTag`, `Action`）
- `std::source_location` 自动捕获日志调用位置
- `std::span` 传递缓冲区
- Deducing this（`this auto`）

## 关键第三方库

| 库                  | 用途                  | 注意事项                                                                                                                                        |
| ------------------- | --------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| SDL3                | GPU 渲染 + 窗口       | **SDL3 API**（非 SDL2）；静态模式下对象会被合并进 VMPUI.lib                                                                               |
| EnTT                | ECS 框架              | header-only；仅内部使用，不出现在公共头                                                                                                         |
| Yoga                | Flexbox 布局          | 编译型；合并进 VMPUI.lib                                                                                                                        |
| freetype / harfbuzz | 字体光栅化 / 文本成形 | 编译型；合并进 VMPUI.lib                                                                                                                        |
| spdlog              | 日志                  | Header-only 模式                                                                                                                                |
| Eigen               | 内部线性代数          | **公共头不依赖 Eigen**：`ui::Vec2/Vec4/Rect` 是自包含公共类型（`include/ui/MathTypes.hpp`），内部经 `EigenConversions.hpp` 边界转换 |

## 自包含发行模式

- 公共头 `include/ui/**` **零第三方依赖**（仅允许 C++ 标准库与平台 SDK，如 `Windows.h`）；门禁脚本 `tools/check_public_headers_self_contained.py` 强制检查。
- 静态模式：`ui` 库在 POST_BUILD 阶段用 `lib.exe`/`llvm-lib` 把 SDL3/yogacore/freetype/harfbuzz（CMRC 模式含 ui_fonts）合并为**单文件 `VMPUI.lib`**。
- 动态模式：`BUILD_SHARED_LIBS=ON` 产出单 `VMPUI.dll`（公共 API 的 `VMP_UI_API` 导出宏为 P1 工作）。
- 发行默认资源后端为 `STD_EMBED`（资源表生成 .cpp 直接编进库，无 cmrc 依赖）；CMRC 仅开发/测试用。
- 消费者只需 `find_package(VMPUI CONFIG REQUIRED)` + `target_link_libraries(app PRIVATE VMPUI::ui)`，无需安装/链接任何第三方库；系统平台库由 `VMPUITargets.cmake` 以 `$<LINK_ONLY:...>` 带出。
- 发布构建建议 `-DENABLE_LTO=OFF`（默认 OFF），保证合并库消费者任意配置即插即用。

## 测试

Google Test 框架，测试位于 `tests/unittest/`，需 `-DENABLE_BUILD_TESTS=ON`。使用 `TEST_F` fixture 模式。
