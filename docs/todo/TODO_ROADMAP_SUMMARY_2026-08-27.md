# TODO 路线总结

> 状态：ACTIVE
> 最后复核：2026-08-27
> 本文是当前待办的执行摘要；`docs/todo/README.md` 是索引入口，历史审查与验证记录不作为当前状态依据。

## 一、已完成基线

| 范围 | 状态 | 验收证据 |
|---|---|---|
| P0-1～P0-3 | DONE | EventLoop、Runtime ownership、回调快照/重入专项测试 |
| P1-1、P1-4～P1-9 | DONE | dirty 重试、typed buffered events、多窗口、fps=0、SDL 唤醒、CMake、fixture 专项测试 |
| P1-2 | DONE | 生产 `UiRuntime::current()` 调用归零；架构边界门禁通过 |
| P1-3 | DONE | 嵌套 Scope 与前 Runtime 提前销毁回归测试通过 |
| P2-1～P2-3 | DONE | System phase、ImageManager 缓存生命周期、fallback 能力矩阵与结构/像素测试通过 |
| P2-4 | DONE | 本轮文档分类、状态统一、重复规划收口完成 |

最近已知验证：Debug `all` 构建通过；公共头自包含门禁、架构边界门禁通过；unit 标签 108/108 通过。

## 二、当前活动规划

| 优先级 | 主题 | 文档 | 下一步与验收 |
|---|---|---|---|
| P1 | HiDPI P5 | `HIDPI_SUPPORT_PLAN.md` | 明确跨平台窗口/字体/裁剪验收；补对应测试后关闭 |
| P1 | 自适应平台缩放 | `ADAPTIVE_PLATFORM_SCALING_PLAN.md` | 与 HiDPI 方案合并边界，确认事件链路和多窗口语义 |
| P1 | Rich Text | `RICH_TEXT_PLAN.md` | 先冻结组件、布局、字体与错误传播 API，再实现最小闭环 |
| P1 | SVG | `SVG_SUPPORT_PLAN.md` | 冻结 source/cache/render API，明确 CPU fallback 和资源生命周期 |
| P2 | 缩放白边 | `SCALING_WHITE_BORDER_OPTIMIZATION_PLAN.md` | 先建立可复现像素基线，再决定采样/裁剪修复 |
| P2 | 测试模块治理 | `TEST_MODULE_REVIEW_PLAN_2026-06-04.md` | 复核测试分类、CTest 标签和 CI 资源隔离 |

## 三、需要复核但不直接开工

- `UI_FRAMEWORK_GAP_ANALYSIS_2026-05.md`：历史能力缺口快照，仅用于背景。
- GPU 真实设备/离屏读回、故障注入、TSan、ABI 和跨 Runtime 集成：属于可选工程增强，不能替代必需专项验收。
- `THEME_STYLE_SYSTEM_PLAN.md`：最小 ThemeSystem 已完成；CSS 选择器、继承、层叠和完整 DSL 需要另立产品规划。

## 四、依赖与执行顺序

1. 先完成 HiDPI P5 与自适应缩放的边界复核，避免 Rich Text/SVG 重复设计 DPI 与资源缩放语义。
2. 冻结 Rich Text/SVG 的公共 API 和 `Result` 错误契约，再实现最小垂直切片。
3. 在功能实现后分别补结构测试、software 像素测试和 Debug 全量构建；不得以历史报告代替复跑。
4. 新增 P0/P1 规划前，先在 README 中登记唯一主文档；完成项移入 `docs/archive/`。

## 五、文档的记录归属

- 当前状态与优先级：`docs/todo/README.md`
- 当前路线摘要：本文
- 架构分析与阶段历史：`docs/ARCHITECTURE_REVIEW_PLAN_2026-08-25.md`
- 已完成验证：`docs/archive/todo-completed-2026-08-25/`
- 问题/环境记录：`docs/todo/problem-log-*.md`，只保留事实，不产生隐含待办
