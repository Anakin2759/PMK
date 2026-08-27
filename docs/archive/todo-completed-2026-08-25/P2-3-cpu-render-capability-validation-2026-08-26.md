# P2-3 CPU fallback 渲染能力验证记录

状态：DONE  
最后复核：2026-08-26  
责任范围：GPU/fallback 能力矩阵、调用侧降级语义、software 像素与结构回归  
来源：`docs/todo/CPU_RENDER_ISSUES.md`

## 历史问题摘要

早期 CPU fallback 存在四类问题：圆角被忽略、图片无法显示、Canvas 圆形退化为矩形，以及斜线抗锯齿质量较低。旧文档中的代码片段和行号已失效，本记录只保留最终状态与可重复验证证据。

## 最终能力契约

能力等级的唯一事实源为 `src/interface/IBackendRenderer.hpp` 中的 `GetBackendCapabilityStatus()`。

| 能力 | GPU | fallback | 最终语义 |
|---|---|---|---|
| 纯色矩形 | Supported | Supported | 两端基础几何一致 |
| 变换纯色四边形 | Supported | Degraded | fallback 使用 `SDL_RenderGeometry` |
| 圆角矩形 | Supported | Degraded | fallback 使用分段软件近似 |
| 边框 | Supported | Unsupported | 省略边框，保留主体并诊断 |
| 阴影 | Supported | Unsupported | 省略阴影，保留主体并诊断 |
| 缓存位图 | Supported | Supported | fallback 用 SDL texture 缓存绘制 |
| 位图 RGB 调色 | Supported | Unsupported | 绘制原图并诊断 |
| 位图 UV 裁切 | Supported | Unsupported | 绘制完整图片并诊断 |
| 填充圆/圆形描边 | Supported | Degraded | fallback 使用软件近似 |
| Capsule | Supported | Unsupported | Canvas 线段降级为 transformed quad，省略圆头 |

`Degraded` 表示功能仍可提交但不保证与 GPU/SDF 像素等价；`Unsupported` 子效果不得使整帧失败。只有资源解码、SDL 绘制/上传或 GPU submit 等真实执行失败才通过 `Result` 传播并保留 dirty。

## 历史问题最终状态

- 圆角：已恢复软件绘制，作为 Degraded 能力维护。
- 图片：基础缓存位图已支持；RGB tint 和 UV crop 是明确的 Unsupported 产品限制，不再作为黑屏缺陷。
- Canvas 圆形：已恢复软件近似，不再退化为正方形。
- Canvas 线段：capsule 不支持时降级为变换四边形，线条主体保留。
- 边框和阴影：明确不支持，只省略子效果。
- 抗锯齿：属于 fallback 质量限制；若未来提高质量，应新建立项。

## 回归证据

- `BackendCapabilityMatrixTest`：GPU 全能力声明和 fallback 三态分类。
- `BackendRenderContractTest.ShapeKeepsBaseGeometryAcrossGpuAndFallbackCapabilities`：同一 Shape 输入在 GPU/fallback 能力下保持位置、尺寸、颜色、Alpha 和基础四边形；fallback 允许省略边框/阴影但必须诊断。
- `CanvasFallbackCapabilityTest`：圆形批次保留，capsule 降级后线段主体保留。
- `ImageFallbackCapabilityTest`：Unsupported 与真实位图绘制错误传播分离。
- `TextFallbackCapabilityTest`、`TextRenderContextContractTest`：fallback 位图能力与必需 manager 契约。
- `FallbackRendererPixelTest`：offscreen software 纯色、圆角、缓存位图 Alpha 和 scissor 像素读回。
- `FallbackWindowLifecycleTest`：单/多窗口 software renderer 生命周期。

2026-08-26 验证：

- `ctest --test-dir build -C Debug -L unit --output-on-failure`：106/106 通过。
- 既有 fallback software 像素与生命周期回归此前已通过；结构门禁不依赖真实 GPU，适合无 GPU 环境必跑。

## 关闭结论

P2-3 的功能契约、降级可观测性、关键控件同输入结构门禁和 fallback 实际像素证据已经建立。真实 GPU 离屏读回可以作为后续增强，但无 GPU 时不应阻塞本项关闭。

未来若要求 fallback 支持阴影、边框、位图调色、UV crop 或更高质量抗锯齿，应创建独立需求，不重新激活旧缺陷清单。
