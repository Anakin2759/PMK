# VMP-ui 架构锐评与优化建议（2026-06-19）

> 结论先行：VMP-ui 已经不是一个玩具 UI Demo，而是一个正在长成小型 UI 框架的 C++23 ECS 项目。问题也很典型：技术路线有野心，局部设计有亮点，但边界治理、运行时依赖、文本管线和工程组织正在进入“再不收敛就会反噬”的阶段。

## 1. 当前架构画像

### 1.1 2026-06-21 复审结论

这次复审的结论比 2026-06-19 更明确：项目不是缺“能力”，而是缺“边界硬度”。`api/detail` 拆分、`RuntimeFacade`、架构边界脚本、`<ui.hpp>` 伞头都已经出现，说明治理方向没错；但执行仍处于“看起来在分层，实际仍互相伸手”的中间态。

当前最危险的不是某个 bug，而是三类债务正在同时固化：

1. `src/api/` 仍然承担实现层职责，公开门面不够薄；
2. `RuntimeFacade::current()` 正在从过渡设施变成新服务定位器；
3. `src/detail/*.cpp` 与 `src/CMakeLists.txt` 的构建覆盖不一致，存在影子实现和迁移幻觉。

锐评：VMP-ui 已经有框架雏形，但还没有真正形成框架纪律。现在继续堆控件，会把 Chain DSL、Runtime、Renderer、Layout、Text Service 绑成一团；先收边界，再扩功能。

VMP-ui 当前采用：

- EnTT ECS 作为实体/组件基础；
- SDL3 GPU 作为渲染后端；
- Yoga 负责 Flexbox 布局；
- FreeType + HarfBuzz 负责字体和文本整形；
- ASIO `io_context` 负责事件循环；
- spdlog 负责日志；
- CMake 静态链接所有第三方依赖。

目录职责大体如下：

| 目录 | 当前定位 | 实际状态评价 |
| --- | --- | --- |
| `src/api/` | 对外 API 与 Chain DSL | 名义上是公开 API，实际仍掺了大量 ECS/Registry/Runtime 逻辑 |
| `src/detail/` | 内部实现层 | 已经建立，但迁移不完整，存在“影子实现”风险 |
| `src/common/` | 组件、事件、类型、错误码 | 组件纯数据方向正确，是架构相对干净的部分 |
| `src/core/` | Application、Runtime、事件循环、平台窗口 | Runtime 能力集中，但也开始成为全局服务定位器 |
| `src/systems/` | ECS 系统 | 部分已支持注入，部分仍依赖 `RuntimeFacade::current()` |
| `src/renderers/` | 控件渲染器 | 能工作，但与 ECS/Registry 耦合较重 |
| `src/managers/` | 字体、图标、图片、资源、文本缓存 | 功能集中，后续需要明确与 service/renderer 的边界 |
| `src/services/` | 跨系统业务服务 | 当前主要是文本编辑，边界还不够独立 |
| `src/singleton/` | Registry、Dispatcher、Logger 包装 | 包装有价值，但 `raw()`/静态入口应继续收窄 |

### 1.2 2026-06-21 当前债务基线

| 债务项 | 当前观察 | 架构评价 | 处理策略 |
| --- | --- | --- | --- |
| API cpp 仍访问 runtime | `src/api/Utils.cpp`、`Theme.cpp`、`Query.cpp`、`Image.cpp`、`Factory.cpp`、`Controls.cpp`、`Canvas.cpp` 仍出现 `RuntimeFacade::current()` | API 仍偏厚，门面没有完全闭合 | 新增 API 不得增加 runtime 访问；逐文件迁入 `detail` |
| detail 源文件未全部编译 | `src/detail/Factory.cpp`、`Controls.cpp`、`Canvas.cpp`、`Query.cpp`、`Theme.cpp`、`Timer.cpp`、`Shortcut.cpp`、`Utils.cpp`、`Log.cpp` 等存在，但未全部进入 `UI_SOURCES` | 迁移状态不可见，容易形成双实现 | 建立“已编译 / 未编译 / 废弃 / 待迁移”状态表 |
| RuntimeFacade 扩散 | `core/`、`systems/`、`services/`、`renderers/` 仍有多处 `RuntimeFacade::current()` | 新服务定位器风险已经成立 | 按目录 baseline，只减不增，系统优先改构造注入 |
| Renderer 读取 ECS/Runtime | renderer 层仍可通过 registry/raw/context 获取状态 | 渲染边界偏软，测试和替换后端困难 | 系统生成 render data，renderer 只消费绘制输入 |
| 文本管线多头决策 | 编辑、测量、缓存、绘制都可能各自推导行高、baseline、caret | 多行文本和光标错位会反复出现 | 引入 `TextLayoutResult` 作为唯一权威布局结果 |
| 文档权威源不足 | README、计划文档、架构评审、脚本规则存在轻微漂移 | 文档多但闭环弱 | 本文作为总评，专题文档承接执行计划 |

