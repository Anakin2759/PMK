# 公共 `ui::Vec2` 契约与 Eigen 边界最小工作包

- 日期：2026-07-18
- 输入来源：`docs/architecture/decision-public-math-types-20260718.md`；全仓 `Vec2`/`Rect`/Eigen 使用核验；现有公共头与单元测试核验
- 作用范围：公共数学值类型、直接暴露 `Vec2`/`Rect` 的 API 头、少量 Eigen 运算边界、API 编译测试
- 改动类型：公开类型替换（源码/ABI 破坏性），采用兼容表面与集中适配器控制风险；不是全仓数学实现重写

## 1. 影响摘要

### 1.1 核验结论

第一方目录 `src/`、`include/`、`tests/`、`example/` 中，`Vec2` 约有 355 处命中、分布于 39 个文件。绝大多数使用只依赖以下能力：

- `{x, y}` 或 `(x, y)` 构造、默认构造、复制/赋值；
- `x()` / `y()` 的 const 读取以及返回 `float&` 后的原地写入；
- 向量 `+`、`-`、一元 `-`、`+=`；
- 向量乘标量；
- 精确 `==`（仅零位移快速判断）；
- 作为组件字段、事件载荷、callback 参数、`optional`/`vector` 元素。

Eigen 特有能力集中且很少：

| 模式 | 第一方命中 | 位置 | 结论 |
|---|---:|---|---|
| `normalized()` / `normalize()` / `norm()` | 0 | 无 | 不进入公共契约 |
| `squaredNorm()` | 2 | `src/systems/StateSystem.cpp` | 改为自有自由函数 `LengthSquared()`，不复制 Eigen 成员 API |
| `cwiseProduct()` | 2 | `src/systems/render/RenderFrame.cpp` | 保持为内部 Eigen 运算，入口显式 `ToEigen()` |
| `cwiseMin()` | 1 | `src/renderers/IconRenderer.hpp` | 保持为内部 Eigen 运算，组件值显式 `ToEigen()` |
| Vec2 与矩阵/仿射变换 | 2 个入参 | `src/common/Types.hpp::MakeTransform2D` | 仅在内部 helper 中显式转换；未发现其他第一方矩阵乘 Vec2 |
| 直接 Eigen 构造写回 Vec2 组件 | 2 | `src/core/WindowSync.hpp` | 改为直接构造 `ui::Vec2`，不经过 Eigen |
| Vec2 组件隐式赋给 Eigen 局部量 | 若干集中点 | `IconRenderer.hpp`、`RenderFrame.cpp` | 统一经内部适配器转换 |

`Rect` 已经出现在公开 `Utils.hpp` 返回值中，因此若本批只建立 `Vec2` 而保留 `Rect` 在 `src/common/Types.hpp`，`Utils.hpp` 仍必须包含 Eigen 聚合头，无法形成可独立验收的公共叶子头。故推荐本批同时迁出**最小 `Rect` 契约**；这不是扩展几何功能，而是闭合 Vec2 的公共头边界。

### 1.2 `Types.hpp` 依赖现状

`src/common/Types.hpp` 当前同时承担：

1. Eigen `Vec2/3/4`、矩阵和 Transform alias；
2. `Rect`；
3. `EdgeInsets`；
4. `MakeVec*`、`Lerp`、矩阵/Transform helper；
5. callback wrapper；
6. 对 `ui/Color.hpp` 的 legacy 聚合导出。

共有 38 个第一方文件直接包含某个 `Types.hpp`，其中 7 个是 `src/api/*.hpp` 公共 API 头。这个聚合职责是 Eigen 传递依赖的根因，但一次拆完 `Types.hpp` 风险过高。本工作包只迁出 `Vec2`/`Rect`，保留 Vec3/Vec4/矩阵/Transform/EdgeInsets/callback 等原职责。

### 1.3 现有测试面

