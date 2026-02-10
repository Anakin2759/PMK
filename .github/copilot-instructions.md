# PestManKill — Copilot 指引

## 项目概览

C++23 多人卡牌游戏（害虫杀），采用 Client/Server 架构，基于自研 ECS UI 框架 + KCP 可靠 UDP 网络层。使用 CMake 4.0 构建，MSVC / Clang-cl 编译，静态链接所有依赖。

## 架构与模块边界

```
src/
  client/   → 可执行文件 PestManKillClient（View 层 + 网络集成）
  server/   → 可执行文件 PestManKillServer（游戏逻辑 ECS）
  ui/       → 静态库，自研 ECS UI 框架（EnTT + SDL3 GPU + Yoga 布局）
  net/      → 静态库，KCP 可靠 UDP 传输层（ASIO + KCP）
  shared/   → INTERFACE 头文件库，消息协议定义（MessageBase CRTP）
  utils/    → INTERFACE 头文件库，Logger / Registry / ThreadPool 等工具
```

**关键设计差异**：UI 模块使用 `Registry` 全局单例访问 `entt::registry`；Server 模块使用 `GameContext&` 上下文注入。两侧 system 均使用 `EnableRegister<Derived>` CRTP，但接口方法名不同（UI: `registerHandlersImpl`，Server: `registerEventsImpl`）。

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

## 核心模式与约定

### 管道 DSL（UI 构建）

UI 实体通过 `operator|` 链式配置，定义在 `src/ui/api/Chains.hpp`：

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

| 类别 | 规则 | 示例 |
|------|------|------|
| 文件名 | PascalCase；UI/shared 用 `.hpp`，server/net 用 `.h` | `Components.hpp`, `KcpSession.h` |
| 类/结构体 | PascalCase | `LayoutSystem`, `KcpSession` |
| 命名空间 | 小写 | `ui::factory`, `ui::chains` |
| 公共函数 | PascalCase | `CreateButton()`, `EmplaceOrReplace()` |
| 私有/实现函数 | camelCase | `registerHandlersImpl()` |
| 成员变量 | `m_` 前缀 + camelCase | `m_registry`, `m_yogaConfig` |
| 常量 | `static constexpr` UPPER_SNAKE | `CMD_ID`, `MAX_LOG_FILE_SIZE` |

## C++23 特性使用

项目广泛使用现代 C++ 特性，生成代码时应优先：
- `std::expected<T, E>` 替代异常做错误处理（网络层）
- `std::move_only_function` 替代 `std::function`（事件回调）
- Concepts 做模板约束（`Component`, `UiTag`, `Action`）
- `std::source_location` 自动捕获日志调用位置
- `std::span` 传递缓冲区
- Deducing this（`this auto`）

## 关键第三方库

| 库 | 用途 | 注意事项 |
|----|------|----------|
| SDL3 | GPU 渲染 + 窗口 | **SDL3 API**（非 SDL2），无 `SDL_setenv` 等旧接口 |
| EnTT | ECS 框架 | `entt::registry` + `entt::dispatcher` + `entt::poly` |
| Yoga | Flexbox 布局 | 通过 `YGNodeRef` 管理布局树 |
| ASIO | 异步 I/O | Standalone 模式（`ASIO_STANDALONE`），非 Boost |
| KCP | 可靠 UDP | Pimpl 模式封装在 `KcpSession` 中 |
| spdlog | 日志 | Header-only 模式 |
| Eigen | 线性代数 | `Vec2 = Eigen::Vector2f` 用于 UI 数学类型 |

## 测试

Google Test 框架，测试位于 `tests/unittest/`，需 `-DENABLE_BUILD_TESTS=ON`。网络层有 Mock（`MockUdpTransport.h`），使用 `TEST_F` fixture 模式。