## 2. 值得保留的亮点

### 2.1 技术路线有辨识度

EnTT + SDL3 GPU + Yoga + HarfBuzz 这套组合方向明确：保留 ECS 的数据驱动能力，又避免直接写一个传统 OOP 控件树。对于自研 UI 框架，这是有长期潜力的路线。

### 2.2 Chain DSL 是核心 API 资产

`entity | Size(...) | BackgroundColor(...) | Show()` 这种声明式管道写法，是项目最有辨识度的用户侧 API。它比裸 ECS 操作更接近 UI 使用者心智，也比大而全的 Builder 类更轻。

这部分应被当作“公开 API 门面”保护，而不是让它反向暴露 EnTT、Registry 或内部组件。

### 2.3 已经有边界意识

`ui::entity`、`src/detail/EntityCast.hpp`、`修改规划.md`、`tools/check_architecture_boundaries.py` 都说明项目已经意识到：

- 公开 API 不能暴露 EnTT；
- `api` / `detail` 应切开；
- 系统不应继续依赖旧的静态 `Registry::Xxx` / `Dispatcher::Xxx`；
- 架构债需要可执行门禁。

方向是对的，问题是执行还没到底。

### 2.4 错误处理和测试方向现代

`Result<T>` / `std::expected`、`ui_errc`、Google Test、架构边界脚本，说明项目并非只堆功能。后续应把这些机制继续用于“防腐”，而不是只当局部工具。

## 3. 尖锐问题清单

### P0：必须优先处理

#### P0-0：最刺耳但最真实的问题——架构进入“半治理”状态

当前项目已经不再是完全混乱的原型，也还不是边界清晰的框架。它卡在最危险的中间层：

- 有 `detail/`，但并非所有内部实现都迁入并纳入构建；
- 有 `RuntimeFacade`，但还没有把它限制在生命周期边界；
- 有公开 API 伞头，但 API cpp 仍在直接找 runtime 和 registry；
- 有架构脚本，但规则还没覆盖“服务定位器扩散”和“影子实现”。

锐评：这类半治理架构比纯乱代码更容易误导维护者，因为目录结构看起来已经对了，实际依赖方向还没对。

**立即规则：**

1. 新增 `src/api/*.cpp` 代码不得新增 `RuntimeFacade::current()`；
2. 新增 `src/detail/*.cpp` 必须同步进入 `src/CMakeLists.txt` 或明确标注为未启用草案；
3. 新增 System 依赖必须优先走构造注入，不允许业务路径中随手找全局 runtime；
4. 新增 Renderer 不得主动承担业务状态推导，只能消费系统整理后的渲染输入。

#### P0-1：`src/api/` 仍然不是纯 API 层

当前 `src/api/` 的问题很直接：它表面上是公开 API，实际上仍在直接干实现层的活。

已观察到的典型现象：

- `src/api/Factory.cpp` 直接包含 EnTT，并承担大量实体创建、控件组装、弹层处理逻辑；
- `src/api/Hierarchy.cpp` 直接操作 `entt::entity`、Hierarchy 组件和父子关系；
- `src/api/Table.cpp` 直接处理 table cell 关系和内部实体；
- `src/api/Utils.cpp` 同时承担层级查询、坐标计算、脏标记、滚动条几何等多类职责；
- `src/api/Image.hpp` 这类公开头中存在访问 runtime registry 的 inline 逻辑风险；
- `src/api/*.cpp` 中仍有多处 `RuntimeFacade::current()` 和 `entt::entity`。

锐评：`api/` 现在不是“门面层”，而是“半个实现层 + 半个公开层”。这会导致公开 API 边界永远收不干净。

**目标状态：**

- `src/api/*.hpp`：只暴露 `ui::entity`、基础类型、Result、公开 enum/struct；
- `src/api/*.cpp`：只做参数转换、返回值转换、转发；
- `src/detail/*`：负责实际 Registry/组件/EnTT 访问；
- Chain DSL 只能绑定公开 API 函数，不能绑定 `detail` 函数。

#### P0-2：`RuntimeFacade` 正在成为“新单例”

