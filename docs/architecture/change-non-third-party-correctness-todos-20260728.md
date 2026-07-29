# 非第三方 TODO 正确性事项修改规划

- **日期**：2026-07-28
- **输入来源**：
  - `docs/todo/NON_THIRD_PARTY_TODO_MARKERS_2026-07-28.md`
  - `docs/pm/run-20260728-0000-非第三方TODO正确性闭环.md`
- **作用范围**：`TextureAtlas` 位图上传与扩容、`DeviceManager` 白色纹理所有权，以及直接渲染调用点和既有单元测试目标。
- **改动类型总判定**：内部接口扩展与 GPU 资源生命周期修正；不改变公共协议、安装接口或构建拓扑。

## 1. 结论与影响摘要

推荐按 **C1 位图真实上传 → C2 白色纹理归属迁移 → C3 图集事务式扩容** 的顺序实施。C3 复用 C1 已确认的命令缓冲、CopyPass、提交失败处理和资源释放规则；C2 独立，但应在 C3 前完成，以先统一设备资源的所有权边界。

| 编号 | 正确性事项 | 改动类型 | 主要模块 | 优先级 | 前置依赖 |
|---|---|---|---|---|---|
| C1 | `TextureAtlas::uploadBitmap()` SDL3 GPU 真实上传 | 局部正确性修复 | `src/managers/TextureAtlas.hpp` | P0 | 无 |
| C2 | `DeviceManager` 创建、持有、访问和销毁白色纹理 | 内部接口扩展 + 所有权迁移 | `DeviceManager`、`RenderSystem` GPU 路径 | P0 | 无；建议在 C3 前完成 |
| C3 | `TextureAtlas::expand()` 迁移旧内容并保留状态 | 状态保持型内部重构 | `src/managers/TextureAtlas.hpp` | P1 | C1 的 GPU 提交/失败处理模式 |

本规划不包含 `BatchManager::optimize()`。

## 2. 现状事实

### 2.1 `TextureAtlas`

1. `TextureAtlas::addGlyph()` 先通过 shelf 分配位置，再调用 `uploadBitmap()`；只有上传返回成功才写入 `m_glyphMap`。
2. `uploadBitmap()` 当前只校验空指针和尺寸，随后记录警告并返回 `true`，未创建 TransferBuffer、CommandBuffer 或 CopyPass，因而会把“未上传”误报为成功。
3. 图集纹理格式是 `SDL_GPU_TEXTUREFORMAT_R8_UNORM`，usage 是 `SDL_GPU_TEXTUREUSAGE_SAMPLER`；位图来源是单通道灰度数据，与目标格式一致，无需像素格式转换。
4. `expand()` 当前创建两倍尺寸的新纹理后释放旧纹理，并清空 `m_glyphMap`、`m_shelves`、`m_currentShelfY`。这会使既有字形缓存和布局状态失效。
5. 若扩容后保留像素坐标，所有既有 `AtlasGlyph` 的归一化 UV 仍必须按新尺寸重算；只复制纹理而不更新 UV 仍是错误状态。
6. `m_texture` 类型是带设备 deleter 的 `UniqueGPUTexture`，但当前成员以默认 deleter 构造，同时析构和扩容中手工调用 `SDL_ReleaseGPUTexture()`。这绕过既有 RAII 约定，且不利于事务式替换；应在本次相关修改中一并纠正，而不是扩散手工释放。
7. 直接调用链为 `FontAtlasManager::{getOrAddGlyph,getOrAddGlyphByIndex}` → `TextureAtlas::addGlyph()`；纹理由 `FontAtlasManager::getAtlasTexture()` 暴露给文本辅助路径。仓库中没有现成的 `TextureAtlas` 单元测试。

### 2.2 `DeviceManager` 与白色纹理

1. `DeviceManager::getWhiteTexture()` 当前固定返回 `nullptr`。
2. GPU 渲染路径已有另一套白色纹理实现：`RenderSystem::createWhiteTexture()` 创建并上传 `1×1`、`R8G8B8A8_UNORM` 的白像素，所有权位于 `RenderSystemImpl::m_whiteTexture`。
3. `RenderFrame.cpp` 每帧在资源就绪后检查/创建该纹理，并把裸指针写入 `RenderContext::whiteTexture`；Shape、Slider、ProgressBar 等渲染器依赖该指针。
4. `DeviceManager::claimWindow()` 在首个窗口声明失败时可能销毁当前候选设备并切换后端。因此任何绑定设备的白色纹理都必须在 `DeviceManager::cleanup()` 中先于设备释放，并在每次新设备创建时重新建立。
5. `DeviceManager::cleanup()` 当前先 `SDL_WaitForGPUIdle()`、释放窗口声明、再销毁设备；尚无白色纹理成员。
6. `GPUWrappers.hpp` 已提供 `UniqueGPUTexture` 和 `UniqueGPUTransferBuffer`，无需新增依赖或另造资源包装。

