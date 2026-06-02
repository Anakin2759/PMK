# 缩放链路白边问题评估与优化规划

> 日期：2026-06-02  
> 状态：规划中  
> 触发场景：`example_ui_demo` 主窗口缩小到接近最小尺寸后，底部出现白色未绘制区域。

## 1. 现象判断

截图中的白边不像控件样式本身，而更像窗口客户区有一段没有被 UI 渲染覆盖。当前 GPU 渲染清屏色为透明黑，若某段客户区没有被根窗口背景绘制，Windows/DWM 或底层窗口背景就会透出，表现为白色区域。

高概率原因不是单点 DPI 乘错，而是以下链路中至少一处尺寸事实不同步：

1. SDL 窗口逻辑尺寸，也就是 `SDL_GetWindowSize()`。
2. SDL backbuffer 物理尺寸，也就是 `SDL_GetWindowSizeInPixels()`。
3. `components::Window::logicalSize / pixelSize / displayScale`。
4. 根窗口 `components::Size`，即 Yoga 计算根尺寸。
5. shader `screen_size`，当前来自逻辑尺寸。
6. GPU viewport / scissor，当前来自物理尺寸和 `dpiScale`。
7. 实际取得的 swapchain texture 尺寸。

白边位于底部，且高度接近标题栏高度，优先怀疑运行期窗口缩放或最小尺寸附近存在“客户区高度”和“布局/渲染高度”不同步，而透明清屏让未覆盖区域直接暴露出来。

## 2. 当前缩放链路评估

### 2.1 UI API 与布局输入

- `Scale.hpp` 在 DSL 写入尺寸时使用 `AppConfig::platformUiScale()` 做一次性放大。
- `Factory.cpp` 默认窗口 flags 在平台缩放开启时包含 `SDL_WINDOW_HIGH_PIXEL_DENSITY`。
- Demo 主窗口声明尺寸为 `1200 x 960`，用户缩小窗口后依赖窗口事件把根 `Size` 同步为当前逻辑客户区尺寸。

风险：`Scale::Metric()` 是全局、创建时生效，不是每窗口、每帧状态。多窗口跨 DPI 或运行期 DPI 变化时，已有组件尺寸不会天然重算。

### 2.2 SDL 窗口与 Window 组件同步

- `WindowSync::SyncWindowDisplayMetrics()` 读取 `SDL_GetWindowSize()`、`SDL_GetWindowSizeInPixels()`，写入 `Window::logicalSize`、`Window::pixelSize`。
- `WindowSync::SyncWindowSize()` 会用根 `Size` 反向调用 `SDL_SetWindowSize()`。
- `SyncWindowProperties()` 只在显示窗口路径触发；常规渲染帧主要调用 `SyncWindowDisplayMetrics()`。
- `PlatformWindowSystem` 将 resize、pixel-size、display-scale、exposed 等 SDL 窗口事件统一 enqueue 为 `WindowPixelSizeChanged`。
- `StateSystem::handlePixelSizeChanged()` 再把窗口逻辑尺寸写回根 `Size`，并标记 layout/render dirty。

风险：同一个根窗口尺寸既可能由用户 DSL 写入，也可能由 SDL 事件写回，还可能被 `SyncWindowSize()` 反向设置到 SDL。这里需要明确“用户拖拽 resize 时 SDL 是事实源；代码显式 `Size()` 时 ECS 是事实源”的优先级，否则容易出现一帧或多帧来回覆盖。

### 2.3 Yoga 布局

- `LayoutSystem::update()` 对 dirty root 读取根 `Size`，传入 `YGNodeCalculateLayout(rootNode, rootWidth, rootHeight, ...)`。
- 布局输出写回 `Position` / `Size`，渲染器再按逻辑像素绘制。

风险：如果 `WindowPixelSizeChanged` 没及时触发，或根 `Size` 被 `SyncWindowSize()` 回写覆盖，Yoga 根高度可能小于实际客户区高度，导致底部没有任何背景批次覆盖。

### 2.4 渲染与 GPU 提交

- `RenderFrame.cpp` 每帧再次调用 `SyncWindowDisplayMetrics()`。
- `screen_size` 使用逻辑宽高，和 UI 顶点逻辑坐标一致。
- `CommandBuffer::execute()` 使用物理宽高设置 GPU viewport。
- `CommandBuffer::recordRenderPass()` 对 scissor 做 `logical * dpiScale` 换算。
- 当前仅在 `batches` 非空时调用 `CommandBuffer::execute()`；无批次帧不会提交清屏。
- GPU clear color 是透明 `{0, 0, 0, 0}`。

风险：非透明普通窗口也使用透明清屏。只要根背景没有覆盖完整 swapchain，白色底色就会暴露。并且 `dpiScale` 当前来自 `Window::displayScale`，它不一定严格等于 `pixelSize / logicalSize`；在 SDL 后端、窗口跨屏或 resize 边界时，viewport 与 scissor 可能出现舍入差异。

## 3. 建议先加的观测点

先不要直接改布局或渲染策略。建议增加一个可开关的诊断日志，例如 `UI_DEBUG_SCALING=ON` 或 `AppConfig::debugScalingMetrics()`。

每次窗口显示、resize、pixel-size changed、display-scale changed、render submit 时记录：