`RuntimeFacade` 的出现比旧式散落的 `Registry::current()` 更好，但当前使用方式有明显服务定位器倾向。

已观察到的扩散位置包括：

- `src/api/*`：大量通过 `RuntimeFacade::current().registry()` 访问 Registry；
- `src/detail/*`：内部实现层也频繁直接取当前 runtime；
- `src/systems/*`：`ActionSystem`、`StateSystem`、`TimerSystem`、`ThemeSystem` 等仍直接取 frame/state/context/windowLookup；
- `src/renderers/*`：渲染器存在读取 theme context 或 registry raw 能力的情况；
- `src/services/TextEditingService.cpp`：文本编辑服务直接取全局 runtime registry。

锐评：这不是彻底的依赖注入，而是“DI 和全局服务定位器混用”。短期方便，长期会卡住多实例、多窗口、测试隔离和后台任务。

**目标状态：**

- `RuntimeFacade` 保留为应用生命周期和 runtime 聚合边界；
- 系统构造时显式注入 `Registry&`、`Dispatcher&`、必要 manager/service/context；
- renderer 尽量消费 `RenderContext` / `RenderItem` / `WidgetRenderData`，不要主动扫 Registry；
- service 由 runtime 创建和注入，不应自己寻找全局 runtime。

#### P0-3：架构边界门禁太窄

`tools/check_architecture_boundaries.py` 已经挂入构建，这是优点。但规则目前偏窄，能挡“明显坏味道”，挡不住更关键的泄漏。

当前主要检查：

- 内部目录反向 include `api/*`；
- `systems/` 使用旧静态 `Registry::Xxx` / `Dispatcher::Xxx`；
- `common/` include runtime/registry。

但缺少：

- 禁止 `src/api/**/*.hpp` include `entt/`；
- 禁止 `src/api/**/*.hpp` include `core/RuntimeFacade.hpp`、`singleton/Registry.hpp`、`singleton/Dispatcher.hpp`；
- 禁止 `src/api/**/*.hpp` 出现 `entt::`；
- 统计 `src/api/**/*.cpp` 的 `entt::entity` 与 `RuntimeFacade::current()` 债务；
- 统计 `systems/`、`renderers/`、`services/` 的 `RuntimeFacade::current()` 债务；
- 统计 `Registry::raw()` / `Dispatcher::raw()` 逃逸点。

锐评：现在有门禁，但不够像“架构宪法”，更像“新增明显违规提示器”。

#### P0-4：`src/CMakeLists.txt` 暴露迁移半成品风险

`src/detail/` 下存在 `Factory.cpp`、`Table.cpp`、`Text.cpp`、`Hierarchy.cpp`、`Controls.cpp`、`Canvas.cpp`、`Icon.cpp`、`Query.cpp`、`Theme.cpp`、`Timer.cpp`、`Shortcut.cpp`、`Utils.cpp` 等文件，但 `src/CMakeLists.txt` 的 `UI_SOURCES` 当前只显式纳入部分 `detail/*`。

这有两个风险：

1. 文件存在但未编译，实际迁移进度可能被高估；
2. `api` 和 `detail` 同名实现长期并存，后续修改者不知道该改哪边。

锐评：这是典型“目录看起来完成，构建实际没覆盖”的迁移陷阱。

### P1：短期能跑，长期拖慢演进

#### P1-1：`detail` 层职责还没有真正成为唯一实现归宿

`detail/EntityCast.hpp` 已经是好开端，但迁移还不一致。简单 setter 如 `Size`、`Visibility`、`Layout` 已适合作为模板；复杂模块如 `Factory`、`Hierarchy`、`Table`、`Utils` 仍是重灾区。

建议不要继续“边写新功能边顺手迁一点”。应按模块建立迁移状态表，逐个完成闭环。

#### P1-2：渲染器层与 ECS 耦合过重

`src/renderers/RendererRegistry.hpp` 中存在 `reg.raw().all_of` / `reg.raw().any_of` 这种逃逸。渲染器读取 ECS 状态并不一定错误，但如果 renderer 同时负责：

- 判断组件组合；
- 读取业务状态；
- 计算布局或派生几何；
- 执行具体绘制；

它会逐渐变成不可测试的大函数集合。

建议引入更明确的渲染数据边界：系统负责收集 ECS 状态并生成 render data，renderer 只消费 render data。

#### P1-3：文本管线没有统一权威布局结果

当前有效需求中提到：“多行文本框换行后，光标显示高度与第二行实际输入字符高度不一致”。这不是单纯的 caret 绘制 bug，而是文本管线边界问题。