### 2.3 已有 SDL3 GPU 用法与测试条件

1. `ImageManager.cpp`、`IconManager.cpp`、`TextTextureCache.cpp`、`RenderResources.cpp` 已采用 Create TransferBuffer → Map/Unmap → Acquire CommandBuffer → Begin/End CopyPass → `SDL_UploadToGPUTexture()` → Submit 的模式。
2. SDL3 本仓头文件声明 `SDL_CopyGPUTextureToTexture()`，复制在 GPU 时间线执行；后续提交按顺序开始执行。`SDL_ReleaseGPUTexture()` 和 `SDL_ReleaseGPUTransferBuffer()` 会在安全时延迟释放。
3. 现有上传代码多数未完整检查 CopyPass 与提交结果，本任务只修复本范围调用，不做仓库级抽取或无关重构。
4. `tests/unittest/CMakeLists.txt` 显式枚举测试源文件，而本任务禁止修改 CMake；因此测试必须追加到已进入 `ui_unit_tests` 的既有测试源文件，不能新增一个未被构建的测试文件。

## 3. 推荐设计

### 3.1 最小资源边界

- `TextureAtlas` 继续独立持有自己的图集纹理和每次操作的短生命周期 TransferBuffer；不引入公共上传服务。
- `DeviceManager` 成为 GPU 白色纹理的唯一所有者；`RenderSystem` 只读取 `DeviceManager::getWhiteTexture()` 的非拥有裸指针。
- 不抽取通用 GPU 上传器：当前三处虽有步骤相似，但错误语义、目标格式、资源提交时机不同，且本任务禁止无关重构。新增抽象的维护成本大于本轮收益。

### 3.2 白色纹理创建时机

白色纹理应作为“候选设备创建成功”的组成部分，在 `DeviceManager::createDevice()` 设置新设备后立即创建并提交上传；只有设备和白色纹理均成功，`createDevice()` 才返回成功。

理由：

- `initialize()` 与后端回退都统一经过 `createDevice()`，不会遗漏重建。
- 建立不变量：`getDevice() != nullptr` 的成功初始化状态下，`getWhiteTexture() != nullptr`。
- 若创建或提交失败，可在候选设备尚未对外确认为成功前清理并尝试下一后端。
- 避免把白色纹理加入 `RenderSystem` 的初始化事务，也避免新增跨类回滚节点。

不采用“首次 getter 懒创建”：getter 会产生隐藏副作用和提交失败语义，且难以保持 `const` 访问契约。

### 3.3 图集扩容事务

扩容采用“先建立新状态、提交复制成功后再提交 CPU 状态”的事务式顺序：

1. 记录 `oldSize`，创建 `newSize` 纹理，但不改 `m_texture`、`m_size`、glyph 或 shelf 状态。
2. 在独立 CommandBuffer/CopyPass 中，将旧纹理 `(0,0,0)` 的 `oldSize × oldSize × 1` 区域复制到新纹理同位置。
3. 提交成功后才交换 `m_texture`，更新 `m_size`。
4. 保留 `m_glyphMap`、`m_shelves`、`m_currentShelfY`；遍历 glyph，按 `newSize` 重算 `u0/v0/u1/v1`。
5. 旧纹理由 RAII 在交换后释放；SDL 负责延迟到 GPU 不再引用时真正销毁。
6. 任一步失败则释放尚未提交为正式状态的新纹理，并保持旧纹理和全部 CPU 状态不变。

不调用 `SDL_WaitForGPUIdle()` 做每次上传或扩容同步；它会把偶发资源更新变为全设备停顿。依赖 SDL 同一设备提交顺序及延迟释放即可。

## 4. 逐条实施规划

### C1 — `TextureAtlas::uploadBitmap()` 真实上传

**改动类型**：局部正确性修复。