- `tests/unittest/test_PublicLeafHeaders.cpp` 已验证公共叶子值类型的 standard-layout、trivially-copyable、尺寸/对齐与 constexpr 行为，可沿用该模式。
- `test_UmbrellaHeader.cpp` 和 `ui_public_header_check` 验证聚合头可编译，但当前链接 `ui`，不能证明 Eigen 不在公共叶子头依赖闭包中。
- `test_DragDrop.cpp`、`test_TweenSystem.cpp` 覆盖 Vec2 callback、构造、赋值和插值。
- `test_Visibility.cpp` 直接用 `Eigen::Vector2f` 写入 Vec2 组件；类型切换后应改为 `ui::Vec2`，它属于测试迁移而非兼容需求。
- 当前没有独立的 Vec2/Rect 契约测试，也没有“不提供 Eigen include path”的公共数学头编译检查。

## 2. 推荐工作包

### 2.1 工作包边界

推荐工作包名称：**WP6A — Public Vec2/Rect Contract + Narrow Eigen Adapters**。

目标是在一次可构建、可回滚的提交中完成：

1. 建立不依赖第三方库的 `ui::Vec2` 与最小 `ui::Rect` 公共叶子头；
2. 让直接暴露 Vec2/Rect 的公共 API 头不再包含 `common/Types.hpp`；
3. 用单一内部适配器处理本批实际遇到的 Eigen Vector2f 边界；
4. 通过兼容的 `x()/y()` 和基础运算让绝大多数内部调用点零改动通过；
5. 不迁移 RenderContext、BatchManager 及渲染数学实现，不收紧 CMake Eigen 可见性。

这不是“一次性全仓替换”：不批量改写访问语法，不把内部 Eigen 类型全部替换为 `ui::Vec2`，不重构 Vec3/Vec4/矩阵系统，只修理由 alias 变为值类型后真实发生的类型边界。

## 3. 最小公共契约

建议新增 `include/ui/MathTypes.hpp`，包含以下契约。

### 3.1 `ui::Vec2`

必须提供：

- 两个连续且顺序稳定的 `float` 标量存储；
- `constexpr Vec2() noexcept`，值初始化为 `(0, 0)`；
- `constexpr Vec2(float x, float y) noexcept`；
- `constexpr float& x() noexcept` / `constexpr float x() const noexcept`；
- `constexpr float& y() noexcept` / `constexpr float y() const noexcept`；
- `operator+`、二元 `operator-`、一元 `operator-`；
- `operator+=`、`operator-=`；
- `Vec2 * float`、`float * Vec2`、`operator*=`；
- 精确 `operator==`（由 C++23 可默认实现）；
- `constexpr float LengthSquared(const Vec2&) noexcept` 自由函数。

编译期不变量：

- `std::is_standard_layout_v<Vec2>`；
- `std::is_trivially_copyable_v<Vec2>`；
- `sizeof(Vec2) == 2 * sizeof(float)`；
- `alignof(Vec2) == alignof(float)`。

明确不提供：

- Eigen 构造函数、转换运算符或模板化 vector-like 构造；
- `normalized()`、`norm()`、`squaredNorm()`、`dot()`、`cwise*()`；
- 表达式模板、矩阵乘法、索引/`data()` API；
- 向量除法、分量乘除等当前公共 API 未证明需要的能力。

理由：保留 `x()/y()` 可将迁移限制在真正的 Eigen 边界；提供 Eigen 风格高级成员会永久扩大公共契约，并使后续继续依赖 Eigen 语义。

### 3.2 `ui::Rect`

本批只提供现有公开 API 与已知调用需要的契约：

- `Vec2 position`、`Vec2 size`；
- 默认构造、`Rect(float x, float y, float width, float height)`、`Rect(Vec2 position, Vec2 size)`；
- `x()`、`y()`、`width()`、`height()`；
- `left()`、`top()`、`right()`、`bottom()`；
- `contains(Vec2)`，保持闭区间语义；
- standard-layout、trivially-copyable、稳定四 `float` 布局的断言。

`topLeft/topRight/bottomLeft/bottomRight/center/intersects/expanded` 当前第一方无外部调用，可暂不承诺。`shrunk(Vec4)` 会重新引入 Eigen Vec4，必须留在内部兼容层或删除未用实现，不能进入公共 `Rect`。

`GeometryRect` 与 `VerticalScrollbarGeometry` 本批保持不变；不在同一工作包合并 `GeometryRect` 和 `Rect`，避免波及已完成 WP3 的公开结构布局。

## 4. 内部转换策略