文本相关逻辑分布在：

- `src/services/TextEditingService.cpp`；
- `src/renderers/TextRenderer.hpp`；
- `src/managers/TextTextureCache.*`；
- `src/managers/TextRenderHelper.hpp`；
- `src/managers/FontManager.*`；
- `src/core/TextUtils.hpp`；
- `src/common/components/Data.hpp` 中的 `TextEdit` / `Caret` 数据。

风险是：输入服务、布局测量、纹理缓存、renderer 分别“猜”行高、baseline、caret rect。只要有一个地方使用不同指标，多行文本就会继续错位。

#### P1-4：树结构表达还没有独立模型

当前 Chain DSL 适合 C++ 手写 UI，但不天然适合：

- 流式构建；
- 序列化；
- diff / patch；
- 远端或配置驱动 UI。

如果强行把 Chain DSL 扩成结构化描述格式，会污染它原本简洁的 API 定位。

建议新增独立声明模型，而不是替换或扭曲现有 DSL。

### P2：可以排后，但要防止继续膨胀

#### P2-1：CMake 主文件复杂度上升

根 `CMakeLists.txt` 与 `src/CMakeLists.txt` 已包含 LTO、clang-tidy、shader 编译、资源后端、CPU fallback、平台缩放、多线程、ASAN、测试等大量逻辑。继续扩张后，会影响新平台、新后端和 CI 配置维护。

#### P2-2：文档多，但权威架构文档少

`docs/` 下已有大量计划文档，但“当前真实状态 + 禁止规则 + 迁移进度 + 验收标准”的单一事实源不足。结果就是计划越来越多，关闭越来越少。

#### P2-3：测试偏功能，缺少架构规则测试

已有单元测试基础，但架构边界本身也应被测试。`check_architecture_boundaries.py` 应升级为主要架构规则测试，并在 CI / 本地构建中持续运行。

## 4. 优化路线图

### 当前治理进展（2026-06-19）

本轮已先执行“止血 + 公开 API 变薄”的低风险闭环，当前状态如下：

| 工作项 | 状态 | 说明 |
| --- | --- | --- |
| 扩展架构边界脚本 | 已完成 | `tools/check_architecture_boundaries.py` 已新增公开 API 头硬门禁，并对 API cpp / runtime / raw 逃逸建立软基线。 |
| 恢复公共伞头 | 已完成 | 新增 `include/ui.hpp`，恢复 README、示例和测试使用的 `<ui.hpp>` 入口。 |
| 清理 `Image` 公开头 | 已完成 | `src/api/Image.hpp` 已从 inline runtime 实现改为纯声明，实现迁入 `src/api/Image.cpp`。 |
| 迁移 `Hierarchy` 到 detail | 已完成 | `src/api/Hierarchy.cpp` 已变为转换/转发层；实际层级操作位于 `ui::detail::hierarchy`。 |
| 迁移 `Table` 到 detail | 已完成 | `src/api/Table.cpp` 已变为转换/转发层；实际表格行列、cell widget 归属逻辑位于 `ui::detail::table`。 |
| 迁移 `Text` 到 detail | 已完成 | `src/api/Text.cpp` 已变为转换/转发层；文本组件、TextEdit 回调和文字样式操作位于 `ui::detail::text`。 |
| 迁移 `Icon` 到 detail | 已完成 | `src/api/Icon.cpp` 已变为转换/转发层；纹理图标、字体图标和移除逻辑位于 `ui::detail::icon`。 |
| 迁移 `Canvas` 到 detail | 已完成 | `src/api/Canvas.cpp` 已变为转换/转发层；绘图命令写入逻辑位于 `ui::detail::canvas`。 |
| 收缩 API cpp 债务基线 | 已完成 | 已从架构脚本基线移除 `Hierarchy` / `Table` 的 `RuntimeFacade::current()` 与 `entt::entity` 债务。 |

当前 `src/api/Hierarchy.cpp`、`src/api/Table.cpp`、`src/api/Text.cpp`、`src/api/Icon.cpp` 与 `src/api/Canvas.cpp` 已不再直接出现 `RuntimeFacade::current()`、`entt::entity` 或 EnTT include。下一步建议优先治理 `Controls`，随后再集中处理高风险的 `Factory` 与工具桶 `Utils`。

### 2026-06-21 复审更新

本次复审确认：前一轮“止血 + 公开 API 变薄”是有效方向，但债务剩余面仍然很清楚。

