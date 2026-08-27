# VMP-ui 当前架构与实现锐评及优化规划

> 日期：2026-08-25；最近推进：2026-08-27
>
> **审查边界**：本文只参考当前工作区的 `todo/**`、`docs/todo/**` 和当前源代码、测试代码、CMake 文件。未参考历史记忆、Git 历史、问题日志、既有架构评审、测试报告或会话结论。
>
> **文档性质**：这是问题审查和实施规划，不代表本文列出的验收项已经完成。每项结论都应在实施前重新以当前代码和可重复测试确认。
>
> **状态**：ACTIVE（核心阶段 A-C 已完成；本文继续作为剩余专项索引）  
> **最后复核**：2026-08-27  
> **责任范围**：运行时、事件、渲染、调度、构建和 TODO 治理  
> **验收**：按本文分阶段验收标准执行专项测试和 Debug 全量构建

### 变更记录（2026-08-26）

> 以下记录按时间保留。各章节中的“当前证据/风险/优化建议”是实施前审查材料；当前结论以对应章节最后一条“推进记录”和本文第 10 节为准。

- P1-3 本轮完成生命周期收紧：`UiRuntimeScope` 通过共享 control block 恢复前一个 Runtime；前一个 Runtime 提前销毁后，恢复路径不会再写回悬空裸指针。
- P1-7 本轮完成 SDL event-driven 唤醒：SDL watch 仅发送轻量调度通知，Registry/Dispatcher 转换统一回到 UI 消费线程；同时修复运行中 EVENT_DRIVEN → FIXED_RATE 无法即时唤醒和纪元被新等待快照吞掉的问题。
- 新增嵌套 Scope 与前一 Runtime 提前销毁回归测试；Debug 构建通过。
- 修正 `BackendRenderContractTest` 对当前背景/边框合并 batch（8 顶点）的过时断言；该变更只同步测试契约，不改变渲染实现。
- P1-2 已完成：生产源码除 `UiRuntime::current()` 定义本身外零调用；P1-7、P2-1、P2-2 按各自专项状态维护，不得以其他测试替代专项验收。
- `docs/todo/README.md` 现作为 TODO 状态唯一索引；功能规划仍为 PLANNED/NEEDS-REVIEW，不因架构治理推进而自动关闭。
- P1-2 完成首个窄边界迁移：`WindowEntityLookup.hpp` 的缓存、匹配、失效和查找函数改为显式接收 `Registry&`，`Factory.cpp` 已传入当前 Registry；架构门禁基线移除该文件 6 处旧债务，ambient 总数由 347 降至 341。
- P1-2 完成第二批系统边界迁移：`TimerSystem` 与 `StateSystem` 保存构造注入的 Logger，移除实现文件内 5 处 `UiRuntime::current()`；门禁基线删除对应文件条目，ambient 总数由 341 降至 336。
- P1-2 完成本轮两个窄边界迁移：`WindowSync.hpp` 的 Registry/Logger 依赖改为显式参数；`ImageManager` 构造注入 Logger，并将解码与 GPU 上传日志改为显式传递。两项旧 baseline 已删除，P1-2 仍保持 ACTIVE，待继续收敛其余 ambient 调用。
- 2026-08-27 P1-2 最终收口：RenderSystem 与 manager 日志全部构造注入；Helper/Animation/Canvas 统一显式 Registry；公共 Factory、Event、Theme、Controls、Layout、Size、Text、Table、Image、Log、Utils 等 API 改为显式 `UiRuntime&`；Runtime-aware Chain 通过 `WithRuntime` 绑定。架构门禁 runtime-current baseline 归零。
- 最终验证：Debug `all` 构建通过；公共头自包含和架构边界门禁通过；unit 标签 108/108 通过。串行构建用于避免 clang-cl 并行编译的内存峰值，不改变产品配置或验收标准。

---

## 1. 尖锐总评

当前项目已经不是原型：它有明确的 ECS UI 主链路、固定帧阶段、公共头隔离、`Result<T>`、GPU/CPU 渲染路径、OBJECT 库拆分和数量可观的测试。

但当前架构的主要问题不是“缺少更多控件”，而是**底层运行时契约没有收紧，部分设计意图停留在注释、TODO 或测试名称中，没有被代码和门禁强制保证**。

最直接的判断如下：

1. **并发事件循环的计数与队列发布协议存在可证明的竞态风险**，不能用压力测试偶尔通过来替代内存模型证明。
2. **公共事件 API 仍依赖 ambient `UiRuntime::current()`**，连接对象没有保存运行时归属；所谓显式 Runtime 目前主要解决了所有权，没有解决依赖传递和生命周期隔离。
3. **事件回调表在派发期间允许用户代码修改容器，但没有快照契约**，这是典型的回调重入与迭代器失效风险。
4. **固定帧管线的 typed dispatcher 修复避免了 EnTT 全量遍历失效，但事件清单仍是手工白名单**，新事件可能成功入队却永远不被派发。
5. **渲染失败与 dirty 状态的契约不完整**，多窗口资源状态仍有明显的共享单窗口假设。
6. **TODO 文档已经同时承担需求、验证记录、旧规划和已完成事项，需求源不再可靠**。继续堆叠新控件规划，会掩盖核心运行时风险。

因此，当前阶段不建议优先扩展 RichText、SVG 或更多控件。应先修复事件循环、事件连接生命周期、回调重入、渲染重试和多窗口状态隔离，再扩展上层能力。

---

## 2. 证据范围与当前基线

### 2.1 当前代码基线

重点代码区域：

- `src/utils/EventLoop.hpp`
- `src/utils/MpscQueue.hpp`
- `src/core/EventLoop.cpp`
- `src/core/TaskChain.hpp`
- `src/helper/Helper.hpp`
- `src/api/Event.hpp`
- `src/api/Event.cpp`
- `src/core/UiRuntime.hpp`
- `src/core/UiRuntimeScope.hpp`
- `src/systems/render/RenderFrame.cpp`
- `src/systems/SystemManager.*`
- `tests/support/UiTestRuntime.hpp`
- `tests/unittest/`
- 根目录和子目录 CMake 文件

### 2.2 TODO 文档呈现出的状态（历史审查快照）

审查时 `todo/` 和 `docs/todo/` 中既有：

- 已完成的构建/测试闭环记录；
- 仍在规划的 Theme、Overlay、Focus、Rich Text、SVG、HiDPI 等能力；
- 问题日志型文档；
- 旧的 UI 能力缺口总表；
- 事件 API 边界规划。

