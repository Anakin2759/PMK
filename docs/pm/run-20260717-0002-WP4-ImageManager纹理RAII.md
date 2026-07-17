# 项目经理协调记录 - WP4 ImageManager 纹理 RAII

- 时间：2026-07-17
- 输入来源：用户指定继续 WP4；承接 `docs/pm/run-20260717-0001-WP4-最小设备锁定批次.md`
- 本轮范围：实施与验证编排；本记录仅定义工作包和验收标准，不修改源码、不执行或声称构建
- 验收标准：ImageManager 独占持有缓存纹理；私有上传链全程传递 owner；释放只依赖 owner 析构；RenderSystem 在 device 前统一释放资源并复位 claim 状态；测试不启动 SDL/GPU

## 工作包
| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | ImageManager 最小纹理 RAII 与 cleanup 收敛 | 代码工厂 | 本记录“工作包 #1” | 源码变更、fake-deleter 测试、交付报告 | 完成 |
| 2 | 定向构建与测试闭环 | 测试构建闭环 | 工作包 #1 交付报告 | 验证报告、问题日志 | 完成 |

## 工作包 #1：实施规格

- **目标**：将 `ImageManager` 的 GPU 纹理缓存改为现有 `wrappers::UniqueGPUTexture` 所有权模型，并最小收敛 `RenderSystem::cleanup()` 的销毁顺序与状态复位。
- **依据**：当前 `ImageManager::m_cache` 持有裸纹理指针，私有加载/上传链返回裸指针，`releaseAll()` 再查询当前 device 并在 device 消失时主动放弃释放；当前 `RenderSystem::cleanup()` 存在按 device 是否为空分叉的资源清理路径，且未统一复位 `DeviceClaimState`。
- **只允许触达**：
  - `src/managers/ImageManager.hpp`
  - `src/managers/ImageManager.cpp`
  - `src/systems/render/RenderBackend.cpp`
  - 新增一个纯 owner 测试文件（建议 `tests/unittest/test_GPUTextureOwner.cpp`）
  - `tests/unittest/CMakeLists.txt`，仅用于登记新增测试源
- **禁止触达**：
  - 公开 `include/` API
  - `ImageRenderer`、ECS、其他 manager
  - `src/common/GPUWrappers.hpp`（复用现有 wrapper，不扩张抽象）
  - 其他 RenderSystem 文件、构建 target/依赖/编译选项
  - shared device lease、完整 SDL/GPU mock、新依赖、无关重命名或格式化、Git 操作
- **实现约束**：
  1. `m_cache` 的 value 改为现有 `wrappers::UniqueGPUTexture`；公开 `loadTexture()` 签名及借用裸指针语义保持不变。
  2. `loadWithStb()`、`loadWithSdlBmp()`、`uploadToGpu()` 等私有上传链全程返回并移动 owner；不得在中间重新退化为需人工释放的纹理裸所有权。
  3. 缓存命中和插入后仅通过 `.get()` 向公开 API 返回非拥有指针；失败路径由局部 owner 自动回收。
  4. `releaseAll()` 仅执行 `m_cache.clear()`；不得查询 `DeviceManager`/当前 device，不得直接调用 `SDL_ReleaseGPUTexture`，不得记录或实施主动泄漏。
  5. `RenderSystem::cleanup()` 必须在 `DeviceManager::cleanup()` 前统一销毁所有 device-bound 资源，包括 ImageManager 缓存；device 为空的轻量路径也不得绕过统一资源 reset。
  6. cleanup 结束后调用 `DeviceClaimState::Reset()`；重复 cleanup 保持幂等。不得引入 device 解锁、热切换或 shared lease 新语义。
  7. 新测试仅验证 owner 的移动、reset/clear、恰好一次释放及空 owner 行为；使用纯 fake deleter/计数器，不创建窗口、device、texture，不调用 SDL 初始化或 GPU API。