**实施步骤**：

1. 校验 `m_device`、`m_texture`、`bitmap`、正尺寸，以及目标矩形不越过 `m_size`；对乘法使用可安全转换的无符号尺寸，拒绝溢出/非法输入。
2. 以 `width × height` 字节创建 `SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD` 的 `UniqueGPUTransferBuffer`。
3. Map 失败时记录 `SDL_GetError()` 并返回 `false`；成功后逐行或连续复制 R8 位图，随后 Unmap。
4. 获取 CommandBuffer；失败则返回 `false`。
5. Begin CopyPass；失败时取消尚未提交的 CommandBuffer，返回 `false`。
6. 填充 `SDL_GPUTextureTransferInfo`：transfer buffer、offset 0、`pixels_per_row = width`、`rows_per_layer = height`。
7. 填充目标 `SDL_GPUTextureRegion`：当前图集、mip/layer 0、`x/y = xPos/yPos`、`z = 0`、`w/h = width/height`、`d = 1`。
8. 调用 `SDL_UploadToGPUTexture(..., cycle=false)`，结束 CopyPass，并检查 `SDL_SubmitGPUCommandBuffer()` 返回值；仅提交成功返回 `true`。
9. 删除占位 TODO 和“未实现却成功”的警告；失败日志应包含阶段和 `SDL_GetError()`。
10. 将 `m_texture` 用携带 `m_device` 的 deleter 初始化，删除析构/扩容中的手工 texture release，为 C3 的安全交换打基础。

**完成判据**：`addGlyph()` 只在命令成功提交后缓存 glyph；上传资源创建、映射、命令获取、CopyPass 或提交失败均返回 `nullopt` 到调用方。

### C2 — `DeviceManager` 白色纹理所有权

**改动类型**：内部接口扩展和所有权迁移；不改变公共安装 API。

**实施步骤**：

1. 在 `DeviceManager` 增加 `wrappers::UniqueGPUTexture m_whiteTexture`，并增加私有 `createWhiteTexture()`（建议返回 `bool`，保持该内部类当前风格）。
2. 创建 `1×1`、`SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM`、`SDL_GPU_TEXTUREUSAGE_SAMPLER` 的纹理；通过 4 字节 upload TransferBuffer 上传 `{255,255,255,255}`，避免依赖主机端整数端序表达。
3. 对纹理、TransferBuffer、Map、CommandBuffer、CopyPass 和 Submit 的失败逐级检查；若已有未提交 CommandBuffer，在失败路径取消。
4. `createDevice()` 在设备创建后调用白纹理创建；失败时清空白纹理、销毁该候选设备、清理 driver 状态并返回 `false`，允许初始化循环尝试下一后端。
5. `getWhiteTexture()` 改为返回 `m_whiteTexture.get()`，保持只读、无副作用。
6. `cleanup()` 在 `SDL_WaitForGPUIdle()` 后、`m_gpuDevice.reset()` 前执行 `m_whiteTexture.reset()`；确保重复 cleanup 幂等。后端切换仍调用同一 cleanup，因此不会跨设备保留纹理。
7. 删除 `RenderSystemImpl::m_whiteTexture`，删除 `RenderSystem::createWhiteTexture()` 声明和实现。
8. `RenderFrame.cpp` 不再每帧创建白纹理；GPU 路径直接从 `m_deviceManager->getWhiteTexture()` 写入 `RenderContext::whiteTexture`。Fallback 的哨兵纹理路径保持原样。
9. `RenderBackend.cpp::cleanup()` 删除对旧 `RenderSystemImpl::m_whiteTexture` 的重复释放，保持其他资源先于 `DeviceManager::cleanup()` 销毁的既有顺序。
10. 不改渲染器和 `RenderContext` 字段类型；它们继续使用非拥有裸指针。

**完成判据**：每个成功的 DeviceManager 设备代际恰有一个白色纹理；后端切换重建；cleanup 后 getter 返回空；RenderSystem 不再拥有或上传第二份白纹理。

### C3 — `TextureAtlas::expand()` 迁移与状态保持

**改动类型**：状态保持型内部重构，失败路径改为事务语义。

**实施步骤**：