例如 `todo/P2-1-listview-build-verify.md`、`todo/P2-2-contextmenu-modal-verify.md`、`todo/P2-4-animation-lifecycle-build-loop.md` 已经包含完成状态，但没有统一的归档规则；这类文件不能继续与未开始计划放在同一优先级列表中。

---

## 3. P0：必须优先处理的问题

## P0-1：EventLoop 的任务计数与队列发布存在竞态

### 当前证据

`src/utils/EventLoop.hpp` 的生产者路径先向 `MpscQueue` 发布任务，再更新 `pending_tasks_`；消费者在成功出队后先减少 `pending_tasks_`，再执行任务。

`src/utils/MpscQueue.hpp` 的槽位通过 sequence 发布。由此存在以下交错：

1. 生产者完成队列槽位发布；
2. 消费者立即出队并对计数执行 `fetch_sub(1)`；
3. 生产者尚未完成 `fetch_add(1)`。

`pending_tasks_` 为无符号计数时，计数可能从 0 下溢到极大值。

### 风险

- `PendingCount()` 不再可信；
- `Exit(..., true)` 的排空条件可能失真；
- 队列事实和统计事实不一致；
- 概率型压力测试通过不能证明正确性。

### 优化建议

不要简单交换 `fetch_add` 顺序，因为入队失败又会产生“计数已增加但队列没有任务”的反向竞态。

推荐方案：

- 将队列可消费状态作为唯一事实源；
- 将唤醒 epoch 与任务统计分离；
- 排空退出明确包含“停止接收、队列为空、没有尚在发布中的生产者”；
- 如需精确计数，引入与队列发布协议一致的 active-producer/发布闸门，而不是继续把 `pending_tasks_` 同时当统计值和停止谓词。

### 验收标准

- 通过可控同步点强制复现“队列已发布、计数尚未更新”的交错；
- 单生产者、多生产者、队列满、并发退出全部不下溢；
- `Exit(0, true)` 排空所有成功入队任务；
- `Exit(0, false)` 的丢弃语义明确且可测试；
- 任务抛异常时计数、唤醒和退出状态仍保持一致；
- 可用平台上使用 ThreadSanitizer 或等价并发检测工具验证。

---

## P0-2：EventConnection 没有 Runtime 归属

### 当前证据

`include/ui/api/Event.hpp` 的 `EventConnection` 主要保存 token；`src/api/Event.cpp` 的析构路径最终通过 `event_bridge::Disconnect(token)` 和当前 Runtime 查找回调表。

当前 Runtime 的 token 从各自上下文重新开始计数。因此，连接在 Runtime A 创建、Runtime B 中析构时，存在操作 B 的无关 token 的可能。没有活动 `UiRuntimeScope` 时，析构路径还可能触发 `UiRuntime::current()` 的终止行为。

### 风险

这是公共 RAII 类型的基础生命周期错误：析构不应依赖调用时恰好存在正确的 TLS Runtime。

### 优化建议

让连接保存稳定的事件域控制块：

- Runtime 创建独立 control block；
- `EventConnection` 保存 control block + token；
- control block 有效时按归属 Runtime 断开；
- Runtime 销毁后 control block 标记失效，连接析构安全变为空操作；
- 不直接保存裸 `UiRuntime*`，避免把隐式悬空变成显式悬空。

### 验收标准

必须覆盖：

1. A 创建连接、B 析构连接，不影响 B；
2. 无 Scope 析构不崩溃；
3. Runtime 先销毁、连接后析构不崩溃；
4. move 构造/赋值只保留一个有效所有者；
5. 重复 `Disconnect()` 幂等；
6. 不同 Runtime 中相同数值 token 绝不串扰；
7. Runtime 销毁后 `Connected()` 安全返回 false。

---

## P0-3：事件回调派发期间修改 vector

### 当前证据

`src/helper/Helper.hpp` 的事件回调表使用 `std::vector<CallbackSlot>`；派发过程直接遍历该 vector，而回调可以调用 `Connect()` 注册同类型新回调。

如果注册触发 `push_back()` 扩容，派发中的迭代器或引用可能失效。断开只标记连接状态并不能解决注册扩容，也没有解决槽位长期积累问题。

### 风险

- 随机崩溃或内存破坏；
- 当前轮是否执行新回调没有明确定义；
- 高频连接/断开导致容器无限积累；
- 嵌套触发顺序不可证明。

### 优化建议

定义并实现快照语义：

- 派发开始时固定本轮回调集合；
- 派发期间新增连接从下一轮生效；
- 派发期间断开对尚未执行回调的生效时机明确；
- 派发结束后压缩断开槽位。

第一版只需稳定 callback handle 快照，不需要引入通用信号框架。

### 验收标准

- A 执行期间注册 B：本轮 B 不执行，下一轮执行一次；
- 自断开安全；
- A 断开 B 的行为符合书面契约；
- 嵌套触发顺序稳定；
- 回调异常时剩余回调和连接状态定义明确；
- 高频连接/断开后槽位能够回收。

---

## 4. P1：应在核心运行时稳定后处理

## P1-1：渲染失败后全局清除 RenderDirtyTag

### 当前证据

`src/systems/render/RenderFrame.cpp` 在窗口提交失败、尺寸无效、资源未就绪、pipeline/fallback 初始化失败等路径后，帧尾仍可能遍历 Registry 并清除全部 `RenderDirtyTag`。

### 风险

失败窗口丢失重试信号；多窗口场景下一个窗口成功会掩盖另一个窗口失败。

### 优化建议

按窗口提交结果清除 dirty：

- 成功提交后才清除对应窗口及其已消费子树；
- 可恢复错误保留 dirty；
- 不可恢复错误进入显式 backend/window error 状态，并限制日志频率。

### 验收标准

- 模拟一次提交失败，下一帧自动重试；
- A 成功、B 失败时只清除 A；
- B 恢复后无需额外属性变化即可渲染；
- dirty 清理范围有单元或集成测试。

### 推进记录（2026-08-25）

状态：**DONE**。

已完成：

