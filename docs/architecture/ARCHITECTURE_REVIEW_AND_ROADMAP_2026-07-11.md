

# VMP-ui 架构锐评与演进路线（2026-07-11）

> 状态：active（Phase 0 已完成；Phase 1 分项推进；WP3 最终 SDK 边界收紧仍受公共头依赖阻塞）
> 范围：公开 API、Runtime、事件与帧循环、System、布局、渲染、资源生命周期、CMake、测试与文档治理
> 原则：先消除确定性风险，再收敛执行模型，最后建设扩展能力。

## 0. 进度总览（代码核验于 2026-07-16）

### 0.1 状态口径

- `completed`：实现、测试和验收条件均已闭环。
- `active`：已有实质实现，但仍存在明确未完成项，不能按完成计。
- `blocked`：已有部分工作，但后续被明确前置决策或依赖阻塞。
- `not-started`：目标能力尚未实现；已有邻近基础设施不计为该工作包完成。
- “完成度”仅用于路线图管理，是按本工作包交付项估算的实现进度，不代表测试覆盖率，也不替代验收条件。

### 0.2 一眼可见的结论

- **已完成：2 项**——Phase 0 架构基线、WP2 System 连接生命周期。
- **正在推进：3 项**——WP1、WP4、WP7。
- **部分完成但受阻：1 项**——WP3，公共头迁移已有成果，但最终 SDK/CMake 边界仍未闭环。
- **尚未形成目标能力：3 项**——WP5、WP6、WP8；它们只有局部基础或前置铺垫。
- **尚未开始且受前置阻塞：1 项**——WP9。

### 0.3 工作包进度看板

| 阶段 / 工作包 | 状态 | 估算完成度 | 已经做了 | 明确没有做 / 未验收 |
| --- | --- | ---: | --- | --- |
| Phase 0：架构基线 | **completed** | **100%** | 静态门禁已统计 `UiRuntime::current()`、PUBLIC include/link 和 queued-event 派发点；`FrameContext` 已记录每调度帧 Layout/Render update 次数；`test_TaskChain.cpp` 覆盖常规帧、即时追加和下一帧复位 | 无剩余交付项；指标继续作为后续工作基线保留 |
| WP1 Runtime 与失败路径 | **active** | **65%** | 已有 `UiRuntime::tryCurrent()`；Runtime 构造不再切换 current；`UiRuntimeScope` 支持嵌套恢复；活动 Runtime 异常提前销毁会清除 stale current；`ApplicationImpl` 持有长期 scope；`CreateApplication()` catch 使用 stderr 后备路径；已有 5 个 Runtime 定向测试 | **没有** SDL 初始化失败注入测试；**没有**统一定义无 active Runtime 时所有 legacy API 的失败行为；非 Application 代码仍大量调用 `UiRuntime::current()` |
| WP2 System 连接生命周期 | **completed** | **100%** | 已建立 `ASSEMBLING / REGISTERED / STOPPED` 状态机；register/unregister 在 manager 层幂等；`removeSystem()` 在 REGISTERED 状态下先 unregister 再 erase；注册后追加明确返回 `false`；manager 级测试覆盖 phase 排序、重复注册/注销、移除后事件不再触发、追加拒绝及全部 12 个内建 System 的 phase 契约 | 无剩余交付项；后续新增内建 System 必须同步更新 phase 契约测试 |
| WP3 构建与 SDK 边界 | **blocked / active** | **68%** | `src/detail/` 已清零；多批 API 头及 Error/Result、Policies 已迁入 `include/`；Icon/Layout/Query/Size 已完成叶子头迁移；新增无内部依赖的 `ui::Callback`，Controls/Text 公开签名不再暴露 component callback；纵向滚动条几何已改为无 EnTT/Eigen 依赖的公共标量 DTO；架构门禁和公开头编译测试已覆盖 | **没有**收紧 PUBLIC include：`${CMAKE_SOURCE_DIR}` 和 `src/` 仍公开；**没有**收紧 PUBLIC link：EnTT/Eigen/spdlog 仍传播；**没有**独立 install/export consumer；Utils 其余 Rect/Vec2 以及 Controls/Text/Animation 等仍受 Eigen 公共类型决策阻塞 |
| WP4 GPU shutdown | **active** | **65%** | 渲染实现已物理拆成 Backend、Resources、Frame；Application 已保证 RenderSystem 先于 `SDL_Quit()` 销毁；首窗 claim 后锁定 device/backend；`ImageManager` 已使用创建 device 绑定的 RAII owner；固定三节点 GPU 初始化事务已让 shader 失败可观察，并统一中途失败逆序回滚与重复 cleanup；cleanup 保证资源先于 device 并复位 claim 状态 | **没有**完整的 `RenderResourceContext` 所有权边界；**没有**真实 GPU 初始化失败注入、资源代际 token 和 100 次窗口创建/销毁生命周期验收；外部原因导致 device 先于 owner 失效时仍缺少类型级保护 |
| WP5 单一帧管线 | **not-started / partial** | **15%** | `TaskChain`、System phase 和帧次数埋点提供了局部顺序及观测基础 | **没有**唯一 `FrameTick`；Task 内节流尚未统一为 scheduler policy；生产帧路径中 **没有**调用公开 `event::DispatchQueued()`，当前 `Enqueue()` 仍需手动派发；固定阶段契约未建立 |
| WP6 Intrinsic Measurement | **not-started** | **0%** | 无本工作包目标实现 | **没有** `IntrinsicMeasureService`；`LayoutSystem.cpp` 仍使用 `content.length() * 8 + 10` 估宽和固定 `SCROLLBAR_GUTTER = 14`；CJK/emoji/shaped metrics 验收未做 |
| WP7 RenderSystem 拆分 | **active / partial** | **25%** | 已拆出 `RenderBackend.cpp`、`RenderResources.cpp`、`RenderFrame.cpp`；已有 `RendererRegistry` 基础 | **没有**真正的 `RenderResourceContext` 和 `RenderPipeline` 类型；`RenderSystemImpl` 仍直接持有 9 类 manager/backend、渲染队列和资源；renderer 分派尚未收敛为唯一机制 |
| WP8 API 版本化 | **not-started / foundation** | **15%** | 已有 `ui::entity`、`EntityHandle` 和部分 runtime-bound Factory 重载，公共头迁移提供了基础 | **没有**稳定的 runtime-bound handle 生命周期契约；**没有**裸 entity API 的 `[[deprecated]]` 标记、迁移周期和版本化文档；公开 API 尚未收敛到 `EntityHandle` |
| WP9 构建与发布矩阵 | **blocked / not-started** | **0%** | 仅验证当前 Windows clang-cl Debug 工作树构建 | 项目根目录 **没有** CMake Presets；**没有** UI 库 install/export/package consumer；**没有** Windows/Linux、MSVC/clang-cl/clang/gcc 持续矩阵；需等待 WP3 边界闭环 |

