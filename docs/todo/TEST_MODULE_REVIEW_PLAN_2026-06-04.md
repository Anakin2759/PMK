# 测试模块锐评与改进规划

> 日期：2026-06-04  
> 范围：`tests/unittest/`、测试构建入口、当前 Google Test 用例组织  
> 结论先行：当前测试不是“没有”，而是“有一批能保命的回归点”；但整体仍停留在补洞式覆盖，离一个 UI/ECS 框架需要的测试体系还有明显距离。

## 1. 当前测试模块速写

当前测试入口集中在 `tests/unittest/CMakeLists.txt`，产物为单一可执行文件 `ui_tests`。测试文件共 14 个，约 2300 行左右，覆盖方向如下：

| 文件 | 主要覆盖点 | 粗评 |
|---|---|---|
| `test_MainWindow.cpp` | DSL、Factory、Hierarchy 基础集成 | 有价值，但更像“组件状态快照测试” |
| `test_UiRuntime.cpp` | `UiRuntimeScope`、Registry/Dispatcher 隔离 | 当前质量较高，是测试体系里少数抓住架构风险的用例 |
| `test_TaskChain.cpp` | 帧任务、事件顺序、延迟渲染任务 | 能防回归，但还没覆盖异常/边界调度 |
| `test_TweenSystem.cpp` | Tween 流水线、运行时隔离 | 覆盖了主路径，边界和组合场景偏少 |
| `test_ThemeSystem.cpp` | 主题状态、控件状态、样式覆盖 | 文件过大，重复断言多，维护成本已经偏高 |
| `test_DragDrop.cpp` | 拖拽 API、Drop 事件、循环阻断 | 用例方向正确，但与 `ActionSystem` 绑定较紧 |
| `test_HierarchyCoverage.cpp` | 父子关系、重挂载、遍历 | 基础扎实，适合抽出公共 Fixture |
| `test_VisibilityCoverage.cpp` | 显示、脏标记、颜色、边框 | Coverage 味很重，容易变成实现细节锁死 |
| `test_UtilsCoverage.cpp` | dirty 标记、窗口祖先查找、别名查询 | 有必要，但命名和断言粒度偏“覆盖率驱动” |
| `test_BatchManager.cpp` | 批处理合并、裁剪、透明度 | 单元边界清晰，是比较健康的单测 |
| `test_TextUtils.cpp` | UTF-8 边界、换行、尾部裁剪 | 健康，但缺少 CJK/emoji/组合字符压力 |
| `test_ResourceProviderCoverage.cpp` | 内嵌字体/图标资源 | 冒烟性质强，缺少错误路径精度 |
| `test_result.cpp` | `Result`、错误码、formatter | 必要且清晰 |
| `test_UmbrellaHeader.cpp` | 公开总头可包含性 | 很薄，但对公开 API 有意义 |

## 2. 锐评

### 2.1 最大问题：测试在追“覆盖点”，不是追“风险闭环”

多个文件以 `Coverage` 命名，测试内容也明显围绕“这个 setter 有没有改组件字段”“这个 no-op 会不会崩”展开。这类测试有用，但风险价值有限：

- 能证明 API 写了字段，不能证明 UI 行为正确。
- 能防止重构时字段名/默认值变动，却可能锁死内部实现。
- 对 ECS/UI 框架真正高风险的布局、输入、渲染、生命周期、跨系统协作覆盖不足。

一句话：当前测试像“仓库货架盘点”，不像“系统验收”。

### 2.2 单一 `ui_tests` 可执行文件会逐渐拖垮反馈速度

所有测试都塞进一个 `ui_tests`：

- 编译粒度粗，任意测试文件变更都可能引发较大链接成本。
- 运行粒度粗，不利于区分 fast unit、integration、render/headless、regression。
- 后续一旦加入 SDL window/GPU/字体渲染相关测试，整个测试套件会被环境依赖污染。

目前还能忍，是因为测试规模不大；继续堆下去会变成“大家都不想跑”的测试。

### 2.3 Fixture 和测试辅助代码重复明显

大量测试都重复创建：

- `UiRuntime m_runtime`
- `std::unique_ptr<UiRuntimeScope> m_scope`
- `SetUp()` / `TearDown()` 注册系统
- `Registry::Get/TryGet` 后做组件字段断言