- 删除 `RenderFrame.cpp` 帧尾对全 Registry 的 `RenderDirtyTag` 全局清理；
- GPU `CommandBuffer::execute()` 改为返回 `Result<void>`，command acquire、swapchain、copy/render pass、submit 等失败可传播到帧级；
- fallback 的 begin/draw/present 改为返回 `Result<void>` 并检查 SDL 失败；
- 仅在对应窗口成功提交后调用 `ClearRenderDirtySubtree()`，失败窗口、其他窗口及 detached dirty 实体保留重试信号；
- 子树清理对失效 child 和异常层级环安全；
- `IRenderer::collectChecked()` 建立渐进式错误契约；Image/Text fallback cached bitmap 失败可传播到帧管线，任一 collect 失败均不提交、不清 dirty；
- 新增 `CommitRenderDirtyOnSuccess()` 作为提交结果与 dirty 消费的唯一策略入口；
- 新增 4 个 dirty 契约测试：失败保留、无重新标脏恢复、同轮 A 成功/B 失败、B 后续恢复、子树范围及异常层级均通过，`RENDER_DIRTY_EXIT=0`；
- 后端/GPU 相关单测 23/23 通过；fallback 生命周期通过 CTest；
- 全量 Debug 构建通过，`ALL_BUILD_EXIT=0`，公共头与架构边界门禁通过。

后续增强项（不阻塞本项关闭）：

- 当前双窗口失败/恢复测试在提交策略层确定性验证；可在后续多窗口 render state 工作包中增加真实双 SDL 窗口集成覆盖；
- 尚无可注入的 fallback SDL draw/present fault seam，当前通过 Result 契约和 GPU failure injection 保证失败路径不清 dirty；
- 无失败源的旧 renderer 由 `collectChecked()` 默认适配，未来新增可失败操作必须覆盖 checked 入口。

## P1-2：显式 DI 与 ambient Runtime 并存

### 当前证据

`UiRuntime` 显式拥有 Registry、Dispatcher、Logger，但多个 manager/system/helper 路径仍调用 `UiRuntime::current()`。`tools/check_architecture_boundaries.py` 主要阻止债务增加，不能自动消除已有 ambient 依赖。

### 风险

- 类型签名无法表达真实依赖；
- 测试必须构造 TLS Scope；
- 异步和跨线程调用容易丢失 Runtime；
- 多 Runtime 隔离依赖调用者自律。

### 优化建议

按边界渐进式去 ambient 化：

1. 先改公共 EventBridge 为显式事件域；
2. System/manager 保存构造注入的 Registry、Dispatcher、Logger；
3. `UiRuntime::current()` 仅保留在最外层 convenience API；
4. 收紧 `UiRuntime::s_current` 可见性；
5. 将现有检查从 baseline 防增长，逐步升级为允许列表 + 债务下降门禁。

### 验收标准

- 新增源码不得直接调用 `UiRuntime::current()`；
- System/manager 测试可不创建 TLS Scope；
- 两个 Runtime 的 timer、event、render context 不串扰；
- 暂时保留的 ambient API 有明确白名单。

### 当前结论（2026-08-26）

状态：**DONE**。生产源码中除 `UiRuntime::current()` 的定义与断言文本外已无调用；架构门禁不再保留 runtime-current baseline。公共 API、Chain、Factory、Helper、System 与 Manager 的运行时服务均由 `UiRuntime&`、`Registry&`、`Dispatcher&` 或 `Logger&` 显式传递。

## P1-3：UiRuntimeScope 保存裸恢复指针

### 当前证据

`UiRuntimeScope` 构造时保存前一个 Runtime 指针，析构时无条件恢复；Runtime 析构只处理当前值，不会让其他 Scope 保存的历史指针失效。

### 风险

嵌套 Scope 或异常展开时，外层 Runtime 若提前销毁，内层 Scope 退出后可能恢复悬空指针。

### 优化建议

二选一：

- 建立严格栈式生命周期并在代码、测试和文档中强制 Runtime 必须晚于 Scope 销毁；
- 或使用可失效 control block，让 Scope 恢复前验证 Runtime 仍存活。

不应同时保留“严格顺序假设”和“异常提前销毁可安全处理”的模糊语义。

### 验收标准

覆盖嵌套 Runtime、嵌套 Scope、异常展开和不同析构顺序；违反顺序时必须可诊断，不能留下悬空 TLS。

## P1-4：typed dispatcher 手工白名单容易漏事件

### 当前证据

`src/core/TaskChain.hpp` 的 `DispatchInternalQueued()` 手工列出各类内部缓冲事件。`Dispatcher::enqueue<T>()` 不会自动把 T 纳入列表，事件标签也没有形成编译期登记约束。

### 风险

新事件可以正常编译和入队，但永远不会被更新，表现为静默失效。

### 优化建议

保留逐类型 update，不恢复 EnTT 无类型全量遍历；同时建立唯一事件目录：

- 每个 buffered 事件声明派发阶段和顺序；
- `DispatchInternalQueued()` 从集中目录展开；
- 通过 lint 或编译期测试检查生产代码中的 `enqueue<T>` 是否登记。

### 验收标准

- 所有生产 `enqueue<T>` 都能映射到唯一帧阶段；
- 未登记类型导致编译或门禁失败；
- Raw → Hit → Hover 顺序有回归测试；
- `Events.hpp` 的即时/缓冲注释与实际管线一致。

### 推进记录（2026-08-25）

状态：**DONE**。

- 审计生产代码全部内部 `Dispatcher::enqueue<T>`：共 13 种事件，与原 typed update 白名单集合一致，无既有漏派发；
- 新增 `src/common/BufferedEvents.hpp`，以有序类型列表作为内部 buffered 事件及派发顺序的唯一事实源；
- `DispatchInternalQueued()` 从集中目录展开逐类型 `update<T>()`，继续禁止 EnTT 无类型全量 update；
- `Dispatcher::enqueue()` 增加编译期登记断言，新事件未加入目录即无法通过编译；公开动态 EventBridge 保持独立队列，不受内部类型目录约束；
- 修正 `Events.hpp` 中 6 个实际 buffered 事件被标为 immediate，以及 5 个未接入/即时事件被误标为 buffered 的注释漂移；
- 新增目录数量、正反 membership 测试，并验证 Raw → Hit → Hover 在一次内部队列阶段按目录顺序完成链式派发；
- `TaskChainTest` 5/5 通过；全量 Debug 构建通过，`ALL_BUILD_EXIT=0`，公共头和架构边界门禁通过。

后续增强项（不阻塞本项关闭）：