### 已完成的边界迁移批次

| 批次                  | 状态      | 结果                                                                                  |
| --------------------- | --------- | ------------------------------------------------------------------------------------- |
| detail/helper 收敛    | completed | `src/detail/` 清零；内部适配集中到 helper；门禁禁止 detail 文件回归                 |
| 首批公共 API 头       | completed | Entity、Event、Scale、State、Theme、Timer 迁入`include/ui/api/`                     |
| DSL 公共 API 头       | completed | Chains、Hierarchy、Log 迁入`include/ui/api/`                                        |
| 叶子公共 API 头       | completed | Icon、Layout、Query、Size 迁入 `include/ui/api/`；旧路径删除并纳入回归门禁           |
| 公共 Callback 解耦    | completed | 新增 `ui::Callback`；Controls/Text 公开签名移除内部 component callback 依赖          |
| 滚动条几何 DTO        | completed | 新增无 EnTT/Eigen 依赖的公共标量几何类型；移除伪 component DTO，保持原计算与命中语义 |
| Error / Result 公共化 | completed | 权威头位于`include/ui/`；`src/common` 仅兼容转发；公共头禁止旧路径                |
| Policies 公共化       | completed | 权威头位于`include/ui/Policies.hpp`；位运算单点提供；内部 traits 不再重复注入运算符 |
| Ninja 构建限流        | completed | 默认 compile pool=2、link pool=1；默认高并发 clang-cl OOM 已止血                      |

**当前主线：** WP2 已完成。下一优先级是继续 WP3 的公共类型解耦，并继续收敛 WP4 的统一 GPU shutdown。
`Types.hpp`/Eigen 的公开类型决策完成前，不机械迁移 Animation 等头，也不强行收紧 CMake
PUBLIC 传播。WP5～WP9 目前都不能标记为已完成。

## 1. 执行摘要

VMP-ui 已越过“Demo 工程”阶段：C++23、ECS、`Result<T>`、GPU RAII、公开实体句柄、测试分组和架构边界检查均已有实质投入。但它仍处在一次未完成的架构迁移中：**表面上从全局单例迁到了 `UiRuntime`，实际依赖图仍大量依靠 `UiRuntime::current()` 这一线程局部 Service Locator。**

最尖锐的评价是：

1. **Runtime 显式化只完成了一半。** System 构造侧开始注入依赖，API、helper、manager、renderer 及部分 System 实现仍在隐式取全局上下文。
2. **RAII 解决了“谁释放”，没有彻底解决“何时释放”。** GPU deleter 捕获裸 device，资源销毁顺序仍靠人工纪律；代码甚至保留 device 先销毁后主动泄漏缓存的分支。
3. **帧循环不是一条管线，而是多个计时器、队列和即时补救路径的叠加。** System phase 只排序注册，不等于显式执行图。
4. **公开头的隔离方向正确，CMake 却仍把 EnTT 和内部 include 路径向消费者传播。** 源码边界在收，构建边界仍开着。
5. **文档多但状态漂移严重。** 一些计划已部分完成，一些规则仍匹配旧路径；继续堆计划会制造“文档看似治理、实际误导决策”。

当前不应优先扩充控件数量。未来 1～2 个版本的主线应是：**Runtime 生命周期 → 资源 shutdown → 单一帧管线 → 布局测量边界 → SDK/consumer 边界。**

---

## 2. 当前架构判断

```mermaid
flowchart TD
    App[ApplicationImpl] --> Runtime[UiRuntime]
    App --> Loop[ASIO EventLoop]
    Runtime --> Registry[EnTT Registry]
    Runtime --> Dispatcher[EnTT Dispatcher]
    Runtime --> Logger[Logger]
    App --> SM[SystemManager]
    SM --> Input[Input Systems]
    SM --> Layout[LayoutSystem]
    SM --> Render[RenderSystem]
    Layout --> Components[ECS Components]
    Render --> Components
    Render --> Managers[GPU/Font/Image/Icon/Pipeline Managers]
    API[Public API] --> Helper[helper/internal bridge]
    API -. legacy implicit context .-> Runtime
    Helper -. legacy implicit context .-> Runtime
    Managers -. UiRuntime::current .-> Runtime
```

### 已经做对的部分