新增内部头 `src/common/EigenConversions.hpp`，仅允许内部目标包含，提供窄转换：

- `Eigen::Vector2f ToEigen(const ui::Vec2&) noexcept`；
- `ui::Vec2 FromEigen(const Eigen::Vector2f&) noexcept`。

建议放入 `ui::detail::eigen` 命名空间。两者必须显式函数调用，不提供隐式构造或 conversion operator。

使用规则：

1. API/ECS 值进入 RenderContext、BatchManager、Transform 或 `cwise*` 前调用 `ToEigen()`；
2. Eigen 计算结果确需写回公开/ECS 值时调用 `FromEigen()`；
3. 若只是 SDL/Yoga 标量输入输出，直接读写 `x()/y()`，不要无意义地往返 Eigen；
4. 同一函数中完成转换，避免长期同时保存等价的 public/internal 两份状态；
5. 不为 Rect 增加 Eigen Rect 类型；矩形继续按四个标量或两个 Vec2 处理。

本批预期适配点：

- `RenderFrame.cpp`：Position/Size/Scale/RenderOffset/ScrollArea 的 `ui::Vec2` 在进入 Eigen 渲染上下文及 `cwiseProduct` 前转换；
- `IconRenderer.hpp`：Icon 组件 size/UV 转为 Eigen 局部量；
- `WindowSync.hpp`：SDL 标量直接构造 `ui::Vec2`，不使用 Eigen；
- `Types.hpp::MakeTransform2D`：translation/scale 经 `ToEigen()` 传入 Eigen Transform；
- `StateSystem.cpp`：两个 `squaredNorm()` 改为 `LengthSquared()`，该处无需进入 Eigen。

## 5. 准确文件范围

### 5.1 必须新增

| 文件 | 目的 |
|---|---|
| `include/ui/MathTypes.hpp` | 自有 Vec2/Rect 公开契约；无 Eigen/SDL/EnTT/Runtime include |
| `src/common/EigenConversions.hpp` | 唯一的 Vec2 ↔ Eigen::Vector2f 内部适配器 |
| `tests/support/PublicMathTypesHeaderCheck.cpp` | 只包含 `<ui/MathTypes.hpp>` 的独立编译检查与静态断言 |

### 5.2 必须修改

| 文件 | 最小修改 |
|---|---|
| `include/ui.hpp` | 导出 `ui/MathTypes.hpp` |
| `src/common/Types.hpp` | 包含公共 MathTypes；删除 Vec2 alias 和 Rect 定义；保留其他 Eigen alias；`MakeTransform2D` 显式转换 |
| `src/api/Animation.hpp` | 用 `ui/MathTypes.hpp` + 已需公共叶子头替代 `common/Types.hpp` |
| `src/api/Canvas.hpp` | 同上 |
| `src/api/Controls.hpp` | 同上 |
| `src/api/Factory.hpp` | 同上；仅为 CreateArrow 引入 MathTypes |
| `src/api/Image.hpp` | 同上 |
| `src/api/Utils.hpp` | 同上；Vec2/Rect 来自 MathTypes |
| `src/api/Table.hpp` | 删除无关 `common/Types.hpp`，直接包含 `ui/Color.hpp` |
| `src/core/WindowSync.hpp` | 两处组件回写改为 `ui::Vec2` |
| `src/renderers/IconRenderer.hpp` | 组件 Vec2 到 Eigen 局部量的显式转换 |
| `src/systems/render/RenderFrame.cpp` | 集中处理 ECS Vec2 与 Eigen RenderContext 的转换和分量运算 |
| `src/systems/StateSystem.cpp` | 两处 `squaredNorm()` 改为 `LengthSquared()` |
| `tests/unittest/test_PublicLeafHeaders.cpp` | 增加 Vec2/Rect 布局、constexpr、运算与闭区间契约测试 |
| `tests/unittest/test_Visibility.cpp` | 测试数据从 Eigen::Vector2f 改为 ui::Vec2 |
| `tests/unittest/CMakeLists.txt` | 将无 Eigen include path 的 `PublicMathTypesHeaderCheck.cpp` 加为独立 object check，并挂到 `ui_api_tests` 依赖 |

### 5.3 仅在编译证明需要时允许触达的封闭清单