- `ApplicationReadyEvent`、`ValueChangedText`、`ValueChangedSelection`、`SendHandlerToEventLoop` 当前为未接入的遗留/预留声明；后续应结合实际功能决定接入或删除，而不是仅凭旧注释加入 buffered 目录；
- 公共 EventBridge 的运行时 `EventId` 队列具有独立快照契约，不应与内部 typed buffered 目录合并。

## P1-5：多窗口缩放和渲染资源仍有单窗口共享状态

### 当前证据

窗口组件保存了每窗口缩放和尺寸，但 `RenderFrame.cpp` 对每个窗口切换共享 FontManager DPI 和文本缓存；缩放日志快照使用 function-static 状态。fallback renderer 的初始化也围绕当前窗口切换。

### 风险

- 不同 DPI 窗口之间反复失效共享文本缓存；
- 后处理窗口改变前一窗口依赖状态；
- 不同 Runtime 的缩放日志互相抑制；
- 多窗口关闭顺序难以证明安全。

### 优化建议

引入最小 per-window render state，以 window/entity 为 key，仅保存：

- backend/window 绑定；
- 离散 scale key；
- 窗口提交状态。

字体缓存按 scale key 分区，不要因为窗口切换而清空全部缓存。不要在此阶段引入完整 render graph。

### 验收标准

- 两个 DPI 不同的窗口连续渲染不反复清空全部文本缓存；
- 窗口关闭顺序任意且 backend 不串绑；
- 两个 Runtime 的缩放状态隔离；
- A → B → A 的 scale key 能验证缓存复用。

### 推进记录（2026-08-25）

状态：**DONE**。

- 新增 `WindowRenderState`，由 `RenderSystem` 按 SDL window ID 持有 fallback backend、离散文本 scale key 和缩放日志快照；移除进程级 function-static 快照；
- fallback backend 改为逐窗口创建、查询和销毁，窗口关闭时仅释放对应状态，系统 cleanup 时统一清空，避免 SDL_Renderer 跨窗口串绑；
- 帧渲染中的屏幕尺寸改为循环局部状态，不再由后处理窗口覆盖共享成员；
- `FontManager` 切换 DPI 时不再清空全部 glyph cache；文本纹理 key 已包含实际字号和 oversample scale，因此不同 scale 分区可共存，窗口 A → B → A 可复用原分区；
- 新增双 offscreen software 窗口测试，验证两个 SDL_Renderer 独立、A → B → A 交替 present、先销毁 B 后 A 仍可提交；新增 scale-key 量化与 A → B → A 稳定性测试；
- fallback 生命周期测试 2/2 通过；全量 Debug 构建通过，`ALL_BUILD_EXIT=0`，公共头和架构边界门禁通过；同时将 `RenderFrame.cpp` 的 ambient Runtime 债务基线由 10 收紧至 8。

后续增强项（不阻塞本项关闭）：

- GPU pipeline 若未来确认受 swapchain format 或窗口设备能力影响，再按实际兼容维度分区；本阶段不提前引入 render graph；
- 当前跨 Runtime 隔离由 render state 的 `RenderSystem` 实例所有权保证，可在多 Runtime 同进程测试基础设施成熟后补直接集成覆盖。

## P1-6：`fps=0` 公开语义与实现不一致

### 当前证据

`tests/unittest/test_CoreEventLoopScheduling.cpp` 将 `setTargetFrameRate(0)` 描述为不锁帧，但 `src/core/EventLoop.cpp` 对非正 FPS 使用约 16ms fallback。

### 优化建议

统一代码、头文件、测试和 TODO 的语义。建议使用显式 schedule mode，不让 0 同时表示默认 60 FPS和 unbounded。

### 验收标准

帧率 0 的定义在 API、实现、测试和规划文档中完全一致，并有实际帧间隔或调度行为验证。

### 推进记录（2026-08-27 复核）

状态：**DONE**。

- 明确 `setTargetFrameRate(0)` 表示恢复默认固定帧率 60 FPS，不再声称“不锁帧”；真正的不锁帧若未来需要，必须增加显式调度模式，不能复用数值哨兵；
- setter 在写入原子状态前将 0 规范化为 `kDefaultTargetFrameRate`，因此 `targetFrameRate()` 返回值、调度器实际输入和文档契约保持一致；
- 删除历史 `16ms` fallback，固定帧率调度只从已规范化的非零 FPS 计算微秒间隔，消除 0 对应 62.5 FPS 的隐式行为；
- 更新 `CoreEventLoopSchedulingTest.TargetFrameRateCanBeChanged`，断言设置 0 后查询值为 60；新增 `ZeroFrameRateUsesDefaultThrottleWithoutBusyLoop`，验证默认调度继续产帧且不会退化为忙循环；
- `CoreEventLoopSchedulingTest` 4/4 通过；全量 Debug `all` 构建通过，公共头自包含与架构边界门禁通过。

后续增强项（不阻塞本项关闭）：

- 极端高 FPS 仍可因整数微秒截断产生零间隔；若该内部接口未来对外暴露，应增加上限或改用更高精度 duration；
- EVENT_DRIVEN 运行中切回 FIXED_RATE 的即时唤醒语义属于 P1-7/调度模式切换问题，另行处理。

## P1-7：EVENT_DRIVEN 模式无法由外部 SDL 事件直接唤醒

### 当前证据

调度器的 event-driven 等待主要依赖 invoke/quit/schedule epoch；SDL 输入由帧内 `pollInput()` 主动轮询。若调度器已睡眠，SDL 事件本身不会自然推进 epoch。

### 风险

鼠标、键盘、resize、expose 可能无法在没有其他 invoke 的情况下推进下一帧。

### 优化建议

明确唯一唤醒 owner：

- 主事件循环阻塞等待 SDL 事件并同时处理任务；或
- SDL event watch 只发轻量 wake signal，实际 Registry/Dispatcher 操作仍在消费线程完成。

不要在 SDL callback 中直接修改 Registry。

### 验收标准

EVENT_DRIVEN 下分别注入 pointer、keyboard、resize、expose，均可在无 invoke 的情况下推进一帧；空闲时不持续产帧。

### 推进记录（2026-08-26）

状态：**DONE**。