#### 2026-06-21 首批落地结果

本轮已先落实一批低风险、直接对应文档建议的改动：

1. `tools/check_architecture_boundaries.py` 新增了 `src/detail/*.cpp` 未纳入 `UI_SOURCES` 的检查与基线；
2. `src/api/Controls.cpp` 已收敛为公开层转发，不再直接访问 `RuntimeFacade::current()`；
3. `src/api/Query.cpp` 与 `src/api/Theme.cpp` 暂未迁入 `detail`，但保持现状可编译，说明后续需要先解决 `detail` 命名空间与头文件组织，再继续推进；
4. `Controls` 相关实现已完成一轮 API/内部桥接拆分验证，证明“先转发、后彻底下沉”是可行路线。

这也暴露了一个现实问题：当前部分 `detail/*.hpp` 仍复用了 `api` 侧 DSL/Action 头，导致直接 include 时容易触发重定义。也就是说，`detail` 目录虽然存在，但还没有彻底摆脱“和 api 共享声明面”的过渡态。

#### 2026-06-22 继续落地结果

本轮继续沿“先让 API 变薄，再逐步收缩基线”的路线推进：

1. `Controls` 完成闭环：`src/api/Controls.cpp` 保持公开类型转换与转发，内部实现下沉到 `src/detail/Controls.cpp`，并通过 `src/detail/ControlsBridge.hpp` 暴露内部桥接入口；
2. `Query` 完成闭环：`src/api/Query.cpp` 不再直接访问 `RuntimeFacade::current()`，内部查询实现进入 `src/detail/Query.cpp` / `QueryBridge.hpp`；
3. `Theme` 完成闭环：`src/api/Theme.cpp` 不再直接访问 runtime context，主题上下文操作进入 `src/detail/Theme.cpp` / `ThemeBridge.hpp`；
4. `src/CMakeLists.txt` 已纳入 `detail/Controls.cpp`、`detail/Query.cpp`、`detail/Theme.cpp`；
5. `tools/check_architecture_boundaries.py` 已同步收缩 `detail-cpp-not-in-ui-sources` 基线；
6. 架构边界检查通过，CMake 构建通过。

本轮经验：对已经有完整 public DSL 的模块，`detail/*.hpp` 不应复用 `api` 侧 Action/Chain 声明。更稳妥的做法是新增 `*Bridge.hpp`，只暴露内部 `entt::entity` 入口，避免 public API 与 detail 实现因同名命名空间或 DSL 声明发生重定义。

| 模块 | 当前状态 | 评价 | 下一步 |
| --- | --- | --- | --- |
| `Hierarchy` / `Table` / `Text` / `Icon` | API 已基本转发到 `detail` | 可作为迁移模板 | 保持不回退，脚本防止重新引入 runtime 访问 |
| `Utils` | 仍在 API 层聚合层级、几何、窗口关闭、坐标转换等杂项 | 工具桶，最容易继续变胖 | 拆到 `detail::hierarchy`、`detail::layout`、`detail::window`、`detail::query` |
| `Factory` | 仍是创建、组装、默认样式、弹层等能力集中点 | 最大 API 债务源，不能零散改 | 先抽 `detail::factory` 内部 primitive，再替换 API 转发 |
| `Controls` / `Query` / `Theme` | API 已通过 Bridge 转发到 `detail` | 本轮已完成闭环，可作为带 DSL 模块迁移模板 | 保持不回退，后续减少同类 bridge 样板 |
| `Image` | API cpp 仍访问 runtime registry | 中等复杂度，适合继续推进 | 迁入 `detail::image` 或 `ImageBridge` |
| `detail/*.cpp` 构建覆盖 | 多个 detail 源文件存在但未纳入 `UI_SOURCES` | 迁移可见性不足 | 先盘点再决定纳入、删除或标注草案 |

#### api/detail 迁移状态表（复审版）

