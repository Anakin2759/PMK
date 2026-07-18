# WP3 公共 Vec2/Rect 与 Eigen 运算边界

- 日期：2026-07-18
- 状态：completed
- 决策依据：`docs/architecture/decision-public-math-types-20260718.md`
- 设计记录：`docs/architecture/change-public-vec2-contract-and-eigen-boundary-20260718.md`

## 实施

- 新增 `include/ui/MathTypes.hpp`，提供无第三方依赖的 `ui::Vec2`、`ui::Rect` 和 `LengthSquared()`。
- 固定 Vec2 两个 float、Rect 四个 float 的 standard-layout、trivially-copyable、尺寸和对齐契约。
- `src/common/Types.hpp` 删除 `using Vec2 = Eigen::Vector2f` 与旧 Rect 权威定义，改为聚合公共数学类型；其余 Vec3/Vec4/矩阵职责暂留。
- 新增内部 `src/common/EigenConversions.hpp`，仅通过显式 `ToEigen()`/`FromEigen()` 进入和离开 Eigen 运算。
- Animation、Canvas、Controls、Factory、Image、Utils、Table 公开头不再直接包含 `common/Types.hpp`。
- RenderFrame、IconRenderer、Transform helper 的真实 Eigen 边界改为显式转换；StateSystem 的长度平方改用公共自由函数。
- 新增只获得项目 `include/`、不链接 ui 且不给 Eigen include path 的公共数学头 object 编译检查。

## 明确未做

- 未迁移 Vec3/Vec4/矩阵/Transform/EdgeInsets。
- 未改变 RenderContext、BatchManager 或 GeometryRect。
- 未将 Eigen 从 PUBLIC 改为 PRIVATE；需等待剩余公共头迁移和独立 consumer。
- 未迁移上述 API 头的物理路径，本批只解除数学类型依赖。

## 验证

- Debug 全量构建：通过。
- 架构门禁、umbrella header、独立 MathTypes header check：通过。
- PublicLeafHeaders/Visibility/DragDrop/Tween/Utils 定向测试：70 passed / 0 failed。
- 全量测试：173 passed / 0 failed。
- 指标：`UiRuntime::current()` 302、PUBLIC include 2、PUBLIC link 3、queued dispatch sites 1。
- 既有 warning：`src/api/Timer.cpp` 忽略 nodiscard 返回值；本批未新增构建 warning。

## 残余风险

这是下一主版本允许的源码/ABI 收敛：仓外消费者若调用 Eigen 专有 Vec2 API 需要迁移。当前公开 API 的 Vec2/Rect 已不再是 Eigen 类型，但 Eigen 仍因其他内部聚合和 CMake 配置传播；后续应迁移 API 头物理路径、继续拆分 Types.hpp，并以 install/export consumer 验证后再收紧依赖。