- `EventLoop` 新增 `notifyExternalEvent()`：只执行原子纪元递增与条件变量通知，可由 SDL event watch 跨线程安全调用，不触碰 Registry、Dispatcher 或日志系统；`invoke()` 和调度模式切换复用同一唤醒原语；
- EVENT_DRIVEN 调度改为持久保存 `observedEpoch`，等待谓词同时检查停止、运行状态、模式变化和纪元变化，消除“通知发生在下一次 wait 前却被新快照吞掉”的窗口；
- `setFrameScheduleMode()` 发布新模式后递增纪元，EVENT_DRIVEN → FIXED_RATE 可立即退出等待并推进一帧，不再等待旧的事件驱动通知或完整固定帧周期；
- `ApplicationImpl` 在 SDL 初始化及业务 handler 注册完成后安装纯唤醒 watch，并在系统注销和 SDL shutdown 前移除；回调只调用 `notifyExternalEvent()`；
- SDL watch 已抽取为生产 RAII seam `SdlEventWakeup`，构造安装、析构移除，不可复制/移动；callback 唯一动作是调用 `EventLoop::notifyExternalEvent()`，不访问 Registry、Dispatcher 或 Logger；
- 删除 `PlatformWindowSystem` 的 SDL callback，避免 SDL 任意线程直接访问 Dispatcher；窗口缩放、像素尺寸、移动和 expose 事件统一由 `InteractionSystem::pollSdlEvents()` 在 UI 线程转换为 buffered events；
- 新增独立 `ui_sdl_event_driven_tests`：真实 `SDL_PushEvent` 注入 mouse motion、key down、window resized、window exposed，4 类事件均在无 `invoke()` 下推进默认帧并转换为内部事件，handler 线程与 EventLoop 消费线程一致；同时覆盖空闲不持续产帧、EVENT_DRIVEN ↔ FIXED_RATE 双向切换、切回后 SDL 再唤醒和 quit 唤醒退出；
- 2026-08-27 当前工作区验收：Debug `all` 串行构建通过（`ALL_BUILD_EXIT=0`）；`ctest -L event-driven` 4/4 通过；`ctest -L unit` 108/108 通过；架构门禁通过且生产 `UiRuntime::current()` 调用为 0，公共头第三方依赖为 0。

后续增强项（不阻塞本项关闭）：

- 当前内部接口已经覆盖所有 SDL 队列事件，因为 watch 不维护事件类型白名单；若未来将 EVENT_DRIVEN 暴露为 Application 公共配置，应另行定义动画和定时器在该模式下的驱动策略；
- 若未来支持更多 SDL 事件类型，可在同一专项测试目标中扩展类型矩阵；当前 P1-7 验收要求的 pointer、keyboard、resize、expose 已全部覆盖。

## P1-8：主库 shared 选择被第三方静态策略覆盖

### 当前证据

VMPUI 作为库允许用户通过 `BUILD_SHARED_LIBS` 选择静态或动态产物，这一设计本身正确。问题在于顶层 CMake 为保证第三方依赖静态构建，执行了 `set(BUILD_SHARED_LIBS OFF ... FORCE)`，但进入 `src/` 前没有恢复用户值，导致用户显式传入 `ON` 仍会进入主库静态分支。

### 风险

主库构建类型与用户输入不一致，shared 分支事实上不可达；同时，若直接取消第三方静态约束，又会让 SDL、FreeType 等依赖随主库选择变为动态，破坏单一 VMPUI 发行产物。

### 优化建议

保留用户对 VMPUI 主库类型的选择，并隔离主库与第三方的构建策略：

- 顶层保存用户的 `BUILD_SHARED_LIBS`；
- 配置第三方期间临时强制 `OFF`，确保依赖静态封装；
- 进入 `src/` 前恢复用户值；
- Windows shared 过渡期使用自动符号导出保证消费者可链接，稳定 ABI 阶段再迁移到显式公共 API 导出宏；
- 静态与 shared 都执行安装后独立消费者验证。

### 验收标准

- 用户选项与配置输出一致；
- 主库为 shared 时，SDL 等编译型第三方仍为 static；
- 静态/动态支持矩阵只有一个事实源；
- shared 模式必须生成 DLL 和导入库，并通过独立安装消费者的配置、链接与运行测试。

### 推进记录（2026-08-25）

状态：**DONE**。

- 顶层新增主库 `BUILD_SHARED_LIBS` 用户选项并保存其值；第三方配置期间仍强制静态，配置完成后恢复用户选择；
- 删除 `src/CMakeLists.txt` 中重复的 shared/static 选项声明，主库聚合目标仅依据顶层唯一选项选择 `STATIC` 或 `SHARED`；
- shared 模式启用位置无关代码；Windows 使用 `WINDOWS_EXPORT_ALL_SYMBOLS` 作为当前过渡导出策略；
- 独立 shared 配置确认：SDL `Build Shared Library: OFF`、`Build Static Library: ON`，VMPUI `Library type: SHARED`；
- 成功生成 `VMPUI.dll`、`VMPUI.lib` 导入库和 PDB；
- shared SDK 安装后，`tests/install_consumer` 通过独立 `find_package(VMPUI)` 配置、编译、链接并成功运行；
- 原静态 Debug 全量构建通过，`ALL_BUILD_EXIT=0`，静态单库合并、公共头和架构边界门禁无回归。

后续增强项（不阻塞本项关闭）：

- `WINDOWS_EXPORT_ALL_SYMBOLS` 解决当前可用性，不等于稳定 ABI；正式承诺 ABI 前应将公共 API 迁移到统一导出宏并增加导出符号基线；
- 第三方项目自身带有 install 规则，当前全工程 install 会附带部分第三方开发文件；自包含发行包瘦身应作为独立安装治理项处理，不影响 VMPUI shared 可构建和可消费结论。

## P1-9：测试 fixture 仍可能使用全量 dispatcher update

### 当前证据

`tests/support/UiTestRuntime.hpp` 和部分测试清理路径使用无类型 `dispatcher.update()`，而 `src/core/TaskChain.hpp` 的生产路径已经明确避免无类型全量 update。

### 风险

测试清理机制和生产安全规则不一致，可能偶发复现迭代器失效，也会掩盖事件白名单遗漏。

### 优化建议

fixture 清理应复用安全的 typed dispatch 入口，或显式销毁 Runtime；不要在测试中无说明地调用生产禁止的全量 update。

### 验收标准

- 测试目录中的 `dispatcher.update()` 全部被审计；
- 无说明的全量 update 为零；
- 清理语义（派发剩余事件或直接丢弃）有文档和测试。

### 推进记录（2026-08-25）

状态：**DONE**。