这说明测试基础设施没有跟上。结果是：

- 新增测试门槛高。
- 用例正文被样板代码淹没。
- 运行时隔离、系统注册、主题重置等约束靠作者自觉，容易漏。

### 2.4 `test_ThemeSystem.cpp` 已经开始失控

`test_ThemeSystem.cpp` 单文件约 439 行，是当前最大测试文件。问题不是“长”，而是它混合了：

- Button 默认主题；
- 显式样式覆盖；
- hover/active/disabled/focus 状态；
- DropDown popup 子项；
- 控件圆角/边框几何；
- Window 几何主题。

这些不是一个测试文件该承受的职责。后续主题系统继续扩展时，这个文件会变成“谁都不敢改的泥球”。

### 2.5 公共 API 与内部实现边界测试不足

项目正在规划 `ui::entity` 与 `entt::entity` 隔离。当前测试大量直接包含内部头：

- `src/singleton/Registry.hpp`
- `src/singleton/Dispatcher.hpp`
- `src/systems/*`
- `common/components/*`

这对内部单测合理，但公开 API 层缺少独立验证：

- `include/ui.hpp` 只做了“能包含”的最低限度检查。
- 没有“外部用户视角”的编译测试集。
- 没有检查公开头是否泄漏 `entt`。
- 没有针对 DSL 作为公开契约的专门测试组。

如果后续要做 API 隔离改造，现在的测试会保护内部行为，但不足以保护“用户可见契约”。

### 2.6 渲染和布局基本缺席

项目核心卖点包括 SDL3 GPU、Yoga Flexbox、ECS UI，但现有测试更多停留在组件字段层。缺口明显：

- Yoga 布局计算结果缺少 golden/数值断言。
- DPI/缩放/白边优化相关没有自动回归测试。
- RenderSystem/Batch/clip/scissor 只覆盖到 `BatchManager` 的低层数据结构。
- 事件输入到视觉结果的链路没有端到端验证。

这意味着最容易被用户感知的 UI 问题，目前主要靠手动跑示例程序。

### 2.7 缺少负向、随机、属性型测试

目前多数用例是“构造一个正常对象，调用一次，检查字段”。缺少：

- 随机层级树重挂载后无环、不丢子、不重复子。
- 随机文本输入下 UTF-8 边界不越界。
- 随机尺寸/缩放下布局结果非 NaN、非负、边界稳定。
- 无效实体、销毁实体、重复注册/注销系统的组合压力。

ECS 框架最怕状态空间爆炸；当前测试还没有真正去打状态空间。

## 3. 改造目标

测试模块应从“覆盖率补丁集合”升级为四层测试体系：

```text
tests/
  unit/          纯函数、数据结构、无 SDL/无窗口/无 GPU
  ecs/           Registry/Dispatcher/System 行为与运行时隔离
  api/           公开 API、include/ui.hpp、DSL、实体句柄边界
  integration/   布局、输入、主题、渲染链路的 headless 或可控集成测试
```

核心目标：

1. 快速测试 5 秒内可跑完。
2. API 契约测试与内部实现测试分层。
3. 高风险模块都有回归用例，而不是只测 setter。
4. 测试 Fixture 标准化，避免每个文件手搓运行时。
5. 能支撑后续 `ui::entity` 公开句柄隔离改造。

## 4. 推荐目录规划

```text
tests/
  CMakeLists.txt
  support/
    UiTestRuntime.hpp
    EntityAssertions.hpp
    ComponentMatchers.hpp
    ThemeTestHelpers.hpp
    HierarchyGenerators.hpp
  unit/
    test_Result.cpp
    test_TextUtils.cpp
    test_BatchManager.cpp
    test_ResourceProvider.cpp
  ecs/
    test_UiRuntime.cpp
    test_TaskChain.cpp
    test_Hierarchy.cpp
    test_DragDrop.cpp
    test_TweenSystem.cpp
  api/
    test_UmbrellaHeader.cpp
    test_PublicHeadersNoEntt.cpp
    test_PublicEntityContract.cpp
    test_ChainsPublicApi.cpp
  integration/
    test_ThemeSystem_Button.cpp
    test_ThemeSystem_Input.cpp
    test_ThemeSystem_DropDown.cpp
    test_LayoutYogaBasics.cpp
    test_WindowScaling.cpp
```