1. 保留 4096 上限和两倍扩容策略；用与初始纹理完全一致的格式、层级、sample count 和 usage 创建新纹理。
2. 获取独立 CommandBuffer 和 CopyPass；构造旧、新纹理的 `SDL_GPUTextureLocation`，复制 `oldSize × oldSize × 1`。
3. CopyPass 创建失败时取消 CommandBuffer；提交失败时释放候选新纹理并保持旧状态。
4. 提交成功后交换 RAII 纹理并更新 `m_size`；不得清空 glyph、shelf 或当前 shelf Y。
5. 重算每个既有 glyph 的四个 UV；像素坐标、尺寸、bearing、advance 均保持不变。
6. 保留已有 shelf 的 x/y/height；新尺寸自然允许继续利用旧 shelf 的右侧空间，并允许从 `m_currentShelfY` 开新 shelf。
7. 删除“需重传全部 glyph”的警告和 TODO，改为成功迁移日志；失败日志标明 create/acquire/begin/submit 阶段。
8. 确认触发扩容的 `addGlyph()` 在 expand 成功后重新 allocate，再通过 C1 上传新 glyph；既有 glyph 查询在扩容前后均返回同一像素指标和按新尺寸更新的 UV。

**完成判据**：扩容成功不丢失任何旧 glyph 或 shelf 状态；扩容失败对外呈现旧图集完整可用；新 glyph 可在扩容后的空间继续添加。

## 5. 显式文件边界

### 5.1 允许实施文件（穷举）

| 编号 | 文件 | 允许改动 |
|---|---|---|
| A1 | `src/managers/TextureAtlas.hpp` | C1、C3；修正本类纹理 RAII 初始化及相关手工释放 |
| A2 | `src/managers/DeviceManager.hpp` | C2；白纹理创建、所有权、getter、候选设备失败回滚、cleanup 顺序 |
| A3 | `src/systems/render/RenderResources.cpp` | 删除 RenderSystem 白纹理创建实现，保留 renderer 初始化 |
| A4 | `src/systems/render/RenderSystemImpl.hpp` | 删除重复白纹理成员 |
| A5 | `src/systems/RenderSystem.hpp` | 删除私有 `createWhiteTexture()` 声明 |
| A6 | `src/systems/render/RenderFrame.cpp` | 改从 DeviceManager 获取白纹理，删除每帧创建路径 |
| A7 | `src/systems/render/RenderBackend.cpp` | 删除旧所有者的白纹理 cleanup，保持设备销毁顺序 |
| A8 | `tests/unittest/test_GPUTextureOwner.cpp` | 追加 TextureAtlas RAII/真实上传/扩容 GPU 集成测试；沿用既有测试目标 |
| A9 | `tests/unittest/test_DeviceClaimState.cpp` | 追加 DeviceManager 白纹理生命周期 GPU 集成测试；不改变状态类生产代码 |

代码工厂不得自行扩大列表。若实现发现必须修改其他文件，应停止该条目并回报架构变更申请。

### 5.2 明确禁止文件与目录

- `docs/todo/NON_THIRD_PARTY_TODO_MARKERS_2026-07-28.md`
- `docs/pm/run-20260728-0000-非第三方TODO正确性闭环.md`
- `CMakeLists.txt`、`src/CMakeLists.txt`、`tests/unittest/CMakeLists.txt` 及全部 CMake 文件
- `src/managers/BatchManager.hpp`，尤其 `BatchManager::optimize()`
- `include/` 下全部公共头和任何公共消息/协议
- `third_party/`、`build/`、`logs/`
- 除 A1～A9 外的源码和测试文件

禁止新增依赖、通用上传框架、公共协议改动、无关格式化/重构和 Git 操作。

## 6. SDL3 GPU 生命周期与同步策略

### 6.1 提交线程

- Acquire 与 Submit 必须在同一线程执行。`TextureAtlas::addGlyph()/expand()` 和 DeviceManager 初始化应继续由现有渲染/初始化线程调用。
- 本轮不增加跨线程队列或锁。若调用方未来从工作线程访问 `TextureAtlas`，属于新需求，当前不承诺线程安全。

### 6.2 上传资源

- CPU 仅在 Map/Unmap 期间写 TransferBuffer。
- CopyPass 结束且 CommandBuffer 提交后即可让 RAII 释放 TransferBuffer；SDL 保证在 GPU 安全时实际释放。
- `uploadBitmap()` 的“成功”定义为命令缓冲提交成功，不定义为 GPU 已执行完成；这是非阻塞渲染资源更新的正确边界。

