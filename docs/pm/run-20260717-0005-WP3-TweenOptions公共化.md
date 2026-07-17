# 项目经理协调记录 - WP3 TweenOptions 公共化

- 时间：2026-07-17 00:05
- 输入来源：用户指定 WP3 下一最小批次；延续公共叶子类型与 SDK 边界解耦主线
- 本轮范围：仅创建协调记录，定义实现与验证工作包；本轮不修改生产代码、测试或构建配置，不执行构建/测试
- 验收标准：`ui::animation::TweenOptions` 的唯一权威定义迁至 `include/ui/TweenOptions.hpp`；旧头仅兼容转发；公共与内部使用切到公共头；默认值、类型性质及架构边界均有自动化验证；ABI、字段、默认值和既有 Animation API 签名保持不变

## 工作包

| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | 影响范围与边界基线核验 | 项目经理（只读） | 用户约束；`TweenOptions` 定义、显式 include 与现有公共叶子测试/门禁 | 本记录中的最小文件范围、风险与验收规格 | 完成 |
| 2 | 公共叶子类型与兼容转发落地 | 代码工厂 | 本记录“工作包 #2” | 源码变更及交付报告（必须列出变更文件） | 完成 |
| 3 | 定向构建、测试与架构门禁闭环 | 测试构建闭环 | 工作包 #2 交付报告；本记录“工作包 #3” | 测试报告、阶段结果、问题日志新增条数 | 完成 |

## 工作包 #2：实现派发规格

- **目标**：新增稳定公共叶子头 `include/ui/TweenOptions.hpp`，将 `ui::animation::TweenOptions` 的原样定义迁入；把 `src/common/Animation.hpp` 缩减为兼容转发头；令公开 Animation API 和内部直接使用方包含公共头；增加公共叶子契约测试和专用架构门禁。
- **依据**：当前权威定义位于 `src/common/Animation.hpp`，其字段均为标量或 `ui::policies` 枚举；当前显式包含旧头的生产文件仅有 `src/api/Animation.hpp` 与 `src/helper/Helper.hpp`。下游通过这些头获得类型的调用点不需要为迁移而改写。
- **只允许触达（预期最小集合）**：
  - 新增 `include/ui/TweenOptions.hpp`
  - `include/ui.hpp`
  - `src/common/Animation.hpp`
  - `src/api/Animation.hpp`
  - `src/helper/Helper.hpp`
  - `src/CMakeLists.txt`
  - `tests/unittest/test_PublicLeafHeaders.cpp`
  - `tools/check_architecture_boundaries.py`
- **禁止文件/事项**：
  - `src/api/Animation.cpp`、`src/api/Factory.cpp`、`src/systems/ActionSystem.hpp`、`tests/unittest/test_TweenSystem.cpp` 等仅通过上游头获得 `TweenOptions` 的调用点，除非出现可复现的独立编译失败并先升级确认。
  - `src/common/Types.hpp`、`src/common/components/Animation.hpp`、其他 Animation API、renderer、第三方目录及无关文档。
  - 不改 `Vec2`、`Color`、Eigen 相关类型或函数签名；不迁移整个 Animation API；不增加新抽象或依赖；不做无关重命名/格式化。
  - 不修改 `TweenOptions` 的命名空间、字段顺序、字段名、字段类型、默认成员初始化值或可见性；不引入构造函数、虚函数、基类、私有成员或 ABI 包装层。
  - 不收紧或扩张 CMake 的 PUBLIC include/link 传播范围；`src/CMakeLists.txt` 只允许更新头文件清单。