- `include/ui.hpp` 已形成统一公开入口，公开事件与实体句柄开始隔离 EnTT。
- `include/ui/Result.hpp` 基于 `std::expected`，错误码、上下文和 `source_location` 设计成熟；`src/common/Result.hpp` 仅保留兼容转发。
- System 具备 phase 排序，注册模式统一。
- GPU 资源已有 RAII wrapper，不再完全依赖散落的手工 release。
- 测试已拆分 unit/ecs/api，存在公开头和公开事件测试。
- `tools/check_architecture_boundaries.py` 说明项目已认识到“边界需要机器约束”。

### 尚未闭环的部分

- Runtime 既是服务容器、所有者，又是 thread-local locator 和 registry context 宿主；API、helper、manager、renderer、system 仍存在隐式访问。
- 新旧 Factory API 并存，错误模型和 runtime 绑定不同。
- 主 Dispatcher 与公开自定义事件队列是两套系统。
- Layout 与 Render 虽没有直接类依赖，却通过文字估宽、滚动条魔法常量等语义耦合。
- RenderSystem 仍承担事件协调、资源所有权、缓存、管线和提交等过多职责。
- CMake PUBLIC 传播范围与“稳定 SDK 边界”目标不一致。

---

## 3. 风险清单与锐评

## 3.1 P0：初始化失败时可能二次崩溃

**证据：**

- `src/core/UiRuntime.hpp` 的 `current()` 直接解引用当前指针。
- `ApplicationImpl` 构造失败时，已构造成员会析构，Runtime 可能先清空 current。
- `src/api/Factory.cpp` 的 `CreateApplication()` 捕获异常后仍通过 `UiRuntime::current()` 记录错误。

**评价：** 错误处理路径依赖已经失效的错误处理环境，是典型的“为记录失败而再次失败”。这是确定性缺陷，不是设计偏好。

**建议：**

- 增加 `UiRuntime::tryCurrent() noexcept`。
- 初始化边界把异常一次性转换为 `Result`，catch 中使用不依赖 Runtime 的后备日志。
- 增加 SDL 初始化失败注入测试。

## 3.2 P0：`UiRuntime::current()` 是换了名字的全局单例

**证据：**

- `UiRuntime` 构造时覆盖 current，析构时清空，却不恢复之前的 Runtime。
- `UiRuntimeScope` 才具备保存/恢复的栈语义。
- API、detail、manager、renderer 等多层仍调用 `UiRuntime::current()`。

**评价：** 当前设计宣称“Runtime 化”，但多 Runtime、嵌套测试和同线程多 Application 的契约均不可靠。若继续让 current 扩散，未来每次拆模块都会受隐式环境牵制。

**建议：**

- 只有 `UiRuntimeScope` 能切换 current；Runtime 构造/析构不隐式抢占。
- 旧 API 可暂留 current 兼容入口，但新增代码禁止依赖它。
- manager/render 注入窄依赖（如 logger、resource context），不要引入重量级 DI 容器。

## 3.3 P0：System 移除可能留下悬挂事件连接

**证据：** `SystemManager::removeSystem()` 直接 erase；注册后的 Dispatcher 连接未必先注销。`addSystemBeforeRegister()` 又只是别名，生命周期状态没有显式建模。

**评价：** 对象销毁了，回调还可能认为对象活着；这是潜在 UAF。CRTP 与 `entt::poly` 的组合没有消除生命周期复杂度，反而增加了必须人工遵守的契约。

**建议：**

- 移除前强制 unregister。
- 建模 `ASSEMBLING / REGISTERED / STOPPED` 状态。
- 禁止注册后静默追加；明确重复注册/注销是否幂等。
- 若内建系统集合长期固定，评估静态 tuple；不要为不存在的插件场景长期支付类型擦除成本。

## 3.4 P0：GPU RAII 仍受裸 device 生命周期支配

**证据：**

- `src/common/GPUWrappers.hpp` 的 deleter 捕获裸 `SDL_GPUDevice*`。
- `ImageManager` 已用保存创建 device 的 `UniqueGPUTexture` 持有缓存纹理，但 wrapper 的 deleter 仍捕获裸 `SDL_GPUDevice*`。
- `RenderSystemImpl` 同时拥有多类 manager/cache/backend，清理顺序依赖手工约定。

**评价：** ImageManager 的裸所有权和主动泄漏已止血，但仍未形成完整生命周期闭包。类型系统没有保证 device 一定晚于所有资源 owner 销毁，外部失效或后续成员重排仍可能打破隐藏契约。

**建议：**

- 先建立并测试统一、幂等的 `RenderResourceContext::Shutdown()`。
- 明确资源依赖 DAG，所有 GPU owner 必须先于 DeviceManager 清理。
- 中期在 deleter 中持有可检测失效的 lifetime token/shared state。
- 覆盖初始化中途失败、重复 cleanup、窗口循环重建和 fallback 切换。

## 3.5 P0：源码边界与 CMake 边界互相打架

**证据：**

- `src/CMakeLists.txt` 将 `EnTT::EnTT`、Eigen、spdlog 作为 PUBLIC 链接依赖。
- `${CMAKE_SOURCE_DIR}` 与源码目录作为 PUBLIC include path 暴露。
- `include/ui.hpp` 已努力避免公开 EnTT 类型，现有计划也要求 EnTT PRIVATE。

**评价：** 消费者即使没有在公开签名中看到 EnTT，也会从 target usage requirements 得到内部依赖和内部头访问权。所谓“私有实现”尚未在构建系统中成立。

**建议：**

- EnTT 改 PRIVATE；测试按需显式链接。
- PUBLIC include 仅保留稳定公开目录。
- Eigen/spdlog 是否 PUBLIC 必须以公开签名为依据。
- 增加真正独立的 install/export consumer 编译测试。

## 3.6 P1：帧循环是一组偶然协作的节流器