以下文件存在 Eigen 接口，但按当前核验应通过标量构造或 RenderContext 的 Eigen 类型继续工作；只有编译器证明存在隐式 Vec2→Eigen 依赖时才允许做局部 `ToEigen()`，不得顺势改接口：

- `src/renderers/ImageRenderer.hpp`
- `src/renderers/CanvasRenderer.hpp`
- `src/renderers/TableRenderer.cpp`
- `src/renderers/TextRenderer.hpp`

若错误扩散到此清单之外，应停止本工作包并回到边界核验，不允许通过给 `ui::Vec2` 增加隐式 Eigen 转换来“快速修通”。

## 6. 本批禁止触达

- 不把 `core::RenderContext::{position,size}` 改为 `ui::Vec2`；
- 不修改 `BatchManager` 的 Eigen 顶点/UV 接口或 `RenderTypes`；
- 不迁移 `Vec3`、`Vec4`、Color 内部向量、Mat2/3/4、Transform2D/3D；
- 不拆完 `src/common/Types.hpp`，不迁移全部 38 个内部 include；
- 不将 Eigen 从 `target_link_libraries(ui PUBLIC ...)` 改为 PRIVATE；该动作必须等所有公共头闭包清理和 install/export consumer 测试完成；
- 不全仓改写 `x()/y()` 为字段访问；
- 不加入 Eigen 兼容构造、隐式 conversion operator 或通用模板适配；
- 不重写 Canvas 曲线/渲染几何算法；
- 不合并、重命名或改变 `GeometryRect`、`VerticalScrollbarGeometry` 布局；
- 不清理与本契约无关的公共 API 头内部依赖、CMake 导出结构或历史 helper。

## 7. 修改规划与依赖

| 优先级 | 步骤 | 文件范围 | 依赖 | 完成条件 |
|---|---|---|---|---|
| P0 | 固定值类型契约 | `MathTypes.hpp`、`test_PublicLeafHeaders.cpp`、header check | 无 | 布局、constexpr、基础运算、Rect 闭区间测试通过 |
| P0 | 接入兼容聚合层 | `Types.hpp`、`EigenConversions.hpp` | 值类型契约 | 其他 Eigen alias 保持；无隐式第三方转换 |
| P0 | 清理直接公开头 | 7 个 Vec2/Rect API 头、`Table.hpp`、`ui.hpp` | 值类型契约 | 这些头不直接包含 `common/Types.hpp`，签名中的 Vec2/Rect 为自有类型 |
| P1 | 修复真实 Eigen 边界 | `WindowSync.hpp`、`IconRenderer.hpp`、`RenderFrame.cpp`、`StateSystem.cpp` | 转换适配器 | 不新增 Eigen 到公开头；内部运算行为不变 |
| P1 | 测试迁移与闭环 | `test_Visibility.cpp`、测试 CMake | 前述步骤 | API/ecs/unit 构建和测试通过；纯公共数学头可在无 Eigen include path 下编译 |

## 8. 验收矩阵

| 维度 | 验收项 | 预期结果 |
|---|---|---|
| 类型布局 | standard-layout / trivially-copyable / sizeof / alignof | Vec2 为 2 floats；Rect 为 4 floats |
| 构造 | 默认、`{x,y}`、`(x,y)`、复制/赋值 | constexpr 可用，默认全零 |
| 访问 | const/non-const `x()/y()` | 读取正确；返回引用可原地更新 |
| 运算 | `+`、`-`、一元 `-`、`+=`、`-=`、两侧标量乘、`*=`、`==` | 与当前调用语义一致 |
| 长度平方 | `LengthSquared({3,4})` | 25；StateSystem 点击/拖动阈值行为不变 |
| Rect | 构造、边界访问、`contains` | 左上及右下边界均包含；越界排除 |
| 公共依赖 | 独立编译 `PublicMathTypesHeaderCheck.cpp`，仅提供项目 `include/` | 无需 Eigen/SDL/EnTT include path |
| 公共签名 | Animation/Canvas/Controls/Factory/Image/Utils | 不出现 Eigen 类型，不包含 `common/Types.hpp` |
| 内部边界 | 搜索 public→Eigen 隐式转换 | 仅存在命名的 `ToEigen`/`FromEigen` 调用 |
| 回归 | `ui_api_tests` | 全过 |
| 回归 | `ui_ecs_tests`（重点 DragDrop/Tween/Visibility） | 全过 |
| 回归 | `ui_unit_tests` | 全过 |
| 构建 | `build Debug (CMake)` | 无新增编译错误/警告 |
| 静态核验 | 搜索第一方 `.normalized/.norm/.cwise*/squaredNorm` | Vec2 公共路径无 Eigen 特有成员；内部 cwise 仅在 Eigen 局部量 |