### 6.3 提交顺序

- 字形上传/图集复制使用独立提交；后续渲染提交依赖 SDL 同设备提交顺序，无需 CPU fence。
- 图集扩容复制提交成功后才替换 CPU 可见纹理句柄。随后上传触发扩容的 glyph，会进入后续提交；再之后的渲染提交按顺序观察完整结果。

### 6.4 销毁与后端切换

- `TextureAtlas` 必须在创建它的 SDL_GPUDevice 销毁前析构；该既有调用方契约保持不变。
- `DeviceManager::cleanup()`：等待 GPU idle → 释放白色纹理 → 释放窗口声明 → 销毁设备。窗口释放与纹理释放的先后均在设备存活期内；关键不变量是纹理先于设备销毁。
- 正常逐帧上传或图集扩容不调用 `SDL_WaitForGPUIdle()`；仅设备总代际 cleanup 使用现有 idle wait。

### 6.5 失败命令处理

- Acquire 成功但 CopyPass 尚未建立即失败：调用 `SDL_CancelGPUCommandBuffer()`。
- CopyPass 已建立：必须先 End；随后 Submit，并以 Submit 的 bool 为最终成功判据。
- 提交失败不得更新 CPU 状态或暴露候选资源。SDL API 不提供对 `SDL_UploadToGPUTexture`/`SDL_CopyGPUTextureToTexture` 的逐调用 bool，因此错误边界落在资源/Pass 获取与提交阶段。

## 7. 测试方案

### 7.1 测试落点约束

由于禁止修改 CMake，不新增测试源文件：

- TextureAtlas 用例追加到 `tests/unittest/test_GPUTextureOwner.cpp`。
- DeviceManager 用例追加到 `tests/unittest/test_DeviceClaimState.cpp`。

GPU 集成 fixture 应尝试创建可用 SDL GPU 设备；环境确实无支持后端时使用 `GTEST_SKIP()` 并输出 SDL 错误。跳过只解决开发机无 GPU 的可移植性，不替代至少一个具备 D3D12 或 Vulkan 的 CI/验收环境。

### 7.2 C1 用例

1. **上传可读回**：创建小尺寸 TextureAtlas，添加已知 R8 图案；用独立 Download TransferBuffer + fence/readback 读取目标区域，断言字节一致。
2. **输入拒绝**：空 bitmap、零/负尺寸返回失败且 glyph map 不新增。
3. **缓存时机**：成功上传后 `getGlyph()` 存在；失败输入后不存在。
4. **析构释放**：沿用/扩充资源 owner 观察，确认带创建设备的 deleter 只释放一次；不得保留手工 release。

### 7.3 C2 用例

1. **初始化不变量**：`DeviceManager::initialize()` 成功时 device 与 white texture 均非空。
2. **白像素可读回**：下载 1×1 RGBA，断言四通道均为 255。
3. **幂等访问**：重复 getter 返回同一非拥有指针，不触发重建。
4. **cleanup**：调用后 device 与 white texture 均为空；重复 cleanup 不崩溃。
5. **代际验证**：如测试环境可稳定触发重建，则验证 cleanup → initialize 后重新得到本代资源；不比较地址必然不同，因为分配器可复用地址。

### 7.4 C3 用例

1. **迁移内容**：小初始图集添加多个已知图案，填满触发扩容；readback 断言扩容前区域内容仍一致。
2. **映射保持**：扩容前后 glyph 的像素坐标、尺寸、bearing、advance 不变；UV 等于像素坐标除以新尺寸。
3. **布局保持**：glyph 数和 shelf 使用状态不归零；扩容触发 glyph 及后续 glyph 均可分配。
4. **上限行为**：达到 4096 后无法继续扩容，已有图集与 glyph 仍有效。
5. **失败原子性**：若不引入注入 seam 无法稳定制造 SDL 提交失败，则以代码审查覆盖“提交前不写成员”的结构性断言；不得为此新增生产级抽象。该项记录为测试限制。

### 7.5 验收执行

按 PM 记录执行 Debug 全量闭环：

1. 构建 `ui`。
2. 构建 `example_ui_demo`。
3. 构建聚合目标 `ui_tests`。
4. 运行全部单元/集成测试。
5. 在至少一个真实 SDL GPU 后端运行 C1～C3 readback 用例；报告后端名、通过/跳过数和 SDL 错误。
6. 人工运行含文本和无纹理 Shape/Slider/ProgressBar 的 demo，确认白纹理渲染与大量字形触发后的显示无回退/闪烁。