- **实现约束**：
  1. `include/ui/TweenOptions.hpp` 必须可独立包含，只依赖稳定公共头 `ui/Policies.hpp` 与必要的标准库头；不得包含 `src/`、`common/`、Eigen、EnTT、SDL 或其他 Animation API 头。
  2. 新头内定义保持完全一致：`duration = 200.0F`、`easing = policies::Easing::EASE_OUT_QUAD`、`mode = policies::Play::ONCE`、`autoCleanup = true`，且字段声明顺序不变。
  3. `src/common/Animation.hpp` 仅保留兼容所需的 `#pragma once`、公共头 include 以及允许的兼容说明注释；不得再声明/定义/别名化 `TweenOptions`，不得包含其他内部头。
  4. `src/api/Animation.hpp` 和 `src/helper/Helper.hpp` 必须直接包含 `ui/TweenOptions.hpp`，不得继续依赖兼容转发头。
  5. `include/ui.hpp` 应从稳定路径导出公共叶子头，但不得借此调整其他导出顺序或迁移其他 API。
  6. 架构门禁应形成显式规则，而非仅依赖通用正则：禁止 `src/api/**/*.hpp` 与 `include/**/*.hpp` 包含 `common/Animation.hpp`（兼容头自身不在这两个目录）；同时验证 `src/common/Animation.hpp` 只承担到 `ui/TweenOptions.hpp` 的兼容转发。门禁失败信息应能区分“公共头反向依赖旧头”和“旧头不再是纯转发”两类问题。
- **验收标准**：
  - `struct TweenOptions` 的权威定义全仓唯一，位于 `include/ui/TweenOptions.hpp`，命名空间仍为 `ui::animation`。
  - 旧 include 路径 `common/Animation.hpp` 仍可提供同一类型，且无副本定义、别名替代或额外内部依赖。
  - `src/api/Animation.hpp` 与内部直接使用方均直接依赖公共头；`src/api` 和 `include` 中不存在对 `common/Animation.hpp` 的 include。
  - `test_PublicLeafHeaders.cpp` 直接 include `<ui/TweenOptions.hpp>`，静态验证 `std::is_standard_layout_v` 与 `std::is_trivially_copyable_v`，并逐字段验证默认值完全不变。
  - 现有 Animation 函数、链式 DSL、`Vec2`/`Color`/Eigen 签名及二进制布局相关字段声明不变；不得出现整个 Animation API 迁移。
  - `src/CMakeLists.txt` 同时登记新公共头与旧兼容头，且 PUBLIC include/link 配置无变化。
- **失败后的升级条件**：若需要修改字段/默认值/签名、引入构造函数或别名层、调整 CMake PUBLIC 传播、迁移 `Vec2`/`Color`/Eigen、触达最小集合外生产调用点，或无法用确定性规则验证旧头为纯转发，则停止扩展并回报项目经理/用户，不自行扩大范围。

## 工作包 #3：验证派发规格

- **任务**：执行 configure（仅在现有构建目录配置与测试开关不足时）、build、定向单测和架构门禁；不得以验证名义重构业务代码。
- **目标**：
  - preset/build-dir：优先复用 `D:/test/VMP-ui/build` 的 Debug 配置；若 `ENABLE_BUILD_TESTS` 未启用则升级说明，不擅自改变持久配置。
  - targets：`ui_architecture_boundary_check`、`ui_public_header_check`、`ui_api_tests`、`ui_ecs_tests`。
  - tests：`PublicLeafHeadersTest.*`、新增 `TweenOptions` 公共叶子测试、现有 Tween/Animation 相关测试；随后执行当前已配置的全量 CTest，生命周期测试按其既有串行与资源锁策略运行。
  - 报告期望：记录命令、阶段状态、通过/失败数量、失败测试名、架构门禁输出摘要、问题日志新增条数及报告路径。
- **失败策略**：
  - 测试筛选名、报告脚本或门禁规则的明显笔误可在工作包 #2 允许文件内最小修正一次。
  - 若失败涉及生产行为、API/ABI、字段默认值、范围外 include 链、CMake PUBLIC 配置或既有生命周期问题，停止修复并写问题日志，升级项目经理。
- **验收标准**：
  - 四个目标均成功；公共叶子测试、Tween/Animation 相关测试及全量 CTest 均通过。
  - 架构门禁明确证明：公共头不反向包含旧内部头，旧头为纯兼容转发，且既有架构债务未增长。
  - 聊天回执必须包含报告路径、各阶段结果及问题日志新增条数；缺任一项视为未验收。
