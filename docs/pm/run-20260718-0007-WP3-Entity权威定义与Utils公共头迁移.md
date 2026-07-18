# WP3 Entity 权威定义与 Utils 公共头迁移

- 日期：2026-07-18
- 状态：completed
- 目标：闭合 Utils 公共头的 include-only 依赖，不把内部 Runtime 实现暴露到 SDK

## 架构选择

`Utils.hpp` 只需要前置声明 `UiRuntime`，因此本批不公开 Runtime 实现。独立头检查发现其 `Entity.hpp` 前置仍转发源码树且内部定义遗漏 `<limits>`，故先将 `ui::entity` 权威定义迁至稳定公共头，再迁移 Utils。

## 实施

- `include/ui/api/Entity.hpp` 成为 `ui::entity` 与 `null_entity` 的权威定义，并显式包含 `<cstdint>`、`<limits>`。
- `src/common/EntityTypes.hpp` 改为纯兼容转发，内部调用无需一次性改写。
- 新增 `include/ui/api/Utils.hpp`，删除旧 `src/api/Utils.hpp`。
- Utils 实现、Factory、单元测试、fallback 测试和 umbrella header 切换稳定路径。
- CMake 公共头清单和架构门禁同步更新。
- 新增 `PublicUtilsHeaderCheck.cpp` object target，仅获得 `${CMAKE_SOURCE_DIR}/include`，不链接 ui、EnTT 或 Eigen。

## 兼容性

`ui::entity` 仍为 `std::uint32_t`，`null_entity` 数值不变。Utils 的函数、模板、`TaskHandle`、`std::function<void()>` 参数及 current-runtime 行为均不变；本批不擅自替换为 `ui::Callback`。

## 验证

- Debug 全量构建：通过。
- `example_ui_demo` 编译及链接：通过。
- 架构门禁、umbrella、MathTypes 和 Utils 独立头检查：通过。
- 全量测试：176 passed / 0 failed。
- 指标：302 / 2 / 3 / 1。
- 仅保留 `Timer.cpp` 的既有 nodiscard warning。

## 明确延后

- `Factory.hpp` 物理迁移。
- 稳定公开 Application PImpl 和 UiRuntime 出口。
- Utils current-runtime 显式化及 Timer API 去重。
- CMake PUBLIC include/link 收紧、install/export 和独立 package consumer。