| 模块 | API 是否薄封装 | detail 是否存在 | detail cpp 是否纳入构建 | 优先级 |
| --- | --- | --- | --- | --- |
| `Size` | 是 | 是 | 是 | 已完成 |
| `Visibility` | 是 | 是 | 是 | 已完成 |
| `Layout` | 是 | 是 | 是 | 已完成 |
| `Animation` | 是 | 是 | 是 | 已完成 |
| `Hierarchy` | 是 | 是 | 是 | 已完成 |
| `Table` | 是 | 是 | 是 | 已完成 |
| `Text` | 是 | 是 | 是 | 已完成 |
| `Icon` | 是 | 是 | 是 | 已完成 |
| `Canvas` | 是 | 是 | 是 | 已完成 |
| `Controls` | 是 | 是 | 是 | 已完成 |
| `Image` | 部分 | 是（仅头） | 不适用/待确认 | P0 |
| `Query` | 是 | 是 | 是 | 已完成 |
| `Theme` | 是 | 是 | 是 | 已完成 |
| `Timer` | 部分 | 是 | 否 | P1 |
| `Shortcut` | 部分 | 是 | 否 | P1 |
| `Factory` | 否 | 是 | 否 | P0，高风险 |
| `Utils` | 否 | 是 | 否 | P0，高风险 |
| `Log` | 部分 | 是 | 否 | P2 |

说明：表中“否”不代表代码不可用，而是表示它还没有达到“API 只做公开类型转换与转发”的目标状态。

### 阶段 1：止血，先防止债务继续扩大

优先级最高。不要先大规模重构，否则会一边修一边新增泄漏。

建议动作：

1. 扩展 `tools/check_architecture_boundaries.py`：
   - 检查 `src/api/**/*.hpp` 是否 include `entt/`；
   - 检查 `src/api/**/*.hpp` 是否 include `core/RuntimeFacade.hpp`、`singleton/Registry.hpp`、`singleton/Dispatcher.hpp`；
   - 检查 `src/api/**/*.hpp` 是否出现 `entt::`；
   - baseline `src/api/**/*.cpp` 中的 `entt::entity`、`RuntimeFacade::current()`；
   - baseline `systems/`、`renderers/`、`services/` 中的 `RuntimeFacade::current()`；
   - baseline `raw()` 逃逸点。
2. 明确新增代码规则：
   - 新增公开头不得包含 EnTT；
   - 新增 API 实现必须先落 `detail`；
   - `api/*.cpp` 只做转换/转发；
   - 系统新增依赖必须通过构造或注册阶段注入。
3. 将根目录 `修改规划.md` 收敛进 `docs/architecture/`，并增加迁移状态表。

### 阶段 2：完成 API / detail 边界切割

推荐顺序：

1. `Visibility`、`Size`、`Layout`：作为薄封装模板；
2. `Hierarchy`：已完成，层级关系已有唯一 detail 实现；
3. `Table`：已完成，cell ownership 和实体归属已下沉到 detail；
4. `Text`：已完成 API/detail 边界切割；后续进入 `TextLayoutResult` 专项设计；
5. `Icon`：已完成 API/detail 边界切割；
6. `Controls`、`Image`；
7. `Factory`：依赖最广，最后集中治理；
8. `Utils`：拆掉工具桶，迁入 `detail::hierarchy`、`detail::layout`、`detail::query` 等明确归属。

验收标准：

- `src/api/**/*.hpp` 不出现 `entt::`；
- `src/api/**/*.hpp` 不 include `entt/`、`RuntimeFacade`、`Registry`、`Dispatcher`；
- `src/api/*.cpp` 中业务逻辑显著减少；
- `src/detail/*` 是 ECS/Registry 访问的主要归宿；
- Chain DSL 继续保持用户侧语义不变。

### 阶段 3：Runtime 访问边界治理

建议把 runtime 访问分成四类：

| 类别 | 允许访问方式 | 说明 |
| --- | --- | --- |
| 应用生命周期 | `RuntimeFacade` | Application、UiRuntime、启动/关闭流程 |
| 系统 | 构造注入 `Registry&` / `Dispatcher&` / context | 禁止随手 `RuntimeFacade::current()` |
| detail 实现 | 短期可 baseline，长期通过 RuntimeContext 参数传入 | 迁移期间允许逐步收敛 |
| renderer | 消费 render data / context | 不应主动查全局 registry |

短期可以不一次清零 `RuntimeFacade::current()`，但必须 baseline，并规定“只减不增”。

### 阶段 4：文本布局与光标同步专项

建议建立统一的文本布局结果，例如 `TextLayoutResult`：

- `glyphRuns`：整形后的 glyph runs；
- `lineBoxes`：每行位置、宽高、baseline；
- `ascent` / `descent` / `lineGap`；
- `caretRects`：每个合法光标位置对应矩形；
- `selectionRects`：选区矩形；
- `contentSize`：整体测量结果。

职责边界：

- `TextEditingService`：只维护编辑状态、光标索引、选择范围；
- 文本布局模块：负责换行、测量、baseline、caret rect；
- `TextRenderer`：只消费布局结果绘制文字、选区、光标；
- `TextTextureCache`：只缓存纹理/字形，不决定文本语义。