本批**不以**“Eigen 从 ui 的 PUBLIC link interface 消失”作为验收项；那是迁移顺序第 5/6 步的后续工作包，提前执行会把公共头闭包问题与类型替换问题耦合。

## 9. 风险与控制

| 风险 | 等级 | 控制措施 |
|---|---|---|
| Eigen alias 替换是 ABI/源码破坏，隐藏调用点编译失败 | 高 | 保留 x()/y() 与基础算术兼容面；限定适配文件；全量构建但不全量重写 |
| 为快速兼容复制 Eigen API，形成永久公共负担 | 高 | 明确负契约；用 `LengthSquared` 和内部 `ToEigen` 替代成员仿制 |
| RenderFrame 中 public/internal 向量混算导致坐标或缩放回归 | 高 | 转换集中在函数入口；保持 RenderContext/BatchManager 为 Eigen；重点运行渲染构建与现有生命周期测试 |
| `Rect::shrunk(Vec4)` 迁出时重新泄露 Eigen | 中 | 不进入公共 Rect；当前无第一方调用，留待后续 Types/EdgeInsets 清理 |
| 测试目标仍通过 `ui PUBLIC Eigen` 掩盖头依赖 | 中 | 增加不链接 ui、不给 Eigen include path 的纯 header object check |
| `GeometryRect` 与 `Rect` 重复引发顺手合并 | 中 | 明确禁止本批合并，保持 WP3 ABI |
| 新 Vec2 内存布局与编译器填充差异 | 低 | 双编译器 static_assert `sizeof/alignof`；只含两个 float 数据成员 |

## 10. SOLID / YAGNI 检查

- SRP：`MathTypes.hpp` 只定义公共值语义；`EigenConversions.hpp` 只处理第三方适配。
- OCP：后续内部新增 Eigen 运算通过适配器扩展，不修改 Vec2 以迎合 Eigen。
- LSP：无继承层次，不适用。
- ISP：公共 Vec2 不暴露 Eigen 表达式接口，只提供调用方已证明需要的最小面。
- DIP：公共 API 依赖自有值类型，内部实现依赖 Eigen；没有为此额外引入接口层。
- YAGNI：不实现 normalized/norm/cwise/矩阵乘法，不拆完整 Types，不迁移整个渲染栈。

引入两个新抽象的理由：公共值类型是已接受架构决策；内部转换函数是隔离第三方边界的唯一稳定接缝。不引入适配器的代价是散落手写 `Eigen::Vector2f{x(), y()}`，后续难以审计边界；不引入公共值类型则无法消除公开 API 的 Eigen ABI 耦合。

## 11. 后续工作（不属于本批）

1. 逐模块迁移剩余公共头闭包，清理 EnTT/Runtime/内部组件泄漏；
2. 拆分 `Types.hpp` 的 Vec3/Vec4、矩阵、EdgeInsets、callback 聚合职责；
3. 根据真实公共需求决定是否扩充 Rect，或最终统一 GeometryRect；
4. Eigen 改为 PRIVATE；
5. 增加 install/export consumer，在完全无 Eigen 的消费工程中编译 `<ui.hpp>`。

## 12. 待确认问题

1. `Rect` 当前属于已公开返回类型；本方案默认允许在同一主版本工作包内同步替换其 ABI，并只承诺已知最小契约。
2. `Rect` 的未使用便利方法是否被仓外消费者依赖无法由本仓核验；默认按决策中的“最小公共运算”不继续承诺，并在迁移说明列出替代写法。
3. 当前日期与 build 目录中的历史测试时间可能不一致；验收以重新配置/构建产生的结果为准，不采信旧产物。
