# WP3 Visibility 公共头迁移

- 日期：2026-07-17
- 状态：completed
- 范围：利用已公共化的 `ui::Color`，迁移一个不依赖 Vec2/Rect/Eigen 的 API 叶子头

## 实施

- 新增稳定公共头 `include/ui/api/Visibility.hpp`。
- 删除旧权威路径 `src/api/Visibility.hpp`。
- 公共头直接依赖 `ui/Color.hpp`、`ui/api/Entity.hpp` 和 `ui/api/Chains.hpp`，不再包含聚合内部头 `common/Types.hpp`。
- `Visibility.cpp`、umbrella header、测试和 CMake 公共头清单切换到稳定路径。
- 架构门禁将 `Visibility.hpp` 加入已迁移 API 头集合，禁止旧路径回归。

## 兼容性

函数签名、DSL 名称、参数传递和运行时行为均未改变。`Color` 的权威定义、布局和 ABI 未改。本批不处理 Vec2/Rect，也不收紧 CMake PUBLIC include/link。

## 验证

- Debug 构建：通过。
- 架构门禁及公开 umbrella header 编译：通过。
- Visibility/PublicLeafHeaders 定向测试：26 passed / 0 failed。
- 全量测试：171 passed / 0 failed。
- 当前架构指标：`UiRuntime::current()` 302、PUBLIC 内部 include 2、PUBLIC 内部依赖 3、queued event 生产派发点 1。
- 新增问题日志：0。

## 残余风险

`Text.hpp` 和 `Table.hpp` 也是 Color-only 候选，但接口面更大；Controls 仍被 `Callback<Vec2>` 阻塞，Animation/Canvas/Utils/Factory/Image 仍需 Vec2/Rect 公共 ABI 决策。