- 已审计 `tests/**` 中全部 `.update()`：生产事件相关调用仅保留显式 `update<T>()`；
- 已移除 `UiTestRuntime.hpp`、Switch/Radio、TabView、TaskChain、Tooltip fixture teardown 中 5 处无类型 `dispatcher.update()`；
- teardown 现在直接销毁 Scope/Runtime，明确采用“销毁 Runtime 并丢弃残留队列”语义，不在断言结束后执行任意业务回调；
- 加强 `TypedUpdateAllowsHandlerToCreateAnotherEventPool`，同时验证父事件和首次创建的新事件处理器均恰好执行一次；
- 工作区 `tests/**` 中无类型 dispatcher `update()` 搜索结果为零；
- 受影响的 4 组共 16 个测试全部通过，`ECS_FOCUSED_EXIT=0`；全量 Debug 构建通过，`ALL_BUILD_EXIT=0`。

---

## 5. P2：中期工程质量问题

## 5.0 当前状态总览（2026-08-27）

| 范围 | 状态 | 说明 |
|---|---|---|
| 阶段 A：运行时可信度 | DONE | P0-1、P0-2、P0-3、P1-9 已完成并有专项测试 |
| 阶段 B：帧管线与渲染失败语义 | DONE | P1-1、P1-4、P1-5、P2-3 已完成；真实 GPU 故障注入属于增强 |
| 阶段 C：隐式依赖与调度歧义 | DONE | P1-2、P1-3、P1-6、P1-7、P1-8 已完成 |
| P2-1～P2-3 | DONE | 当前内建 System、像素缓存基础契约、fallback 能力矩阵已完成 |
| P2-4：TODO 需求源治理 | DONE | 文档治理规则和本次状态整理已完成；后续仅维护新增规划 |
| 阶段 D：上层 UI 能力 | PARTIAL | Theme/Overlay/Focus 等已有实现；Rich Text、SVG、完整 Style/DSL 仍待产品规划 |

> 本表是当前状态入口。历史审查章节中的问题描述、风险和建议保留用于追溯，不应覆盖此处及各章节最后的推进记录。

## P2-1：SystemManager 默认 phase 会掩盖系统遗漏

当前系统装配依赖硬编码列表，部分 System 显式覆盖 phase，默认 LOGIC 会让新 System 忘记声明阶段仍能编译。

建议：

- 要求每个内建 System 显式声明 phase；
- 测试每个 System 恰好注册一次；
- 验证 phase 和注销顺序；
- 保持当前简单装配，不要立即引入插件容器。

### 推进记录（2026-08-26）

状态：**DONE（当前内建系统范围）**。

- Interaction、TextInput、HitTest、Tween、Layout、Render、State、Action、Timer、Theme、Shortcut、FocusNavigation、Overlay 均显式实现 `getPhase()`；
- `PlatformWindowSystem` 已移除，其窗口事件改由 InteractionSystem 在 UI 线程消费；
- `SystemManagerTest.BuiltInSystemsExposeExpectedPhaseContract` 验证 13 个内建系统的 phase 序列；
- 未引入插件容器；后续新增内建 System 必须同时提供显式 phase 和对应测试。

## P2-2：ImageManager CPU 像素缓存缺少边界

当前已有 `loadPixels()` 和同路径缓存测试，但没有明确容量、释放和统计契约。

建议先增加：

- `clearPixels`/`releaseAll`；
- 当前缓存数量和字节数统计；
- 显式生命周期测试。

在获得实际压力数据前，不建议直接引入复杂 LRU。

### 推进记录（2026-08-26）

状态：**DONE（基础生命周期与统计契约）**。

- 新增 `clearPixels()`，明确释放 CPU RGBA 像素缓存；
- 新增 `pixelCacheEntryCount()` 与 `pixelCacheByteSize()`，建立可观测的条目和字节数契约；
- `releaseAll()` 与析构同时释放 GPU 纹理和 CPU 像素缓存；
- `ImageManagerPixelsTest.LoadsAndCachesTemporaryBmpWithoutGpuDevice` 验证缓存命中、统计值和主动清理；
- 暂不引入未经压力数据证明的 LRU/容量淘汰策略。

## P2-3：Fallback 与 GPU 后端能力不等价

早期 CPU fallback 缺陷已审计并归档到 `docs/archive/todo-completed-2026-08-25/P2-3-cpu-render-capability-validation-2026-08-26.md`；fallback 不作为与 GPU 像素等价的后端。

建议：

- 建立 GPU/fallback 能力矩阵；
- 对不支持能力返回明确状态或降级结果；
- 共享几何/脏标记契约，避免两个后端各自定义成功语义；
- 为关键控件补同一输入、布局和截图回归。

### 推进记录（2026-08-25 至 2026-08-26）

状态：**DONE**。

