# WP4 下一最小批次：Offscreen + Fallback 100 次窗口生命周期

- 日期：2026-07-17
- 输入来源：用户要求只读分析 WP4 当前 65% 后的下一最小可实施批次
- 作用范围：测试基础设施、`Application`/`SystemManager`/`RenderSystem`/`StateSystem` 窗口生命周期集成路径；不修改公开 API
- 当前基线：Application systems-before-SDL、首窗 device/backend 锁定、ImageManager RAII、固定三节点 GPU 初始化事务与 shader `Result` 已完成；165 tests 全绿

## 1. 影响摘要

### 推荐结论

下一批优先做 **“SDL offscreen 视频驱动 + 强制 CPU fallback 的 100 次顺序窗口创建/关闭集成测试”**。

这是三个剩余项中唯一能在不引入 shared device lease、完整 GPU mock、公开 API 改动或 WP7 资源上下文重构的前提下低风险落地的项目。它会真实初始化 SDL video、真实创建隐藏的 offscreen `SDL_Window`，并真实创建/销毁 SDL software renderer；**不会创建 `SDL_GPUDevice`，不会执行 GPU claim、shader、pipeline、command buffer 或 GPU submit**。因此它是窗口与 fallback 生命周期的确定性基线，不能冒充“真实 GPU 100 次生命周期”验收。

### 三项剩余工作的可实施性判断

| 候选项 | 当前可低风险推进 | 判断 |
|---|---|---|
| 真实 GPU 故障注入 | 否 | `DeviceManager` 直接调用 `SDL_CreateGPUDeviceWithProperties`、`SDL_ClaimWindowForGPUDevice`，没有内部 seam；环境变量或错误 backend 只能稳定覆盖“所有设备创建失败并切 fallback”，不能精确注入 shader/device/claim 各阶段失败。强做会引入函数表、完整 mock 或不稳定的驱动依赖。 |
| device 外部先行失效类型保护 | 否 | GPU owner deleter 捕获裸 `SDL_GPUDevice*`。类型级保护必然需要 lifetime token/shared state、lease 或 wrapper 所有权模型调整，正是本轮禁止范围；局部空指针判断无法证明外部 device 仍有效。 |
| 100 次窗口生命周期 | 是，限定 fallback/offscreen | 现有 `Application`、公开 `CreateWindow(UiRuntime&, ...)`、`CloseWindow()`、内部 dispatcher update 已能贯通真实创建与关闭路径；SDL 构建已启用 `SDL_DUMMYVIDEO=ON`、`SDL_OFFSCREEN=ON`，且 offscreen 源已进入当前构建。 |

## 2. 现有能力核验

### 2.1 测试支持

- `tests/unittest/CMakeLists.txt` 已提供统一测试 target 配置并链接 `ui`、SDL3、EnTT、GoogleTest。
- 现有 `test_ApplicationLifecycle.cpp` 只验证纯协调器顺序，不调用 `SDL_Init()`。
- 现有 `test_GPUTextureOwner.cpp` 与 `test_GpuInitializationTransaction.cpp` 都是 sentinel/fake callback 纯逻辑测试，不覆盖真实窗口。
- 当前没有项目自有测试调用 `SDL_CreateWindow()`、`SDL_DestroyWindow()` 或验证 100 次窗口循环。
- 为避免 SDL 全局状态、环境变量和 `AppConfig` 单例污染现有 165 tests，生命周期测试应使用**独立测试进程/独立 executable**，不并入 `ui_unit_tests` 进程。

### 2.2 SDL dummy/offscreen/fallback

- 当前 `build/CMakeCache.txt` 明确为：`SDL_DUMMYVIDEO=ON`、`SDL_OFFSCREEN=ON`、`SDL_VULKAN=ON`。
- 当前生成的 SDL build config 定义了 `SDL_VIDEO_DRIVER_DUMMY` 与 `SDL_VIDEO_DRIVER_OFFSCREEN`；offscreen 实现源已编入 SDL target。
- `offscreen` 比 `dummy` 更适合作为首选：它具有真实 offscreen window 实现并保留 framebuffer/EGL/Vulkan 支持；本批强制 fallback 后只依赖 SDL software renderer，不依赖 GPU surface 能力。
- `dummy` 可作为环境兼容性的手工备选，但不应在同一测试内自动从 offscreen 回退，否则会掩盖 CI 配置漂移。
- `FallbackBackendRenderer` 会依次尝试 direct3d11/opengl/opengles2/software；在 offscreen 测试中最终应允许 software 成功。测试必须核验 backend 初始化成功，而不是只核验窗口句柄。