- **验收条件**：
  - `ImageManager` 中不存在缓存纹理的裸拥有权容器，也不存在纹理失败路径的手工 `SDL_ReleaseGPUTexture`。
  - 上传任一步骤失败时，已创建纹理由 owner 恰好释放一次；成功插入缓存后 owner 唯一归属缓存。
  - 重复加载命中同一路径时返回同一借用指针，不产生第二 owner。
  - `releaseAll()` 的实现不访问 `m_deviceManager`，清空后缓存为空，重复调用安全。
  - `RenderSystem::cleanup()` 的所有退出路径均满足“资源先于 device 清理”，并最终复位 `m_deviceClaimState`；重复调用安全。
  - fake-deleter 测试覆盖：move 后源为空、目标析构释放一次、`reset()` 释放一次、容器 `clear()` 每个 owner 各释放一次、空 owner 不释放；测试过程零 SDL/GPU 启动与 API 调用。
  - 交付报告列出全部变更文件，并逐条映射上述验收条件；若无改动或受阻，必须明确原因。
- **失败升级条件**：若必须改变公开 API、修改 `GPUWrappers.hpp`、触达 ImageRenderer/ECS/其他 manager、引入 shared device lease/完整 GPU mock，或无法在 device 销毁前保证 owner deleter 所需 device 有效，则停止扩展并回报用户。

## 工作包 #2：验证规格

- **目标**：验证 RAII 所有权与 cleanup 编译集成，不启动真实 SDL/GPU。
- **target/tests**：Debug；构建 `ui`、`ui_unit_tests`；运行新增 fake-deleter owner 测试及全量 `ui_unit_tests`。
- **报告期望**：返回报告路径、configure/build/test 各阶段结果、问题日志新增条数；未执行不得写为通过。
- **失败策略**：仅可修复本批测试登记或测试笔误；生产代码/API/生命周期失败退回工作包 #1；同阶段连续两次失败后升级用户。
- **本批不验收**：真实 GPU 资源释放、SDL/GPU 初始化、ImageRenderer/ECS 行为、多 device/热切换、shared device lease、压力测试。

## 调度时间线
- 2026-07-17：完成只读范围核验，形成最小实施与验证工作包；未修改源码，未执行构建。
- 2026-07-17：`ImageManager` 缓存及上传链改用 `UniqueGPUTexture`/`UniqueGPUTransferBuffer`；`releaseAll()` 收敛为 owner 容器 clear。
- 2026-07-17：RenderSystem cleanup 统一在 device 前 reset 全部资源并复位 claim 状态；新增纯 fake-deleter owner 测试。
- 2026-07-17：Debug 构建成功；架构门禁通过；定向测试 12/12、全量测试 157/157。架构指标为 302/2/3/0；未运行真实 GPU 验收。

## 待用户决策
- [ ] 无；仅触发升级条件时请求范围或生命周期语义决策。

## 结论
- 状态：完成
- 关键产物：`ImageManager` 纹理 RAII、统一 cleanup、`test_GPUTextureOwner.cpp` 及验证记录
- 变更文件：`src/managers/ImageManager.hpp/.cpp`、`src/systems/render/RenderBackend.cpp`、`tests/unittest/test_GPUTextureOwner.cpp`、`tests/unittest/CMakeLists.txt`；同步收紧 `tools/check_architecture_boundaries.py` 中下降后的文件级债务基线
- 验收映射：缓存无裸 owner；私有上传链全程移动 owner；公开接口仅返回 `.get()`；`releaseAll()` 不访问 device；cleanup 资源先于 device 且最终 reset claim；fake deleter 验证 move/clear/空 owner 和恰好一次释放
- 未验收：真实 GPU 初始化/释放、资源代际 token、device 外部先行失效和 100 次窗口生命周期压力测试
- 下一工作包：继续 WP4 初始化中途失败与重复 cleanup 的可测试资源 DAG，不扩大到 shared device lease