- 在内部 `IBackendRenderer` 边界建立 `BackendType × BackendCapability` 三态矩阵，明确区分 `SUPPORTED`、`DEGRADED` 和 `UNSUPPORTED`；暂不暴露公共 API，避免在需求未确定前污染 `include/ui/**`；
- 首批能力覆盖纯色矩形、变换四边形、圆角矩形、边框、阴影、缓存位图、位图调色/UV 裁切、填充圆、圆形描边和 capsule；GPU 当前声明完整支持，fallback 按实际软件近似能力分级；
- 保留既有 `supports()` 布尔入口作为兼容适配：`DEGRADED` 仍表示功能可用，只有 `UNSUPPORTED` 返回 false；能力等级的唯一事实源是集中矩阵；
- 审计发现 `FallbackBackendRenderer::drawBatch()` 在纹理检查后存在无条件成功返回，使后续矩形、圆角、圆形和旋转四边形实现全部不可达；已删除该提前返回，并在实际绘制完成后返回 `Ok()`；
- 新增矩阵表驱动测试，覆盖 GPU 全能力支持和 fallback 三态分类；新增 offscreen software 像素读回测试，验证纯色矩形中心着色、圆角矩形中心着色且角点保持清屏色，防止“返回成功但没有绘制”再次发生；
- 新增内部 `backend_capability_level` CPO，使生产调用方可以查询三态等级；既有 `backend_supports` 从同一等级查询派生，避免布尔入口与矩阵漂移；
- 扩展 offscreen software 像素读回测试，验证缓存位图整体 Alpha 调制和 scissor 裁剪：裁剪外保持清屏色，裁剪内按半透明 Alpha 混合；
- `IBackendRenderer` 增加可覆盖的 `capabilityStatus()`，保证后端经接口类型擦除后仍可注入三态能力；`supports()` 和 CPO 均从该入口派生；
- 新增 Runtime-local、per-window 的 `WindowCapabilityDiagnostics`：按 capability + status 去重，能力缺失只记录一次，不将永久不支持误报为帧失败，也不因 dirty 重试逐帧刷日志；
- Image fallback 已区分基础位图、RGB tint 和 UV crop：基础能力不支持时明确诊断并跳过；RGB tint/UV crop 不支持时明确记录“忽略并绘制原始完整位图”，实际解码或 SDL 上传失败仍通过 `Result` 传播；
- Text fallback 在基础位图能力不支持时明确诊断并跳过；Shape fallback 对圆角记录软件近似降级，对阴影/边框记录不支持并仅省略子效果，保留背景提交；
- Canvas filled circle/circle outline 已查询并报告 `DEGRADED`；LINE/POLYLINE/CUBIC_BEZIER 使用的 capsule 在 fallback 不支持时改为普通 transformed quad，省略圆头但保留线条，不再由 backend 整批跳过；
- 新增 Canvas 结构测试，验证 capsule 降级后生成非轴对齐四边形且 `draw_mode=0`，并验证 filled circle 保留批次和降级诊断；矩阵/CPO/诊断/Canvas 专项测试 6/6 通过，既有像素与生命周期测试保持通过；
- 新增 Shape 调用侧结构测试：fallback 下圆角报告 `DEGRADED`，阴影和边框报告 `UNSUPPORTED`，仅省略不支持子效果；基础背景批次仍生成且阴影参数保持清零；
- 新增 Image 调用侧结构测试：当 `CACHED_BITMAP` 由测试 backend 经类型擦除注入为 `UNSUPPORTED` 时，产生明确诊断、跳过图片且 `collectChecked()` 保持成功，避免永久能力缺失阻断窗口提交；
- Text fallback 的 `CACHED_BITMAP` 能力判断已前移到字体栅格化之前，基础位图能力不支持时避免执行无意义的 CPU 字体工作；
- 新增 Text 调用侧结构测试，验证 `CACHED_BITMAP=UNSUPPORTED` 时不触发位图绘制、产生一次明确诊断且 `collectChecked()` 成功；
- 收紧 Text 渲染上下文契约：`BatchManager` 与 `FontManager` 是 `collectChecked()` 的必需依赖，缺失时返回明确的 `DEVICE_UNAVAILABLE`，不再静默跳过；生产帧中二者由 `RenderSystemImpl` 独占，在同步 collect 完成前地址和生命周期稳定，并在 root context 构造边界设置断言；fallback backend 仍保持条件性非空，因为 GPU 路径用空指针表示非 fallback；
- 新增 Text 必需依赖契约测试，分别覆盖缺失 BatchManager 和 FontManager；全量 unit 标签测试 106/106 通过；
- 新增 Image 实际失败传播测试：使用有效临时 BMP 到达 `drawCachedBitmap()`，由测试 backend 注入 `ASSET_UPLOAD_FAILED`，断言 `collectChecked()` 保留同一错误而非转为降级成功；
- 能力矩阵、CPO、诊断去重及 Canvas/Shape/Image/Text 结构测试 10/10 通过；software 像素和多窗口生命周期集成测试 3/3 通过；
- 新增 `BackendRenderContractTest` 同输入结构门禁：同一 Shape 控件分别按 GPU 与 fallback 能力收集，强制位置、尺寸、颜色、Alpha 和基础四边形一致；允许 fallback 省略阴影/边框，但必须产生三态诊断；该门禁不创建真实 GPU，适合无 GPU CI 必跑；
- 早期 `CPU_RENDER_ISSUES.md` 已从活动 TODO 移除并重写为归档验证记录，失效行号和历史实现片段不再作为当前事实源；
- 2026-08-26 全量 unit 标签测试 106/106 通过；本轮最终 `all` 重跑因另一个并发 CMake/Ninja 进程锁定 `build.ninja`，在 Generate recompaction 阶段报 `Permission denied`，属于构建目录并发锁冲突；此前本变更相关目标和全量 unit 均已成功，最近一次无冲突全量 Debug 结果仍为 `ALL_BUILD_EXIT=0`；
- 全量 Debug 构建通过，`ALL_BUILD_EXIT=0`，公共头和架构边界门禁通过。

### 历史剩余工期评估（2026-08-25，已失效）

> 本表是初始审查时的估算，不代表当前剩余工作量。当前待办以 `docs/todo/README.md` 和 `docs/todo/TODO_ROADMAP_SUMMARY_2026-08-27.md` 为准。

以下为单人连续开发、现有 Debug 构建环境稳定且不包含新增产品需求的历史估算：

| 收敛范围 | 剩余工作 | 预计时间 |
|---|---|---:|
| 关闭 P2-3 | 已完成：三态契约、关键控件同输入结构门禁、software 像素验证和缺陷文档归档 | DONE |
| 完成阶段 B | P2-3 已完成；真实 GPU 离屏读回和更完整控件截图作为可选增强 | 1–3 个工作日 |
| 完成阶段 C 核心项 | P1-2 ambient Runtime 渐进迁移、P1-3 Scope 生命周期、P1-6 fps=0、P1-7 SDL 唤醒；P1-8 已完成 | 6–10 个工作日 |
| 完成本文 A–C 核心优化 | 含专项测试、全量构建、文档收尾及约 20% 缓冲 | 8–14 个工作日 |
| 连同 P2 工程治理 | 再含 System phase、Image 像素缓存边界和 TODO 归档治理 | 11–18 个工作日 |

最大不确定项是 P1-7：SDL 事件唤醒需要在“主循环阻塞等待”和“SDL event watch 只发 wake signal”之间完成实现与跨平台验证；若发现平台驱动行为差异，可能额外增加 2–4 个工作日。上层 Theme/RichText/SVG/新控件不计入上述“核心优化完成”。

## P2-4：TODO 管理没有需求源治理

当前 TODO 文件混合了完成记录、问题日志、旧能力盘点和未来计划，优先级与状态不统一。

建议建立轻量规则：

1. `docs/todo/` 只放未完成规划；
2. 已完成文件移动到 `docs/test-reports/` 或 `docs/archive/`；
3. 每个规划必须包含状态、范围、依赖、验收标准、最后复核日期；
4. 每个 P0/P1 只能有一个主文档，避免重复计划；
5. 过时计划标记 `SUPERSEDED`，不能静默保留。

---

## 6. 分阶段实施路线