### 2.3 可测试性

- `Application` 可由测试进程使用 `--backend=cpu` 构造；其构造会真实执行 `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)`，析构通过 `ApplicationLifecycle` 保证 `SystemManager` 在 `SDL_Quit()` 前销毁。
- `CreateWindow(UiRuntime&, ...)` 可创建真实隐藏窗口并同步触发 `WindowGraphicsContextSetEvent`，从而进入 `RenderSystem` fallback 初始化。
- `CloseWindow()` 将 `CloseWindow` 事件 enqueue；测试可显式调用 runtime dispatcher 的定向 update，使 `StateSystem::destroyWidget()` 同步触发 `WindowGraphicsContextUnsetEvent` 并销毁 SDL window。
- `RenderSystem` 没有公开诊断接口可直接断言 backend 类型或 renderer 计数；本批不应为测试增加公开 API。成功创建窗口、关闭后 ECS entity 失效、window ID 查找为空、循环无崩溃，以及进程退出无 SDL 错误，是当前可观察面。
- `DeviceManager` 不适合本批直接测试：构造虽可用，但 `initialize()` 依赖 active `UiRuntimeScope` 和真实 SDL GPU；内部 backend 列表及创建函数均不可注入。

## 3. 具体最小工作包

### 工作包名称

**WP4-B5：Offscreen Fallback 100 次窗口生命周期基线**

### 目标

在一个隔离测试进程内，真实初始化 SDL offscreen video 与 fallback software renderer，顺序执行 100 次“创建隐藏窗口 → 同步关闭 → 验证窗口和实体均已释放”，最后析构 Application，并验证无崩溃、无悬挂窗口和无重复销毁。

### 文件范围

| 优先级 | 文件 | 计划 |
|---|---|---|
| P0 | `tests/unittest/test_FallbackWindowLifecycle.cpp`（新增） | 单一集成测试；进程启动后、Application 构造前固定 video driver 为 `offscreen`，传入 `--backend=cpu`；循环 100 次。 |
| P0 | `tests/unittest/CMakeLists.txt` | 新增独立 `ui_lifecycle_tests` executable，复用 `configure_ui_test_target()`，单独 `gtest_discover_tests` 并标记 `lifecycle`/`sdl` 标签；不加入 `ui_unit_tests` 源列表。 |
| P1，仅测试暴露真实缺陷时 | `src/systems/render/RenderBackend.cpp` | 只允许修复 fallback window-unset 的内部清理/解绑时机；不得新增公开接口或 GPU 所有权抽象。 |

默认先只改前两个测试文件。生产文件不是预设必改项；测试失败后必须先确认是 retained fallback renderer、SDL driver 不可用还是测试关闭事件未派发，再决定是否进入 P1。

### 测试步骤与断言

1. 在任何 SDL video 初始化前请求 `offscreen` driver；构造 Application 后断言 `SDL_GetCurrentVideoDriver()` 为 `offscreen`。不允许静默改用 `windows` 或 `dummy`。
2. 通过独立 argv 传入 `--backend=cpu`，确保不调用 `DeviceManager::initialize()`。
3. 对每次迭代：
   - 调用 runtime-bound `CreateWindow()`；断言结果成功、entity 有效、window ID 非零且 `SDL_GetWindowFromID(id)` 非空。
   - 调用现有 `CloseWindow(entity)`，随后定向派发 `events::CloseWindow`。
   - 断言 entity 已失效、`SDL_GetWindowFromID(id)` 为空。
4. 共执行 100 次，采用顺序单窗口模型；不并发、不运行 `Application::exec()`、不提交渲染帧。
5. Application 离开作用域后断言 SDL video 已退出；测试进程正常结束。
6. 定向运行 `ui_lifecycle_tests`，随后运行原有 165 tests，确认新集成测试不污染原进程基线。

### 是否实际初始化 SDL/GPU

| 能力 | 本批行为 |
|---|---|
| SDL video/events | **是**，由 `Application` 真实初始化和退出 |
| SDL offscreen window | **是**，100 次真实创建/销毁 |
| SDL_Renderer fallback | **是**，真实创建/销毁，预期 software driver |
| `SDL_GPUDevice` | **否** |
| GPU window claim | **否** |
| shader/pipeline/command buffer | **否** |
| GPU frame acquire/submit/present | **否** |

因此本批完成后，路线图只能记录为“100 次 fallback/offscreen 窗口生命周期基线完成”；WP4 的“真实 GPU 100 次生命周期”仍未验收。

## 4. 修改类型与设计约束