**证据：**

- EventLoop 线程按约 16 ms 投递。
- loop 内还存在 `SDL_Delay(1)`。
- InputTask 再按 32 ms 节流，RenderTask 再按 16 ms 节流。
- 同时存在 `trigger`、`enqueue/update`、定向 update 和平台即时布局/渲染补救。

**评价：** 当前没有唯一 frame clock。输入延迟、阻塞后补帧、同帧布局次数和 buffered 事件阶段均难以严格推理。`SystemPhase` 只是连接顺序，不是执行计划。

**建议管线：**

```mermaid
flowchart LR
    Tick[FrameTick] --> Poll[Poll platform input]
    Poll --> InputQ[Dispatch queued input]
    InputQ --> Logic[Logic / timer / animation]
    Logic --> PublicQ[Dispatch public queued events]
    PublicQ --> Layout[Layout]
    Layout --> Render[Render]
    Render --> End[EndFrame]
```

节流应成为 scheduler policy，而不是散落在 Task 私有字段中。

## 3.7 P1：公开事件的 `Enqueue` 未必意味着“下一帧”

**证据：** `src/detail/` 已清零；公开事件 callback/queue 当前位于 `src/helper/Helper.hpp` 的
`ui::detail::event_bridge`，由 `src/api/Event.cpp` 转发。主帧路径仍未调用公开事件的
`DispatchQueued()`，因此 `Enqueue()` 不会自动形成明确的“下一帧派发”契约。

**评价：** 两套 buffered event 语义对使用者不可见，API 名称却暗示相同行为。这是契约歧义。

**建议：** 若承诺下一帧派发，将公开队列接入固定 frame stage；否则重命名并明确要求手动 dispatch。补充 disconnect-during-dispatch、递归 trigger 和异常策略测试。

## 3.8 P1：Layout 不 include Render，不代表真正解耦

**证据：** `LayoutSystem.cpp` 包含固定 scrollbar gutter、默认叶节点尺寸及 `content.length() * 8 + 10` 的文字宽度估算。

**评价：** CJK、emoji、组合字符和字体变化会直接击穿该估算。布局层已经知道视觉实现细节，只是耦合从类型层转移到了魔法常量层。

**建议：** 引入窄 `IntrinsicMeasureService`：Text 使用 shaped metrics，Image/Icon 使用固有尺寸，控件使用 theme metrics。Scrollbar thickness 从主题 metric 读取，不抽象整个 Yoga。

## 3.9 P1：RenderSystem 是被注释承认、但尚未处理的 God Object

**证据：** `RenderSystemImpl` 同时协调 device、字体、图标、图像、pipeline cache、text cache、batch、command buffer 和 backend。

**评价：** 继续新增 renderer 会让“接入一个控件”变成“修改中央总管”。若 `RendererRegistry` 与硬编码分派并存，还会形成双轨扩展机制。

**建议：** 只拆两个稳定职责：

1. `RenderResourceContext`：资源所有权、初始化和 shutdown；
2. `RenderPipeline`：collect、sort、batch、submit。

`RenderSystem` 只保留事件订阅和窗口生命周期协调。`RendererRegistry` 要么真正成为唯一入口，要么删除。

## 3.10 P1：错误基础设施成熟，错误政策却不统一

**证据：** 同时存在 `Result<T>`、初始化异常、裸 bool/nullptr、日志后失败和析构 catch-all；README 对错误类型的描述也已过时。

**评价：** 缺的不是新错误框架，而是“一类错误由谁转换、由谁记录”的边界。

**建议策略：**

- 公开边界及可恢复 I/O：`Result<T>`；
- programmer error：assert/contract；
- 构造器内部可抛，但只在最外层转换一次；
- 析构必须 noexcept；
- 错误只在最终消费边界记录，避免重复日志。

## 3.11 P2：架构门禁与文档已发生状态漂移

**证据：** 门禁已覆盖 `src/detail/*` 回归、已迁移 API 旧路径和公共错误头旧路径；2026-07-15 已将过时的
`RuntimeFacade::current()` 规则替换为真实 `UiRuntime::current()` 基线，并增加 PUBLIC include/link 与 queued event
生产派发点指标。`FrameContext` 已补充调度帧序号及 Layout/Render update 次数，`QueuedTask` 在 buffered event
派发前开帧并复位计数，两个 System 在 early return 前记录入口调用；README 与部分 todo 对当前实现描述也已过时。

**评价：** 失真的门禁比没有门禁更危险，因为它制造“检查已通过”的虚假安全感。

**建议：** 给所有架构计划增加 `proposed / active / completed / superseded` 状态；更新门禁以统计当前 `UiRuntime::current()`、PUBLIC include、EnTT 传播和 API 内部头依赖，基线只能下降。

---

## 4. 优先级与风险矩阵

| 优先级 | 工作项                              | 不处理风险                       | 变更风险 |
| ------ | ----------------------------------- | -------------------------------- | -------- |
| P0     | 修复 Application 初始化失败二次崩溃 | 初始化失败直接崩溃               | 低       |
| P0     | 固化 Runtime/current 生命周期契约   | 多 Runtime、测试隔离、析构不确定 | 中       |
| P0     | System 移除前注销                   | 悬挂回调、UAF                    | 低       |
| P0     | GPU shutdown 顺序与故障测试         | 泄漏、悬空 device、重建失败      | 中       |
| P0     | 收紧 EnTT 与内部 include 传播       | 私有实现成为事实公共 API         | 中       |
| P1     | 统一帧时钟与事件阶段                | 输入延迟、乱序、难复现卡顿       | 中高     |
| P1     | 公开事件接入帧循环                  | `Enqueue` 契约不符合预期       | 低中     |
| P1     | 固有尺寸测量服务                    | CJK/emoji/字体布局错误           | 中       |
| P1     | 拆 RenderResourceContext/Pipeline   | God Object 持续膨胀              | 中高     |
| P1     | 收敛新旧公开 API                    | Service Locator 永久化           | 中       |
| P1     | 更新架构门禁                        | 新债无法被 CI 发现               | 低       |
| P2     | consumer/install/package 验证       | 外部集成阶段才暴雷               | 中       |
| P2     | 调度、布局、生命周期集成测试        | 高风险回归依赖手测               | 中       |
| P2     | 文档状态治理                        | 重复规划、错误决策依据           | 低       |