原则：光标高度不能由 renderer 自己猜，必须来自布局结果。

### 阶段 5：新增结构化 UI 树模型

不要替换 Chain DSL。建议并行新增数据化模型：

- `UiNodeSpec`：描述节点类型、属性、children；
- `UiTreeBuilder`：支持流式构建；
- `UiTreePatch`：支持 diff/patch；
- `UiTreeSerializer`：负责序列化/反序列化；
- `BuildEntityTree(UiNodeSpec)`：转换为现有 ECS 实体树。

定位：

- Chain DSL：手写 C++ UI；
- UI Tree Spec：配置化、远程化、可序列化 UI；
- 二者都落到同一套 `detail` 实现，避免双实现。

### 阶段 6：工程组织收敛

建议拆分 CMake：

- `src/cmake/UiOptions.cmake`：选项；
- `src/cmake/UiSources.cmake`：源文件；
- `src/cmake/UiShaders.cmake`：shader 编译；
- `src/cmake/UiResources.cmake`：资源嵌入；
- `src/cmake/UiPlatform.cmake`：平台链接。

同时检查 `src/detail/*.cpp` 是否应纳入构建，避免迁移半成品长期存在。

## 5. 建议落地文档拆分

当前文档可先以本文件为总评，再拆成以下专题文档：

1. `docs/architecture/change-api-detail-boundary-20260619.md`
   - `api` / `detail` 边界迁移规则、状态表、验收标准。
2. `docs/architecture/change-runtime-access-boundary-20260619.md`
   - `RuntimeFacade`、`Registry`、`Dispatcher` 访问策略和 DI 路线。
3. `docs/architecture/change-text-layout-caret-sync-20260619.md`
   - 多行文本布局、caret、selection、缓存一致性方案。
4. `docs/architecture/overview-ui-tree-spec-20260619.md`
   - 可流式、可序列化 UI 树模型设计。
5. `docs/architecture/architecture-boundary-rules-20260619.md`
   - 架构边界规则与脚本检查项。

## 6. 建议优先核验的文件

### API/detail 边界

- `src/api/Factory.cpp`
- `src/api/Hierarchy.cpp`
- `src/api/Table.cpp`
- `src/api/Utils.cpp`
- `src/api/Image.hpp`
- `src/detail/Factory.cpp`
- `src/detail/Hierarchy.cpp`
- `src/detail/Table.cpp`
- `src/detail/Utils.cpp`
- `src/detail/EntityCast.hpp`

### Runtime / DI

- `src/core/RuntimeFacade.hpp`
- `src/core/UiRuntime.hpp`
- `src/core/SystemManager.cpp`
- `src/singleton/Registry.hpp`
- `src/singleton/Dispatcher.hpp`
- `src/systems/ActionSystem.hpp`
- `src/systems/StateSystem.cpp`
- `src/systems/TimerSystem.cpp`
- `src/systems/ThemeSystem.cpp`

### Renderer / ECS 解耦

- `src/renderers/RendererRegistry.hpp`
- `src/renderers/TextRenderer.hpp`
- `src/renderers/TableRenderer.cpp`
- `src/renderers/ShapeRenderer.hpp`

### 文本布局与光标

- `src/services/TextEditingService.cpp`
- `src/managers/TextTextureCache.*`
- `src/managers/TextRenderHelper.hpp`
- `src/managers/FontManager.*`
- `src/core/TextUtils.hpp`
- `src/common/components/Data.hpp`

### 架构门禁

- `tools/check_architecture_boundaries.py`
- `src/CMakeLists.txt`

## 7. 推荐执行顺序

| 顺序 | 工作项 | 价值 | 风险 |
| --- | --- | --- | --- |
| 1 | 扩展架构边界脚本并 baseline | 防止继续腐蚀 | 已完成 |
| 2 | 建立 API/detail 迁移状态表 | 让迁移可见 | 已在本文更新 |
| 3 | 清理公开头泄漏 | 立刻改善 API 边界 | 已处理 `Image.hpp` / `<ui.hpp>` |
| 4 | 迁移 `Hierarchy` 到 detail | 统一实体树语义 | 已完成 |
| 5 | 迁移 `Table` 到 detail | 统一 cell ownership | 已完成 |
| 6 | 迁移 `Text` 到 detail | 为文本布局专项铺路 | 已完成 |
| 7 | 设计 `TextLayoutResult` | 解决光标/换行根因 | 中高 |
| 8 | 迁移 `Icon` 到 detail | 清理中等复杂度 API 债务 | 已完成 |
| 9 | 迁移 `Canvas` 到 detail | 清理中等复杂度 API 债务 | 已完成 |
| 10 | 迁移 `Controls` | 清理中等复杂度 API 债务 | 中 |
| 11 | 迁移 `Factory` | 清理最大债务源 | 高 |
| 12 | RuntimeFacade 访问 baseline 后逐步清零 | 提升可测性和多实例能力 | 中高 |
| 13 | UI Tree Spec 专项设计 | 支撑序列化/流式 UI | 中 |