- 改动类型：测试能力扩展；若触发生产修复，则为 `RenderBackend` 局部生命周期修复。
- 无公开 API 变化，无数据格式变化，无 CMake 拓扑重构，仅新增一个测试 executable。
- YAGNI：不引入 GPU mock、SDL 函数表、设备工厂接口或资源上下文。
- DRY：复用现有测试 target 配置，不复制编译/链接选项。
- SOLID：不新增生产抽象；独立测试 executable 只负责隔离 SDL/进程全局状态，符合 SRP。没有为 DIP 人为制造接口。
- 新测试 executable 的引入理由：SDL driver、SDL 全局初始化和 `AppConfig` 都是进程状态，独立进程能避免污染 165 个纯/半纯测试。不独立的代价是测试顺序敏感和并行执行不稳定。

## 5. 风险与控制

| 风险 | 等级 | 控制 |
|---|---|---|
| `WindowGraphicsContextUnsetEvent` 在 fallback 分支直接返回，renderer 可能保留已销毁 window 的关联状态 | 中 | 这是本批最可能发现的真实缺陷；若复现，仅允许在 `RenderBackend.cpp` 内做最小解绑/cleanup 修复。 |
| SDL 销毁 window 时可能连带销毁 renderer，而 fallback owner 后续再次 cleanup | 中高 | 以独立进程循环尽早暴露崩溃/双释放；不要用 mock 掩盖。若 SDL 契约确认自动销毁，则停止并先修 owner 状态同步。 |
| offscreen + software renderer 在特定 SDL 构建不可用 | 低（当前 Windows 构建已启用） | driver 不匹配或 renderer 创建失败应明确失败，不自动 fallback 到 windows/dummy。 |
| `AppConfig` 单例保留 `cpu` 配置 | 低 | 独立 executable/进程隔离；不并入现有 unit test 进程。 |
| 测试只创建/关闭，不执行 frame submit | 中（残留） | 明确不计为 GPU 生命周期验收；真实 GPU smoke 留待具备可控环境后单列。 |
| 日志量较大导致测试慢 | 低 | 先以 100 次串行作为验收，不扩到并发或长时 soak。 |

## 6. 停止条件

出现以下任一情况，停止扩大本批并回报，不通过架构扩张强行解决：

1. offscreen driver 无法在当前构建被选择，且修复需要修改第三方 SDL 或全局 SDL 构建选项；
2. fallback renderer 无法在 offscreen 下初始化，且只能通过 GPU backend 或平台真实窗口才能运行；
3. 修复要求 shared device lease、lifetime token、GPU wrapper 改签或 `RenderResourceContext`；
4. 需要公开 `RenderSystem` 内部状态才能通过测试；
5. 关闭一个窗口会要求 WP5 帧管线或 WP7 全面拆分才能确定性派发；
6. 测试暴露 SDL 对 renderer/window 销毁契约与当前 owner 模型冲突，且无法在 `RenderBackend.cpp` 单文件内安全修正；此时单独建立 fallback owner 修复工作包。

## 7. 验证建议

- 构建：Debug 下构建 `ui_lifecycle_tests`，再构建原有测试 targets。
- 定向：独立运行生命周期测试，期望 1/1 通过，内部循环 100/100。
- 回归：原 165 tests 仍为 165/165；新测试单独计数，不把总数误报为仍然 165。
- 稳定性：定向测试连续执行 3 次，用于排除 SDL 全局状态和窗口 ID 复用导致的偶发问题；不把 3×100 宣称为单次 300 窗口并发验收。
- 观测：记录实际 video driver、fallback renderer driver、每轮失败索引、SDL error；不要求新增生产诊断 API。

## 8. 后续门槛

本批通过后，下一步仍不应直接做“外部 device 先行失效类型保护”。应先二选一：

1. 在有稳定 GPU runner 的环境新增 **真实 GPU smoke**：真实平台窗口、固定 backend、少量（如 3～10 次）初始化/claim/资源初始化/shutdown；它是环境验收，不做精确故障注入。
2. 若必须精确覆盖设备创建/claim 故障，再单独批准一个仅位于 `DeviceManager` 内部的窄 SDL call seam；在未批准前不引入完整 GPU mock。

## 9. 待确认问题

- 默认假设：本轮接受先完成“fallback/offscreen 100 次基线”，并明确不将其记作真实 GPU 生命周期完成。
- 默认假设：CI/开发机允许独立 SDL 测试进程串行运行；若测试框架全局并行，需要为 `ui_lifecycle_tests` 设置串行属性。