## 5. CMake 目标规划

不要继续把全部测试塞进一个目标。建议拆成：

| Target | 内容 | 依赖 | 运行频率 |
|---|---|---|---|
| `ui_unit_tests` | 纯函数、低层数据结构 | `ui` + gtest，尽量不碰系统注册 | 每次提交必跑 |
| `ui_ecs_tests` | Registry/Dispatcher/System | `ui` + 内部头 | 每次提交必跑 |
| `ui_api_tests` | 公开 API 契约 | 只允许包含 `ui.hpp` 或公开头 | 每次提交必跑 |
| `ui_integration_tests` | 布局/主题/渲染链路 | 可依赖 SDL/headless 环境 | PR/夜间/手动 |

短期如果不想大改 CMake，也至少先加 label：

```text
unit
ecs
api
integration
```

并保证能通过 CTest 按 label 跑局部测试。

## 6. 测试基础设施规划

### 6.1 新增 `tests/support/UiTestRuntime.hpp`

统一运行时隔离：

- 自动创建 `UiRuntime`。
- 自动进入 `UiRuntimeScope`。
- 可选注册/注销指定系统。
- `TearDown()` 清理 Dispatcher 队列、主题状态、全局上下文。

建议提供：

```cpp
class UiRuntimeTest : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    UiRuntime& runtime();
};
```

### 6.2 新增组件断言工具

当前用例反复写：

```cpp
const auto* comp = Registry::TryGet<T>(entity);
ASSERT_NE(comp, nullptr);
EXPECT_EQ(...);
```

建议封装：

- `RequireComponent<T>(entity)`
- `ExpectHasTag<T>(entity)`
- `ExpectNotHasTag<T>(entity)`
- `ExpectColorNear(actual, expected)`
- `ExpectVec2Near/ExpectVec4Near`

收益：减少样板代码，提高断言信息质量。

### 6.3 新增层级测试生成器

针对 Hierarchy/DragDrop：

- 构造链式树；
- 构造多兄弟树；
- 随机重挂载；
- 校验无环、无重复 child、parent/children 双向一致。

这些比单个手写用例更能打穿 ECS 状态问题。

## 7. 分阶段执行计划

### 阶段 A：整理与止血（1～2 天）

目标：不改变生产代码，只降低测试维护成本。

- [ ] 新建 `tests/support/`。
- [ ] 抽出 `UiRuntimeTest` 基类。
- [ ] 抽出常用组件/Tag 断言。
- [ ] 将 `test_ThemeSystem.cpp` 拆成 Button/Input/DropDown/Window 几个文件。
- [ ] 将 `*Coverage.cpp` 文件改名为按模块命名：
  - `test_VisibilityCoverage.cpp` → `test_Visibility.cpp`
  - `test_UtilsCoverage.cpp` → `test_Utils.cpp`
  - `test_HierarchyCoverage.cpp` → `test_Hierarchy.cpp`
  - `test_ResourceProviderCoverage.cpp` → `test_ResourceProvider.cpp`

验收：测试行为不变，构建通过，CTest 发现测试数量不减少。

### 阶段 B：API 契约测试补齐（2～3 天）

目标：支撑公开 API 与内部 EnTT 隔离改造。

- [ ] 新增 `tests/api/test_PublicHeadersNoEntt.cpp`。
- [ ] 新增“外部用户视角”测试：只包含 `ui.hpp`，不包含 `src/*`。
- [ ] 新增 `ui::entity` / `ui::null_entity` 行为测试。
- [ ] 新增 Chains DSL 编译与行为测试。
- [ ] 增加脚本或 CMake 检查：公开头不包含 `entt/`，不出现 `entt::entity`。

验收：公开 API 测试不依赖 `Registry.hpp`、`Dispatcher.hpp`、组件头。

### 阶段 C：高风险 ECS 行为测试（3～5 天）

目标：从字段测试升级到状态一致性测试。

- [ ] Hierarchy 随机/参数化重挂载测试。
- [ ] DragDrop 多层树循环阻断压力测试。
- [ ] 系统重复 register/unregister 幂等性测试。
- [ ] Dispatcher queued/immediate 混合顺序测试。
- [ ] RuntimeScope 嵌套 + 系统连接生命周期测试。