## 8. 里程碑与派发顺序

| 里程碑 | 条目 | 文件 | 输出 | 可并行性 |
|---|---|---|---|---|
| M1 | C1 | A1、A8 | 真实 R8 上传、RAII 修正、readback 测试 | 与 M2 可并行，但合并时 A8 可能冲突 |
| M2 | C2 | A2～A7、A9 | 白纹理单一所有权及生命周期测试 | 与 M1 可并行 |
| M3 | C3 | A1、A8 | 事务式复制扩容、状态/UV 保持测试 | 必须在 M1 后 |
| M4 | 全量验证 | 不改文件 | Debug 构建、测试、demo 报告 | M1～M3 后 |

建议代码工厂串行处理 M1 → M2 → M3，以减少两个工作包同时编辑测试文件和 GPU 失败处理风格不一致的风险。

## 9. 风险与缓解

| 风险 | 等级 | 影响 | 缓解/判据 |
|---|---|---|---|
| 扩容复制成功但未重算 UV | 高 | 旧字形采样到错误区域 | C3 强制遍历重算，并测试精确 UV |
| 白纹理仍有双重所有者 | 高 | 重复资源、悬空指针或错误设备释放 | 删除 RenderSystem member/creator；仅 DeviceManager 持有 |
| 后端切换后沿用旧设备纹理 | 高 | 未定义行为/崩溃 | cleanup 先 reset 白纹理；每次 createDevice 重建 |
| 提交失败仍写入 glyph/新 atlas 状态 | 高 | CPU/GPU 状态分裂 | 以 Submit bool 为提交点，之前不写正式成员 |
| `TextureAtlas` 手工 release 与 RAII 混用 | 高 | 双释放或漏释放演进风险 | 初始化正确 deleter，删除手工 release |
| GPU readback 测试在无 GPU CI 全跳过 | 中 | 正确性回归未被自动发现 | 至少一个 GPU runner 必跑；报告 skip，人工 demo 补充但不替代 |
| 每 glyph 一次提交造成性能压力 | 中 | 大量新字形时提交开销 | 本轮先保证正确性；批量上传属于后续性能需求，不在本次抽象 |
| 多线程调用 Atlas/DeviceManager | 中 | SDL 提交线程违规、数据竞争 | 明确限定现有渲染/初始化线程；不在本轮增加锁 |
| `clear()` 只清 CPU 映射、不清 GPU 像素 | 低 | 旧像素残留但无有效 UV 引用 | 保持现状；后续分配上传覆盖有效区域，不属于本任务 |

## 10. SOLID / YAGNI 检查

- **SRP**：DeviceManager 管设备绑定资源；TextureAtlas 管图集纹理与布局；RenderSystem 不再重复持有设备基准纹理。
- **OCP**：不新增公共扩展点；当前需求由现有内部方法完成。
- **LSP**：无新增继承或替换关系。
- **ISP**：不扩充公共接口；getter 保持最小只读访问。
- **DIP**：直接依赖 SDL GPU 是现有底层 manager 的职责，不为测试制造通用接口层。
- **YAGNI/DRY**：不抽取全仓 GPU uploader，不批量修复其他 manager，不实现异步上传队列或 atlas 重打包。

## 11. 待确认问题

1. **GPU 验收环境（阻塞“真实 GPU 用例不得全跳过”的最终签收，不阻塞代码实施）**：当前资料未指定哪台 CI/验收机保证 D3D12 或 Vulkan 可用。PM/测试闭环需指定至少一个 GPU runner；否则只能报告 readback 测试跳过，不能宣称 GPU 正确性完全闭环。
2. **稳定失败注入（非阻塞）**：在禁止无关抽象和 CMake 变更的边界下，无法稳定自动制造 SDL CopyPass/Submit 失败。推荐本轮采用事务式代码结构审查覆盖，不新增生产注入接口；若必须自动验证该路径，需要另行批准测试 seam。
3. **TextureAtlas 调用线程（非阻塞，按现状约束）**：代码中未发现跨线程直接调用点。本规划要求其在渲染线程使用；若产品实际会从字体工作线程调用，需先新增上传排队设计，本规划不能直接安全覆盖该场景。
