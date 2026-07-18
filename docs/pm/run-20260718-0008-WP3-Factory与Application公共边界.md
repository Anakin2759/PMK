# WP3 Factory 与 Application 公共边界

- 日期：2026-07-18
- 状态：completed
- 目标：完成最后一个 Factory 物理公共头迁移，并提供可安全消费 Factory 应用返回值的稳定 Application 类型

## 架构选择

Factory 公开签名继续只前置声明 `UiRuntime`，不将 EnTT、spdlog 或 Runtime 实现暴露到 SDK。由于
`CreateApplication()` 返回 `Result<std::unique_ptr<Application>>`，调用方析构返回值时需要完整 Application 类型，故同步新增
公开 PImpl 外壳，并保持析构函数 out-of-line。

## 实施

- 新增 `include/ui/Application.hpp`，公开稳定 PImpl 声明；`src/core/Application.hpp` 改为兼容转发。
- 新增权威 `include/ui/api/Factory.hpp`，删除旧 `src/api/Factory.hpp`。
- Factory/Application 实现、umbrella header、fallback 生命周期测试切换稳定路径。
- 新增 Application 与 Factory include-only object checks，仅获得 `include/`，不链接 ui 或第三方依赖。
- CMake 公共头清单登记新路径；架构门禁禁止旧 Factory 路径回归并清除已偿还基线。
- 示例移除显式 `${CMAKE_SOURCE_DIR}/src` include，验证只通过公开 umbrella 消费。

## 兼容性

Factory 函数、handle 字段、token、退出回调和 current-runtime 行为均未改变；本批不公开完整 UiRuntime，也不收紧全局
PUBLIC 依赖。Windows 宏策略采用公共屏蔽层：Factory 在 Windows 上先完成 SDK 头的幂等包含，再移除
`CreateWindow`/`CreateDialog` 通用宏；显式 `CreateWindowExW`、`CreateDialogParamW` 等 Win32 API 保持可用。

## 验证

- Debug 全量构建：通过。
- `example_ui_demo` 编译及链接：通过，不再依赖显式源码 include。
- Application/Factory/umbrella 独立公开头检查：通过。
- Factory 后置包含 Windows SDK 的宏回归检查：通过。
- 架构门禁：通过；指标 302 / 2 / 3 / 1。
- 全量测试：176 passed / 0 failed。
- CMake diagnostics：无新增诊断。

## 明确延后

- 公开完整 UiRuntime 的长期出口与 runtime-bound API 收敛。
- Types.hpp 剩余职责拆分。
- CMake PUBLIC include/link 收紧。
- install/export、独立 package consumer 与平台编译矩阵。