验收：能发现 parent/children 不一致、事件串线、系统重复订阅等问题。

### 阶段 D：布局与渲染可验证化（5～8 天）

目标：覆盖用户最能感知的问题。

- [ ] Yoga 基础布局 golden 测试：横排、竖排、padding、spacing、fill/fixed。
- [ ] 缩放/DPI 数值测试：不同 scale 下 layout rect 与 clip rect 稳定。
- [ ] scissor/clip 嵌套测试。
- [ ] 文本布局 CJK/emoji/长单词回归集。
- [ ] Render path 先做数据层断言，避免一开始就做截图测试。

验收：布局或缩放算法变动时，测试能给出明确数值 diff。

### 阶段 E：测试运行体验与 CI 化（2～4 天）

目标：让测试真的被频繁运行。

- [ ] 拆分 `ui_unit_tests`、`ui_ecs_tests`、`ui_api_tests`、`ui_integration_tests`。
- [ ] 为 CTest 增加 label。
- [ ] 增加 `ENABLE_BUILD_TESTS=ON` 的开发预设。
- [ ] README 或 docs 增加测试运行说明。
- [ ] 如果接入 CI，至少跑 unit/ecs/api；integration 可夜间跑。

验收：开发者能快速跑小测试，CI 能稳定拦住核心回归。

## 8. 优先补的测试清单

### P0：马上补

- [ ] 公开头无 EnTT 泄漏检查。
- [ ] `ui::entity` 与内部实体转换边界测试。
- [ ] `UiRuntimeScope` 下系统连接不串线测试。
- [ ] ThemeSystem 拆文件，避免继续膨胀。
- [ ] Hierarchy parent/children 双向一致性统一断言。

### P1：近期补

- [ ] Yoga 布局基础 golden 测试。
- [ ] DragDrop 随机树循环阻断测试。
- [ ] TextUtils 增加 CJK、emoji、非法 UTF-8 或边界输入。
- [ ] Dispatcher immediate/queued 顺序组合测试。
- [ ] Theme 显式覆盖与主题拥有字段的参数化测试。

### P2：中期补

- [ ] Headless 渲染数据链路测试。
- [ ] 输入事件到组件状态变化的集成测试。
- [ ] DPI/缩放回归测试。
- [ ] 性能烟雾测试：大量实体布局/主题更新不退化到离谱。

## 9. 测试命名规范建议

当前命名混有 `Coverage`、`Integration`、模块名。建议统一：

```text
test_<Module>.cpp
test_<Module>_<Scenario>.cpp
```

用例命名遵循：

```text
Operation_WhenCondition_ExpectedResult
```

示例：

- `AddChild_WhenChildHasOldParent_ReparentsAndMarksBothParentsDirty`
- `DragDropped_WhenTargetIsDescendant_DoesNotCreateCycle`
- `SetTheme_WhenUserOverrodeBackground_DoesNotOverwriteExplicitValue`

## 10. 反模式清单

后续写测试应避免：

- 为了覆盖率写“一调用一字段”的无意义测试，却不覆盖行为风险。
- 在 API 测试里包含 `src/singleton/Registry.hpp`。
- 用 `EXPECT_NO_FATAL_FAILURE` 假装验证无效输入；无效输入应验证状态没有被污染。
- 单个测试文件继续无限膨胀。
- 测试依赖执行顺序或共享全局状态。
- 只测 happy path，不测重复调用、销毁实体、空实体、跨 runtime。

## 11. 最终验收标准

完成本规划后，测试模块应满足：

- `ui_unit_tests`、`ui_ecs_tests`、`ui_api_tests` 可独立运行。
- 公开 API 测试不包含内部头。
- `test_ThemeSystem.cpp` 不再是单文件巨石。
- 组件断言与运行时 Fixture 复用统一。
- Hierarchy/DragDrop/Runtime/Dispatcher 有状态一致性测试。
- 布局和缩放至少有一组数值 golden 回归。
- CTest label 可区分 `unit`、`ecs`、`api`、`integration`。

## 12. 一句话总结

当前测试模块已经能防一部分“手滑改坏字段”的回归，但还不能支撑 UI 框架级别的架构演进。下一步不要盲目堆用例，先分层、抽 Fixture、补 API 契约，再把布局、渲染、输入这些真正高风险路径纳入自动化。