---

## 5. 路线图

## Phase 0：建立基线（2～3 天）

**当前状态：completed。** 静态边界基线已固化并由门禁执行：`UiRuntime::current()` 305 处、PUBLIC 内部 include 路径 2 项、PUBLIC 内部依赖 3 项、queued event 生产帧派发点 0 处。运行时侧由 `FrameContext` 记录单调递增的调度帧序号与当帧 Layout/Render update 入口次数；任务链测试已覆盖常规刷新各一次、同帧即时 Render 追加以及下一调度帧复位。该指标统计 System update 调用，不等同于 Yoga root 数或 GPU present 数。

**目标：** 在改架构前先让债务可计数。

- 统计 `UiRuntime::current()` 的总量及按目录分布。
- 记录 PUBLIC include/link 传播项。
- 记录每帧 Layout/Render 次数和公开 queued event 派发点。
- 为现有 `docs/todo` 标记状态，过时文档不再作为实施依据。

**验收：** CI 输出架构指标；新增债务会失败或至少显式告警。

## Phase 1：止血并固化生命周期（1～3 周）

### WP1 Runtime 与失败路径

**当前状态：active。** 第一轮止血已完成：`tryCurrent()` 已提供；Runtime 构造不再隐式修改 current，异常提前销毁活动 Runtime 时析构会清除 stale TLS 指针；`ApplicationImpl` 通过长期 `UiRuntimeScope` 绑定 legacy API；`CreateApplication()` catch 已改用不依赖 Runtime 的 stderr 后备日志。当前仍需清理非 Application 内部的隐式访问，并补 SDL 初始化失败注入测试。

- ~~增加 `tryCurrent()`；修复 `CreateApplication()` catch。~~ 已完成。
- ~~current 仅由 `UiRuntimeScope` 切换。~~ Application 生命周期已接入；其余旧调用点继续按基线下降。
- 定义无 active Runtime 时兼容 API 的失败行为。

**验收：**

- SDL 初始化强制失败返回 `Result` 且不崩溃。
- 嵌套两个 scope 后 current 逐层恢复。
- 无 scope 调用被测试覆盖，不再产生未定义解引用。

### WP2 System 连接生命周期

**当前状态：completed。** `SystemManager` 已建立 `ASSEMBLING / REGISTERED / STOPPED`
状态机；重复 register/unregister 在 manager 层为 no-op；REGISTERED 状态下 `removeSystem()` 会先注销目标 System
再删除；注册后调用 `addSystem()` / `addSystemBeforeRegister()` 会返回 `false`。新增 manager 级测试已覆盖 phase
排序、幂等、remove-before-unregister、移除后不再接收事件、追加拒绝，以及全部 12 个内建 System 的逐项
phase 契约。

- ~~增加 manager 状态机。~~ 已完成。
- ~~remove 前 unregister；禁止注册后静默追加。~~ 已完成。
- ~~覆盖重复注销和移除后触发事件。~~ 已完成。
- ~~补充全部内建 System 的逐项 phase 契约测试。~~ 已完成。

**验收：** 无悬挂回调；全部内建 system 的 phase 顺序有测试。

### WP3 构建与 SDK 边界

**当前状态：blocked / active。** 多批公共头迁移和公共 callback 解耦已经完成；最终 CMake 收紧仍被剩余 `src/api`/`src/common`、Eigen 公共类型和独立 consumer 缺失阻塞。

- EnTT PRIVATE；PUBLIC include 仅公开目录。
- 区分内部 API 测试和独立 consumer 测试。
- 更新当前路径和符号对应的架构门禁。

**验收：** 独立 consumer 不需要 EnTT include path；公开头检查与现有测试全部通过。

### WP4 GPU shutdown

**当前状态：active。** 渲染源码已初步拆分；ImageManager 主动泄漏已消除，cleanup 已统一资源先于 device 的顺序，但独立资源上下文和真实 GPU 生命周期验收尚未完成。

Application 生命周期 P0 止血已完成：新增私有 `ApplicationLifecycle` 协调器，仅在 `SDL_Init()` 成功后接管
`SDL_Quit()`。正常析构显式断开 Application 回调并注销 System handler，随后通过幂等 `Shutdown()` 执行
`m_systems.reset()`，确保其中的 RenderSystem/GPU 资源先于 SDL 会话退出；若 SDL 初始化后的任一步构造抛出，
成员逆序展开同样先销毁 SystemManager，再由协调器恰好回滚一次 SDL。测试覆盖未 armed 不退出、构造失败回滚一次、
systems-before-quit、重复 shutdown 以及系统销毁步骤抛出后仍执行 quit。本批没有解决 device 已失效时的资源 deleter、
backend 切换或 manager 主动泄漏问题。

