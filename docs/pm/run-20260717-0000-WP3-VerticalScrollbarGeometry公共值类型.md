# 项目经理协调记录 - WP3 VerticalScrollbarGeometry 公共值类型

- 时间：2026-07-17
- 输入来源：用户指定批次；`docs/architecture/ARCHITECTURE_REVIEW_AND_ROADMAP_2026-07-11.md` 的 WP3 公共类型解耦主线
- 本轮范围：只做影响核验、实现拆包与验收定义；源码实现及构建由主 Agent 后续执行
- 验收标准：`VerticalScrollbarGeometry` 成为 `include/ui/` 下仅含标量的公共值类型；内部 component/Eigen DTO 被移除；纵向滚动条计算、命中与可见性语义不变；不处理 `Utils.hpp` 其余 `Vec2`/`Rect`，不重构 renderer

## 工作包
| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | 当前依赖与行为基线核验 | 项目经理（只读） | 路线图 WP3、`VerticalScrollbarGeometry` 全部引用 | 本记录的影响文件、风险与验收项 | 完成 |
| 2 | 公共标量 DTO 与内部适配落地 | 主 Agent | 本记录工作包 #2 边界 | `include/ui/Geometry.hpp` 及生产/消费适配 | 完成 |
| 3 | 定向构建与测试闭环 | 主 Agent | 工作包 #2 交付报告 | Debug 构建与定向/全量测试结果 | 完成 |

## 工作包 #2：实现派发规格

- **目标**：新增 `ui::VerticalScrollbarGeometry` 公共值类型；数据成员只使用 `bool`/`float` 等标量。将 `GetVerticalScrollbarGeometry()` 的返回类型及调用点切换到该类型，并删除 `ui::components::VerticalScrollbarGeometry`。
- **依据**：路线图“WP3 构建与 SDK 边界”及当前主线“继续 WP3 的公共类型解耦”；现实现位于 `src/common/components/Layout.hpp`，其中四个 `Rect` 间接携带 Eigen `Vec2`。
- **只允许触达（预期最小集合）**：
  - 新增 `include/ui/VerticalScrollbarGeometry.hpp`
  - `include/ui.hpp`
  - `src/CMakeLists.txt`
  - `src/common/components/Layout.hpp`
  - `src/api/Utils.hpp`
  - `src/api/Utils.cpp`
  - `src/helper/Helper.hpp`
  - `src/systems/StateSystem.cpp`
  - `tests/unittest/test_Utils.cpp`
  - `tests/unittest/test_PublicLeafHeaders.cpp`
  - 如需固化边界，允许最小更新 `tools/check_architecture_boundaries.py`
- **禁止文件/事项**：renderer 文件；`src/common/Types.hpp`；其他 `Vec2`/`Rect` API；CMake PUBLIC include/link 收紧；新依赖；横向滚动条算法；滚动条样式常量；无关重命名、格式化或架构重构。
- **实现约束**：
  1. 公共头自包含，不 include `common/Types.hpp`、Eigen、EnTT 或任何 `src/` 头。
  2. 用标量表达 container/viewport/track/thumb 的 `x/y/width/height` 以及现有高度与滚动范围数据；默认值必须保持“不可见、全零”。
  3. `Utils.cpp` 保留现有公式、常量、clamp、最小 thumb、inset 和 `visible` 置位时机，仅做 DTO 写入适配。
  4. `StateSystem.cpp` 的 track/thumb 命中必须保持现有 `Rect::contains()` 的闭区间语义：左右、上下边界均命中。
  5. 不要求 renderer 消费新 DTO；renderer 现有行为与文件保持不变。
- **验收标准**：
  - `VerticalScrollbarGeometry` 的权威定义仅存在于 `include/ui/`，命名空间为公共 `ui`；旧 component 定义及前置声明为零。
  - 全仓符号引用均返回/使用公共类型，公共头依赖图不引入 Eigen/EnTT/内部头。
  - 单元测试覆盖：无 ScrollArea、非纵向、内容不溢出、正常溢出、scroll offset 两端 clamp、最小 thumb、track/thumb 边界命中及边界外不命中。
  - `test_PublicLeafHeaders.cpp` 直接 include 新头并验证其可独立使用；建议静态验证 standard-layout、trivially-copyable 和默认全零/不可见契约。
  - 目标 `ui`、`ui_public_header_check`、`ui_ecs_tests`、`ui_api_tests` 构建通过；新增/相关测试通过；架构边界检查无新增债务。
- **失败后的升级条件**：若保持命中行为必须修改 renderer、必须决定通用 `Rect`/`Vec2` 公共 ABI、必须改变滚动条公式/样式常量，或出现范围外调用者，则停止扩展并回报主 Agent/用户，不在本批次自行决策。

## 主要风险

1. **命中边界漂移**：把 `Rect::contains()` 改成手写比较时，`<=` 被误改为 `<` 会改变边缘点击行为。
2. **字段映射错误**：四组矩形展开为标量后，track/thumb 的 x、宽度或 inset 易错位；应以现有公式逐字段对照。
3. **可见性时机漂移**：当前仅在具备纵向滚动、内容溢出且 trackHeight 大于零后置 `visible=true`；提前置位会改变 hover/press。
4. **隐式公共依赖残留**：新头若复用 `Rect` 或 include `Utils.hpp`，仍会把 Eigen 带入边界，批次即未达标。
5. **范围膨胀**：`Utils.hpp` 仍有其他 `Vec2`/`Rect` 是已知后续决策，本批不得顺手迁移；renderer 也不得为“统一 DTO”而改造。

## 调度时间线
- 2026-07-17：按 Full PM 路径建立记录；只读核验路线图、定义、生产者和 StateSystem 消费点。
- 2026-07-17：形成主 Agent 可直接执行的文件范围、行为约束、风险和验收门槛；未修改源码，未执行构建。
- 2026-07-17：主 Agent 新增公共 `GeometryRect`/`VerticalScrollbarGeometry`，删除内部伪 component DTO，并保持 Utils 公式与 StateSystem 闭区间命中语义。
- 2026-07-17：Debug 构建成功，架构门禁通过；相关定向测试 40/40，Utils 定向测试 19/19，全量测试 145/145。

## 待用户决策
- [ ] 无；若实现中触发“失败后的升级条件”，再请求范围/设计决策。

## 结论
- 状态：completed
- 关键产物：`include/ui/Geometry.hpp`、公共 DTO 生产/消费适配、几何边界契约测试
- 下一工作包：形成 `Types.hpp`/Eigen 的公共 SDK 类型决策；在此之前不强推剩余 API 头迁移或 CMake PUBLIC 收紧