## 8. 最终建议

不要继续把主要精力放在新增控件上。当前最该做的是“边界治理”：

1. 先让脚本把坏味道挡住；
2. 再让 `api` 真正变薄；
3. 然后让 runtime 依赖从服务定位器回到注入；
4. 最后处理文本管线和 UI 树模型。

一句话：VMP-ui 的方向是对的，但现在已经到了必须从“能跑”升级到“可演进”的节点。否则越往后，Chain DSL、ECS、Runtime、Renderer 会互相缠死，任何一个小需求都会变成跨层补丁。

## 9. 2026-06-21 优化建议总表

| 优先级 | 建议 | 具体动作 | 验收标准 |
| --- | --- | --- | --- |
| P0 | 冻结 API 层新增债务 | `src/api/*.cpp` 新增代码不得直接调用 `RuntimeFacade::current()`；确需访问 ECS 时先建 `detail` 函数 | 架构脚本能统计并阻止 API runtime 债务增加 |
| P0 | 清点 detail 影子实现 | 对所有 `src/detail/*.cpp` 标记“已编译 / 待纳入 / 草案 / 删除” | `src/detail` 不再有无人负责的同名实现 |
| P0 | 迁移 `Controls` / `Query` / `Theme` | 先做中等复杂度模块，形成 Factory 前置经验 | 对应 `api/*.cpp` 不再直接取 runtime registry/context |
| P0 | 拆 `Utils` 工具桶 | 按层级、布局、窗口、查询拆成明确 detail 命名空间 | `api/Utils.cpp` 只剩公开函数转发 |
| P0 | 规划 `Factory` 专项 | 不直接大改，先抽创建 primitive、默认样式、弹层创建三类内部函数 | `Factory.cpp` 行为不变，内部实现落到 `detail::factory` |
| P1 | RuntimeFacade 去服务定位器化 | System 构造注入 `Registry&`、`Dispatcher&`、frame/context/service | `systems/` 中 `RuntimeFacade::current()` 数量只减不增 |
| P1 | Renderer 输入数据化 | RenderSystem 收集状态，Renderer 消费 `RenderItem` / `WidgetRenderData` | renderer 不主动扫 registry，不做业务状态判断 |
| P1 | 统一文本布局结果 | 设计 `TextLayoutResult`，承载 line box、baseline、caret rect、selection rect | 光标、选区、绘制使用同一份 layout metrics |
| P1 | Manager / Service 边界重命名 | Manager 管生命周期和缓存；Service 管跨组件业务流程 | 新增能力能明确落到 manager 或 service，不再随意放置 |
| P2 | README 降级为用户入口 | README 只讲使用和模块概览，治理细节链接到 `docs/architecture/` | README 不再承载迁移计划细节 |
| P2 | CMake 模块化 | 拆 `UiOptions.cmake`、`UiSources.cmake`、`UiShaders.cmake`、`UiResources.cmake` | `src/CMakeLists.txt` 只保留装配逻辑 |

### 推荐的下一轮执行顺序

1. 更新 `tools/check_architecture_boundaries.py`：把 API runtime 债务、detail 构建覆盖、`raw()` 逃逸做成可见 baseline。
2. 盘点并修正 `src/detail/*.cpp` 与 `src/CMakeLists.txt` 的状态不一致。
3. 迁移 `Controls`、`Canvas`、`Query`、`Theme`，降低中等复杂度债务。
4. 拆 `Utils`，防止它继续吸收无归属逻辑。
5. 最后集中处理 `Factory`，不要把它当普通文件顺手改。

### 架构纪律一句话版

- `api` 只说人话，不碰 ECS 内脏；
- `detail` 负责脏活，但必须被构建和测试覆盖；
- `RuntimeFacade` 管生命周期，不当全局取物柜；
- `System` 通过注入拿依赖，不在业务路径找单例；
- `Renderer` 只画，不猜业务状态；
- 文本布局只能有一个权威结果。