最小设备锁定批次已完成：新增纯状态 `DeviceClaimState`，将“首个窗口成功 claim”设为 device/backend 锁定点。
`ensureInitialized()` 现在只建立候选 device、字体和 fallback 基础；PipelineCache/shader、TextTextureCache、CommandBuffer
和 renderer 延后到成功 claim 后创建，因此首窗 backend fallback 销毁候选 device 时尚无旧 device 资源。`DeviceManager`
在已有成功 claim 后遇到新窗口失败会直接返回错误，不再 cleanup 或切换全局 device；frame 补救路径在 claim 失败后立即
跳过窗口，不再继续创建 pipeline，white texture 也只在设备锁定且资源 ready 后创建。该批消除了“资源创建后静默切换
device”的根因，但没有为 GPU wrapper 增加资源代际 token，也没有解决外部原因导致 device 先行失效时的防御性释放。

ImageManager 纹理 RAII 批次已完成：缓存 value 和私有解码/上传链改用现有 `UniqueGPUTexture`，公开 API 继续只返回
借用裸指针；纹理和上传缓冲的失败路径由 owner 自动回收，`releaseAll()` 仅清空 owner 容器，不再查询当前 device、
手工释放或主动泄漏。RenderSystem cleanup 已合并 device-null 分叉，统一在 `DeviceManager::cleanup()` 前销毁 manager、
cache、renderer、command buffer、white texture 和 pipeline，并在末尾复位 `DeviceClaimState`。纯 fake-deleter 测试验证
创建 device 绑定、move 唯一所有权、容器 clear 恰好一次释放及空 owner 行为；本批未启动真实 SDL/GPU。

初始化回滚批次已完成：新增不依赖 SDL/EnTT/Runtime 的固定三节点事务，严格按
`PipelineCache -> TextTextureCache -> CommandBuffer` 提交，并在失败或 shutdown 时按逆序各清理一次。
`PipelineCache::loadShaders()` 已返回内部 `Result<void>`，shader 缺失不再只记录日志后继续进入 resources-ready；
初始化失败进入 failed 状态，避免每帧无界重试。正常 cleanup 与失败回滚复用事务入口，清理 visitor 抛出时仍继续
处理剩余节点。8 项纯逻辑测试覆盖成功、三处失败、乱序提交、失败后禁止重试、未开始 shutdown、重复 shutdown 和
清理异常隔离；测试未启动真实 SDL/GPU。

- 绘制资源 DAG，将清理集中到幂等入口。
- 覆盖中途初始化失败、重复清理、窗口循环创建销毁。

**验收：** 连续创建销毁 100 次稳定；正常路径不再以“主动泄漏”收尾。

## Phase 2：统一执行模型（1 个版本）

### WP5 单一帧管线

**当前状态：not-started / partial。** 局部阶段顺序已经存在，但尚未形成唯一 FrameTick。

- 建立唯一 `FrameTick` 和固定阶段。
- 移除 Task 内散落的 16/32 ms countdown。
- 将 public queued events 接入固定阶段。
- 对必要的即时补救点建立白名单及原因说明。

**验收：**

- 输入附加延迟不超过一帧。
- 默认每帧 Layout/Render 最多一次。
- 所有 queued event 的阶段可测试、可追踪。

### WP6 Intrinsic Measurement

**当前状态：not-started。** 尚无独立测量服务，估宽与固定 gutter 仍待替换。

- Text/Image/Icon/自定义控件通过窄接口提供固有尺寸。
- 删除字符串长度估宽。
- 滚动条尺寸改由 theme metric 提供。

**验收：** CJK、emoji、组合字符与 shaped metrics 在明确容差内一致；Layout 不依赖具体 renderer/manager。

## Phase 3：收敛渲染职责（1 个版本）

### WP7 RenderSystem 拆分

**当前状态：active / partial。** Backend、Resources、Frame 已有物理拆分，职责和所有权仍集中在 RenderSystem。

- 建立 `RenderResourceContext`。
- 建立 `RenderPipeline`。
- 统一 renderer 注册/分派机制。
- GPU 与 CPU fallback 共享可测试的数据阶段。

**验收：**

- `RenderSystemImpl` 不再直接持有大量散列 manager。
- 新 renderer 接入无需修改中央分派。
- fallback 与 GPU 至少共享 collect/sort/batch 测试。

## Phase 4：稳定 SDK 与发布闭环（2～4 个版本）

### WP8 API 版本化

**当前状态：not-started。** 已有句柄与公共头基础，尚未形成版本化和废弃契约。

- 稳定入口收敛到 runtime-bound `EntityHandle`。
- 裸 entity API 进入明确废弃周期。
- 公开事件、错误、句柄生命周期形成契约文档。

### WP9 构建与发布矩阵

**当前状态：blocked。** 依赖 WP3 完成独立 consumer 和 install/export 边界闭环。

- CMake Presets 覆盖 MSVC、clang-cl、Linux clang/gcc。
- 完成 install/export/package consumer 验证。
- 增加 integration/headless、布局 golden、资源生命周期和性能烟雾测试。

**验收：** 至少两平台、三编译器配置持续通过；发布产物可被独立工程消费。

---

## 6. 量化指标

| 指标                            |         基线 |                     目标 |
| ------------------------------- | -----------: | -----------------------: |
| `UiRuntime::current()` 调用数 |          305 | 每版本单调下降；新增为 0 |
| PUBLIC 内部 include 路径        |            2 |                        0 |
| PUBLIC 内部依赖传播             |            3 |                        0 |
| EnTT 对 consumer 传播           |            1 |                        0 |
| 正常可达主动泄漏分支            |     当前存在 |                        0 |
| 默认单帧 Layout 次数            | 常规刷新帧 1 |                     ≤ 1 |
| 默认单帧 Render 次数            | 常规刷新帧 1 |                     ≤ 1 |
| queued event 生产帧派发点       |            0 |                        1 |
| 窗口循环创建/销毁稳定次数       |       未固定 |                   ≥ 100 |
| 独立 consumer 构建              |       未闭环 |                  CI 必过 |
| 架构文档状态缺失                |         多处 |                        0 |

