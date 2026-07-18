# WP3 Animation 公共头迁移

- 日期：2026-07-18
- 状态：completed
- 前置：公共 `Color`、`Vec2`、`TweenOptions`、Entity 与 Chains 已提供稳定路径

## 实施

- 新增稳定公共头 `include/ui/api/Animation.hpp`。
- 删除旧权威路径 `src/api/Animation.hpp`。
- `Animation.cpp`、`Factory.cpp` 和 `include/ui.hpp` 切换到稳定路径。
- `src/CMakeLists.txt` 公共头清单补入新路径。
- PublicLeafHeaders 增加 `<ui/api/Animation.hpp>` 直接包含。
- 架构门禁将 `Animation.hpp` 加入已迁移 API 头集合，禁止旧路径回归。

## 兼容性

公开动画函数、默认参数、integral/enum 转发模板、重载函数指针、EntityAction 和 Chain DSL 均保持不变。本批不修改 Tween 组件、TweenSystem、Runtime 或动画插值行为。

## 验证

- Debug 全量构建：通过。
- `example_ui_demo` 编译及链接：通过。
- 架构门禁、umbrella header 和公开头检查：通过。
- 全量测试：173 passed / 0 failed。
- 指标：302 / 2 / 3 / 1。
- Windows introspection 提示不影响构建 result code 0；本批未新增 warning。

## 残余风险

`Entity.hpp` 仍转发到源码树 `common/EntityTypes.hpp`，因此本批不代表 install-only consumer 或 CMake PUBLIC 边界闭环。剩余 Canvas、Controls、Factory、Utils 的依赖面更复杂；Canvas 是下一低风险候选，但公开 `Painter` 契约应先补最小测试。