### 阶段 A：运行时可信度（已完成）

历史实施顺序：

1. P0-1 EventLoop 计数/排空协议；
2. P0-2 EventConnection Runtime 归属；
3. P0-3 回调快照和重入契约；
4. P1-9 审计测试 fixture 的全量 update。

阶段门槛：并发、跨 Runtime、回调重入和事件清理测试全部通过；没有新增无说明的 ambient 依赖。

### 阶段 B：帧管线和渲染失败语义（已完成）

历史实施顺序：

1. P1-1 dirty 按窗口和提交结果清理；
2. P1-4 buffered event 唯一目录和登记门禁；
3. P1-5 per-window render state；
4. P2-3 GPU/fallback 能力矩阵。

阶段门槛：事件不丢失、失败可重试、多窗口隔离和后端降级行为可测试。

### 阶段 C：隐式依赖和调度歧义（已完成）

历史实施顺序：

1. P1-2 渐进式减少 `UiRuntime::current()`；
2. P1-3 明确 Scope 生命周期；
3. P1-6 统一 `fps=0` 语义；
4. P1-7 设计 EVENT_DRIVEN 的 SDL 唤醒路径；
5. P1-8 统一 CMake 静态/动态支持矩阵。

阶段门槛：依赖能从构造函数或 API 参数看出，调度模式和构建模式没有互相矛盾的文档/实现。

### 阶段 D：上层 UI 能力（部分完成，后续产品线）

以下能力中部分已在阶段 A-C 期间提前实现；本节不再作为“必须等待后才能开始”的路线门禁：

- Theme/Style；
- Overlay/Popup；
- Focus/Keyboard Navigation；
- List/Tab/Menu/Tooltip/Modal 等控件；
- Rich Text、SVG 和外部 DSL。

这些能力应复用已经稳定的事件、浮层、焦点、渲染失败和多窗口契约，不应各自重新实现生命周期管理。具体剩余项见 TODO 总结文档。

---

## 7. TODO 文档整理建议

### 7.1 已完成事项

以下类型文件不应继续作为待办：

- `todo/P2-1-listview-build-verify.md`
- `todo/P2-2-contextmenu-modal-verify.md`
- `todo/P2-4-animation-lifecycle-build-loop.md`
- `todo/test-listview-loop.md`

建议保留内容但移动到验证归档，或统一增加：

```text
状态：DONE
完成日期：YYYY-MM-DD
验证报告：...
后续遗留项：无/链接到新规划
```

### 7.2 规划与验证分离

规划文件只描述：目标、范围、设计、风险、依赖、验收标准。

测试报告只描述：环境、命令、结果、失败、修复和复验。

不要让“125/125 通过”这种历史结果充当当前架构状态证明；每次实施前都应重新验证。

### 7.3 优先级重排

建议暂时将已有上层能力规划降级，新增以下核心规划作为主线：

- `P0-EVENTLOOP-PUBLISH-DRAIN`
- `P0-EVENT-CONNECTION-OWNERSHIP`
- `P0-EVENT-CALLBACK-SNAPSHOT`
- `P1-RENDER-DIRTY-RETRY`
- `P1-BUFFERED-EVENT-REGISTRY`
- `P1-PER-WINDOW-RENDER-STATE`

---

## 8. 明确不建议现在做的事

1. 不建议为了本次事件问题直接升级 EnTT 大版本；先保持项目侧 typed dispatch 契约稳定。
2. 不建议立即引入完整 Render Graph；当前首要问题是窗口状态和 dirty 语义，不是渲染拓扑表达能力。
3. 不建议为去除所有 `UiRuntime::current()` 一次性重写全仓；先按公共 API、System、manager 分层迁移。
4. 不建议立即引入通用事件框架替换现有 bridge；先定义 Runtime 归属和回调快照语义。
5. 不建议继续增加大量控件而不补 Theme、Overlay、Focus 等根能力。
6. 不建议把随机压力测试当作并发正确性证明；必须加入可控交错测试。

---

## 9. 首个实施工作包：P0 运行时契约

### 目标

在不改变公共控件 API 的前提下，修复最危险的三个运行时契约：

- EventLoop 生产/消费计数；
- EventConnection Runtime 归属；
- 事件回调派发快照。

### 变更范围

- `src/utils/EventLoop.hpp`
- `src/utils/MpscQueue.hpp`（如确需补充发布/排空状态）
- `include/ui/api/Event.hpp`
- `src/api/Event.cpp`
- `src/helper/Helper.hpp`
- `tests/unittest/`
- `tests/support/`

### 完成定义

- 有可控竞态回归测试；
- 有两个 Runtime 的连接隔离测试；
- 有回调新增、断开、自断开、嵌套触发测试；
- 全量 Debug 构建通过；
- 相关测试重复运行结果稳定；
- 新增设计约束写入源码注释和对应 TODO，不只写在测试名称里。

### 推进记录（2026-08-25）

状态：**DONE**。

已完成并通过专项验证：

- `MpscQueue` 在 release 发布槽位前执行任务计数回调，消费者不再看到尚未计数的任务；
- `EventLoop` 将已进入 `Post()` 的生产者纳入 drain 停止条件，最后一个生产者在退出阶段唤醒消费者；
- `EventConnection` 保存所属 EventDomain 的弱引用，不再在析构/断开时依赖当前 TLS Runtime；
- 回调派发采用槽位快照，新连接从下一轮开始生效，已在本轮断开的待执行回调会被跳过；
- 回调执行后重新查找事件槽容器，避免注册新事件导致 `unordered_map` rehash 后使用失效迭代器；
- 增加可控交错测试，验证槽位在外部记账完成前不可见，以及 drain 会等待已进入 `Post()` 的生产者；
- 增加跨 Runtime 相同 token 隔离、移动所有权、幂等断开、自断开和嵌套触发测试；
- `ui_api_tests`：23/23 通过；`ui_eventloop_stress_tests`：7/7 通过，连续两轮退出码均为 0；
- 全量 Debug `all` 构建通过，`ALL_BUILD_EXIT=0`；公共头自包含门禁与架构边界门禁均通过。

后续增强项（不阻塞本工作包关闭）：

- 若未来需要验证 `TryEmplaceBeforePublish` 回调自身的异常路径，应维持其 `noexcept` 约束，不允许在槽位保留期间传播异常；
- 当前事件域仍是单线程使用契约，若开放跨线程公共事件注册/派发，需要另立同步协议工作包。