---

## 7. 明确不做

- 不引入完整 DI 容器；优先构造器注入和窄服务接口。
- 不在帧阶段尚未固定前引入通用任务图调度器。
- 不为了“架构纯洁”把每个 System 拆成多层接口。
- 不长期并存硬编码 renderer 分派和 `RendererRegistry` 两套机制。
- 不优先扩充更多控件来掩盖生命周期、焦点、事件和帧管线问题。
- 不照搬旧 todo；先核对代码现状和文档状态。

---

## 8. 决策门

实施前需明确以下产品级选择：

1. 是否支持同线程多 Application/多 Runtime，还是仅要求测试隔离？支持
2. `ui::event::Enqueue()` 是否承诺自动在下一帧派发？承诺
3. 发布目标是源码内嵌静态库，还是可安装 SDK？  都要，可选
4. CPU fallback 是正式后端，还是仅调试/故障降级路径？正式后端
5. 下一主版本是否允许公开 API 收敛到 `EntityHandle` 并弃用裸 entity？是

在这些问题未决前，可安全推进 Phase 0 和 Phase 1；Phase 2 之后的接口形态应受决策门约束。

---

## 9. 建议实施顺序

```mermaid
flowchart LR
    B[指标基线] --> R[Runtime 止血]
    R --> S[System 生命周期]
    R --> G[GPU shutdown]
    S --> F[单一帧管线]
    G --> F
    B --> C[CMake/consumer 边界]
    F --> M[Intrinsic Measurement]
    F --> P[RenderSystem 拆分]
    C --> SDK[稳定 SDK]
    M --> SDK
    P --> SDK
```

**一句话路线：先让失败可控、对象能安全死去、每帧行为可推理，再谈可插拔和 SDK 化。**

---

## 10. API / Helper 边界收敛进展（2026-07-11）

本轮已确立以下边界：

- 外部稳定入口为 `<ui.hpp>`；`src/api/*.hpp` 保持公共声明兼容，但不允许包含 EnTT、`helper/` 或 `detail/`。
- EnTT 实体转换及内部普通 helper 集中到 header-only `src/helper/Helper.hpp`，所有头内定义使用 `inline`。
- 公开 `ui::chains` DSL 保持兼容；内部 helper 不再复制链式 action 或 `operator|` 包装。
- API 的 `.cpp` 负责将公开 `ui::entity` 适配为内部 `entt::entity`，EnTT 原生依赖不进入公开头。
- 已迁移 EntityCast、Utils 内部重载、Size、Visibility、Layout、Hierarchy、Icon、Query、Theme、Event、Canvas、Animation、Controls、Table、Text。
  这里的“Animation”等表示内部实现/适配迁移完成，不表示同名公共头均已物理迁入 `include/ui/`。
- 已删除对应的 detail 实现与重复镜像头；架构检查已增加公共头禁止 EnTT/helper/detail 的规则。

`src/detail/` 已清零。最后五个文件均确认是过期公共镜像或未使用重复声明：`Factory.hpp` 的唯一调用改用权威 `api/Factory.hpp`，`Image.hpp`、`Log.hpp`、`Shortcut.hpp`、`Timer.hpp` 直接删除。架构门禁已增加 `src/detail/*` 禁止回归规则。

公开事件 callback/queue 目前仍通过 `src/helper/Helper.hpp` 内部的 `ui::detail::event_bridge` 适配。这里的 `detail` 是实现命名空间，不表示 `src/detail/` 物理目录仍然存在。

下一阶段的主要边界阻塞已不再是 detail，而是公共头仍物理位于 `src/api`/`src/common`：`include/ui.hpp` 依赖 PUBLIC 暴露整个源码目录，部分公共 API 还可达内部 component 头。因此，收紧 PUBLIC include path 和将 EnTT linkage 改为 PRIVATE，应在“公共头迁入 `include/ui/` + 公共 callback/type 与内部 component 解耦 + 独立 consumer 验证”工作包中完成，不能仅靠修改 CMake 可见性强推。

公共头物理迁移已启动，首批 `Entity.hpp`、`Event.hpp`、`Scale.hpp`、`State.hpp`、`Theme.hpp`、`Timer.hpp` 已迁入 `include/ui/api/`，旧 `src/api` 副本已删除，`ui.hpp` 与内部调用点已切换到新路径。架构门禁同时禁止这批头重新出现在 `src/api`。该批次保持构建和公开调用兼容。

尚未执行 CMake PUBLIC include 与 EnTT linkage 的最终收紧：剩余 API 头仍在 `src/api`，且部分公开签名仍引用位于 `src/common` 的 Eigen 值类型。`Controls.hpp`/`Text.hpp` 的 callback 解耦和 `Utils.hpp` 的 scrollbar geometry 公共值类型已经完成；后续必须形成 Eigen 公开类型决策。

第二批公共 DSL 骨架迁移已完成：`Chains.hpp`、`Hierarchy.hpp`、`Log.hpp` 已迁入
`include/ui/api/`，旧 `src/api` 副本已删除。其余 API 头统一通过
`ui/api/Chains.hpp` 依赖公开 DSL 基础，源码、测试与 `ui.hpp` 对 Hierarchy/Log 的引用也已切换到
稳定公开路径。CMake 头列表和架构门禁已同步，禁止这三个旧路径回归。