| 字段 | 来源 |
|------|------|
| `windowID` | SDL event / Window component |
| `event.type`、`data1/data2` | SDL window event |
| `sdlLogical` | `SDL_GetWindowSize()` |
| `sdlPixel` | `SDL_GetWindowSizeInPixels()` |
| `computedFramebufferScale` | `pixel / logical`，分别记录 x/y |
| `displayScale` | `platform::GetWindowFramebufferScale()` |
| `uiScale` | `platform::GetWindowUiScale()` |
| `windowComp.logicalSize/pixelSize` | Window component |
| `root Size` | root `components::Size` |
| `render screen_size` | `RenderContext::screenWidth/screenHeight` |
| `viewport` | `CommandBuffer::recordRenderPass()` 入参 |
| `first/root background batch rect` | BackgroundRenderer 或 BatchManager 输出 |

验收目标：复现白边时，能直接看到哪一段高度少了那一截，而不是靠截图猜。

## 4. 优化规划

### P0：统一窗口尺寸事实源

目标：避免 resize 时 ECS 和 SDL 互相覆盖。

1. 引入窗口尺寸同步状态，例如 `Window::lastAppliedLogicalSize` 和 `Window::resizeSource`。
2. 用户 DSL 或程序化 `Size()` 修改根窗口时，标记为 `EcsRequestedSize`，允许 `SyncWindowSize()` 调 SDL。
3. SDL resize / pixel-size event 到来时，标记为 `PlatformReportedSize`，由 `StateSystem` 写回 root `Size`，本帧禁止 `SyncWindowSize()` 反向覆盖。
4. Dialog 可保留固定尺寸策略，但普通 `WindowTag` 应以 SDL 当前客户区为 resize 后事实源。

### P1：把 framebuffer scale 改为可验证的派生值

目标：GPU viewport/scissor 只依赖同一组逻辑/物理尺寸。

1. 在 `SyncWindowDisplayMetrics()` 中计算：
   - `framebufferScaleX = pixelWidth / logicalWidth`
   - `framebufferScaleY = pixelHeight / logicalHeight`
2. `displayScale` 可继续保存平台报告值，但渲染用 `dpiScale` 应优先来自 `pixel/logical`。
3. 若 x/y scale 不一致，scissor 分别按 x/y 缩放，避免只用单个 float。
4. 对 `displayScale` 和 `pixel/logical` 差异超过 epsilon 的情况打 debug log。

### P2：保证整张 swapchain 每帧都有确定内容

目标：普通窗口永远不露白，透明窗口仍能保持圆角/透明能力。

1. `CommandBuffer` 增加 `clearOnly()` 或允许空 batch 也执行 render pass clear。
2. 非透明窗口 clear color 使用窗口背景色或主题背景色，alpha 为 1。
3. 透明/无边框/圆角窗口继续允许 alpha 0 clear，但必须确保根背景覆盖其非透明区域。
4. 渲染提交前检查 root background batch 是否覆盖 `[0, 0, logicalWidth, logicalHeight]`；debug 下可警告。

### P3：根窗口布局覆盖客户区

目标：root `Size` 与 SDL logical client size 始终一致，除非是固定 Dialog。

1. `StateSystem::handlePixelSizeChanged()` 写 root `Size` 后立即 `MarkLayoutAndVisualChanged(root)`。
2. `RenderFrame` 如发现 `Window::logicalSize` 与 root `Size` 不一致，debug 下记录；必要时触发下一帧 layout dirty。
3. 普通窗口根背景建议作为 viewport 背景而不是普通子树背景，降低内容溢出或布局失败导致露底的概率。

### P4：缩放策略从全局改为每窗口状态

目标：支持多窗口、多显示器、跨 DPI 热变化。

1. `scale::Metric()` 继续作为默认 API，但需要定义它只用于创建时初始尺寸。
2. 新增每窗口 `WindowScaleContext`：`uiScale`、`framebufferScaleX/Y`、`logicalSize`、`pixelSize`。
3. 字体、图标、图片纹理缓存键纳入窗口 scale 或 raster scale，避免不同 DPI 窗口互相污染。
4. `WindowDisplayScaleChanged` 单独触发缓存失效，不和普通 resize 混在一起。

### P5：自动化验证

目标：防止白边回归。

1. 增加最小尺寸窗口截图验证：`300x200`、`320x240`、`800x600`。
2. 对截图底部和右侧 4px 做像素扫描，普通窗口不允许出现大面积纯白或透明底色。
3. 覆盖 `1.0x`、`1.25x`、`1.5x`、`2.0x` forced scale。
4. 增加 resize 过程验证：连续缩小、放大、跨越最小尺寸。

## 5. 推荐落地顺序

1. 先加 `UI_DEBUG_SCALING` 观测日志，复现一次白边，确认缺口发生在 SDL、Window component、Yoga root、viewport 还是 batch 覆盖。
2. 做 P2 的“非透明窗口不露底”兜底，这一项风险最低，能先消除用户可见白边。
3. 做 P0/P3，修正 resize 事实源和 root 尺寸同步。
4. 做 P1，把渲染用 scale 从单 float `displayScale` 收敛为 `pixel/logical` 派生值。
5. 最后做 P4/P5，覆盖多窗口和自动化验证。

## 6. 预期结论

白边的直接视觉原因是：窗口客户区底部有区域未被 UI 背景绘制，而当前透明 clear 让底色透出。根本原因需要通过尺寸探针确认，但从代码链路看，最可能是 resize 后 SDL logical size、root Yoga size 与 GPU viewport/backbuffer size 在一段时间内不一致。优化应先建立缩放链路观测，再统一窗口尺寸事实源，并让普通窗口渲染路径具备不露底的兜底清屏策略。