- **失败后的升级条件**：同一阶段修复后仍失败，或发现失败不属于本批允许文件范围，则不进行第二轮源码扩张，直接提交完整问题日志供用户决策。

## 最小文件范围说明

1. **类型权威与兼容**：`include/ui/TweenOptions.hpp`、`src/common/Animation.hpp`。
2. **直接依赖切换**：`src/api/Animation.hpp`、`src/helper/Helper.hpp`。
3. **公共导出与构建登记**：`include/ui.hpp`、`src/CMakeLists.txt`。
4. **契约与边界验证**：`tests/unittest/test_PublicLeafHeaders.cpp`、`tools/check_architecture_boundaries.py`。
5. 当前搜索到的其余 `TweenOptions` 使用点通过上述直接头获得完整定义，暂不纳入；只有独立编译证据才能触发升级，不得预防性扩包。

## 主要风险

1. **ODR/重复定义**：若旧头保留结构体副本或另建同名包装，会造成权威定义不唯一或包含组合失败。
2. **ABI/默认值漂移**：字段重排、枚举默认值误写、增加构造函数或访问控制都可能改变布局、聚合性质或调用行为。
3. **伪公共化**：新公共头若继续包含 `common/Animation.hpp`，或公开 API 仍经旧头间接取得类型，边界债并未消除。
4. **兼容转发回潮**：仅检查公共目录 include 不足以阻止旧头未来重新承载实现，必须同时验证旧头内容约束。
5. **门禁误报/漏报**：include 写法可使用引号、尖括号或 `src/common/...` 前缀；规则需覆盖这些形式，并避免扫描 `build/`、`third_party/`。
6. **范围膨胀**：`Animation.hpp` 仍暴露 `Vec2`/`Color`/Eigen 是已知后续问题，本批只公共化纯值类型，不以“清理依赖”为由迁移整个 API 或收紧 CMake。

## 调度时间线

- 2026-07-17 00:05：按用户明确的编排请求进入 Full PM 路径；只读核验 `TweenOptions` 权威定义、显式旧头 include、公共叶子测试和架构门禁现状。
- 2026-07-17 00:05：完成最小文件范围、代码工厂派发闸门、测试构建目标、失败回路与风险定义；未修改生产/测试，未执行构建。
- 2026-07-17：新增公共 `TweenOptions.hpp`，旧头收敛为纯兼容转发，公开/内部直接调用点切换到稳定头。
- 2026-07-17：新增公共值类型契约测试与架构门禁；Debug 构建通过，定向 7/7、全量 167/167。

## 待用户决策

- [x] 无；本最小批次已实施并验收。

## 结论

- 状态：完成
- 关键产物：`include/ui/TweenOptions.hpp`、旧路径纯转发、公共叶子契约测试和专用门禁
- 实际变更：新增 `include/ui/TweenOptions.hpp`；修改 `include/ui.hpp`、`src/common/Animation.hpp`、`src/api/Animation.hpp`、`src/helper/Helper.hpp`、`src/CMakeLists.txt`、`tests/unittest/test_PublicLeafHeaders.cpp`、`tools/check_architecture_boundaries.py`
- 验收结果：权威定义唯一；命名空间、字段顺序、类型和四项默认值不变；standard-layout/trivially-copyable；旧 include 路径继续可用；公共头不再包含 `common/Animation.hpp`
- 验证结果：Debug 构建、公开头检查和架构门禁通过；定向测试 7/7；全量测试 167/167；架构指标 302/2/3/0；问题日志新增 0 条
- 明确未做：Vec2/Color/Eigen 迁移、整个 Animation API 迁移、CMake PUBLIC include/link 收紧、独立 install consumer
- 下一工作包：继续选择不依赖 Eigen 决策的公共叶子类型，或正式制定 Vec2/Color 公共 ABI 方案