第三批无内部类型叶子头迁移已完成：`Icon.hpp`、`Layout.hpp`、`Query.hpp`、`Size.hpp` 已迁入
`include/ui/api/`，实现文件和 `ui.hpp` 已切换到稳定路径，旧 `src/api` 镜像已删除。四个头只依赖 Entity、Chains、
Policies、Result 与标准库，不引入 EnTT、component、Runtime 或 Eigen；架构门禁已禁止旧路径回归，并新增稳定路径
编译测试。本批未触碰 `Types.hpp`/Eigen 的产品级公开类型决策，也未提前收紧 CMake PUBLIC 传播。

公共 callback 解耦批次已完成：新增独立公共头 `include/ui/Callback.hpp`，以
`std::move_only_function<void(Args...)>` 提供 `ui::Callback<Args...>`；`Controls.hpp`、`Text.hpp` 及其 DSL 的公开签名
不再包含或暴露 `common/components/Interaction.hpp` 和 `ui::components::on_event`。内部 `on_event` 暂时保留为
`ui::Callback` 的兼容别名，因此 move-only 语义、参数形式、转发方式和同步调用时机均未改变。架构门禁已禁止公开
API 头重新包含 `common/components/*`，测试同时验证公共类型与内部兼容别名保持同一具体类型。本批仍未迁移
Controls/Text 头，因为其 `Color`/`Vec2` 公开签名继续受 `Types.hpp`/Eigen 决策阻塞。

纵向滚动条几何公共化批次已完成：新增 `include/ui/Geometry.hpp`，其中 `GeometryRect` 和
`VerticalScrollbarGeometry` 仅由 `bool`/`float` 组成，保持 standard-layout 与 trivially-copyable，不依赖 EnTT、Eigen、
component 或 Runtime。原 `common/components/Layout.hpp` 中带 `is_component_tag` 的伪组件定义已删除，Utils 生产者和
StateSystem 消费者统一使用公共 DTO；内部只在 Utils 实现边界把 Eigen `Rect` 拷贝为标量矩形。滚动条公式、最小 thumb、
inset、offset clamp、`visible` 置位时机以及矩形闭区间命中语义均保持不变。该批次没有迁移 `Utils.hpp`，因为其余
`GetAbsolutePosition()`、`GetEntityRect()` 等公开签名仍使用 Eigen `Vec2`/`Rect`。

本轮还针对 clang-cl 解析巨型 header-only Helper 时的高内存占用，为 Ninja 增加全局可配置任务池：
默认编译并发为 2、链接并发为 1。该措施避免默认按 CPU 核数启动大量重型翻译单元导致
`LLVM ERROR: out of memory`；它是构建稳定性止血，不能替代后续按领域降低 `Helper.hpp` include fanout。

Phase 0 静态架构基线已于 2026-07-15 落地到 `tools/check_architecture_boundaries.py`。门禁现在直接统计并
baseline 真实 `UiRuntime::current()`（当前 302 处）、PUBLIC 内部 include 路径（2 项）以及 EnTT/Eigen/spdlog
PUBLIC 依赖（3 项）；任何新增或基线过期都会失败。公开 queued event 在生产帧路径中的自动派发点当前为 0，
该指标会持续输出，但在 WP5 接入固定 FrameTick 前不把“缺少派发点”作为构建失败。Phase 0 剩余项仅为
Layout/Render 每帧次数的运行时埋点与 CI 留存。

执行快照（2026-07-17，Debug）：构建成功；架构门禁和公开头检查通过；GpuInitializationTransaction 定向测试 8 passed / 0 failed，全量测试 165 passed / 0 failed。新增纯逻辑测试覆盖固定顺序提交、三处初始化失败逆序回滚、失败后禁止重试、未开始/重复 shutdown 和清理异常隔离；未启动真实 SDL/GPU。架构指标为 `UiRuntime::current()` 302 处、PUBLIC 内部 include 2 项、PUBLIC 内部依赖 3 项、生产帧 queued-event 派发点 0。
剩余公共头仍受 `src/common` Eigen 值类型以及 Factory/Runtime 边界阻塞；下一批应完成基础数学值类型决策，
不应直接强制收紧 PUBLIC include/link。

错误基础设施公共化批次已完成：权威 `ErrorCodes.hpp`、`Result.hpp` 已迁入 `include/ui/`，
`src/common` 下保留兼容转发头，内部实现和测试已切换到稳定公共路径。架构门禁现已禁止公共头重新包含
`common/ErrorCodes.hpp` 或 `common/Result.hpp`。本批未改变 `UiErrc`、`Error`、`Result<T>`、`TRY` 宏或错误码数值，
因此保持源码兼容；这里的 completed 仅指头文件位置和依赖方向，异常、裸 `bool`/`nullptr` 等错误政策统一仍属后续工作。

Policies 公共化批次已完成：权威定义迁入 `include/ui/Policies.hpp`，不再依赖内部
`traits/PoliciesTraits.hpp`；`src/common/Policies.hpp` 仅保留兼容转发，全仓调用点已切换到 `ui/Policies.hpp`。
位运算和 `HasFlag` 现在由公共头单点提供，并仅允许实际 flags 类型；旧 traits 仍保留内部类型检测兼容，
但不再重复注入运算符。架构门禁已禁止公共头重新依赖 `common/Policies.hpp` 或 `src/traits`。

以上结果复用同一份 2026-07-16 Debug 执行快照，不代表 WP3、Runtime 或发布矩阵已经验收完成。
下一批候选是动画公共类型，但 `src/api/Animation.hpp` 的公开签名仍依赖 `Types.hpp` 中的 Eigen 别名，
因此应先决定“原样公共化 Eigen 类型”还是引入自有数学值类型，不能仅移动 Animation 头来伪造 SDK 隔离。
