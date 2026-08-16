# 项目经理协调记录 - P0-1 CPU fallback 渲染修复

- 时间：2026-08-14
- 输入来源：`docs/architecture/ARCHITECTURE_REVIEW_2026-08-13.md` P0-1 与用户端到端闭环要求
- 本轮范围：规划、实现、中文注释收尾、验证全流程
- 验收标准：CPU fallback 不再静默丢弃非白纹理；圆角、图片、Canvas 圆与斜线得到小而正确的修复或节流告警；新增窄范围回归测试；相关构建、测试与公共头门禁通过；不提交或推送 Git

## 已实读基线

- `src/renderers/FallbackBackendRenderer.hpp`：圆角近似已有部分实现；非白纹理 warning 条件误置在 renderer/空批次分支，真正纹理跳过仍静默；四边形仍按 AABB 绘制。
- `src/renderers/CanvasRenderer.hpp`：圆使用 SDF 参数矩形批次；线段已经生成有向四边形，但 fallback 的 AABB 消费方式破坏方向。
- `src/renderers/ImageRenderer.hpp`、`src/managers/ImageManager.hpp`：图片仍只走 GPU 上传，没有 CPU 像素/bitmap 分支。
- `tests/unittest/CMakeLists.txt`：尚无 fallback renderer 窄测目标；现有 lifecycle target 可复用 offscreen SDL 属性。

## 工作包
| # | 工作包 | Agent | 输入 | 产物 | 状态 |
|---|---|---|---|---|---|
| 1 | 影响分析与实施规划 | 架构师 | 架构评审 P0-1、CPU issue 文档、实读基线 | `docs/architecture/P0_1_CPU_FALLBACK_RENDER_PLAN_2026-08-14.md` | 待办 |
| 2 | 小范围源码与回归测试落地 | 代码工厂 | 工作包 #1 规划条目 | 源码/测试/CMake 变更与交付报告 | 待办 |
| 3 | 中文 Doxygen/注释事实对齐 | 中文 Doxygen 注释与 Git 同步 | 工作包 #2 交付报告 | 注释变更表；Git 状态为未执行 | 待办 |
| 4 | CPU fallback 构建测试闭环 | 测试构建闭环 | 工作包 #2/#3 产物 | 验证报告与问题日志 | 待办 |

## 派发边界

### #1 架构师
- 目标：形成最小正确实现的逐文件规划，明确完整支持与 warning 降级边界。
- 依据：`docs/architecture/ARCHITECTURE_REVIEW_2026-08-13.md` P0-1、`docs/todo/CPU_RENDER_ISSUES.md`、上述实读基线。
- 允许文件：只读 `src/renderers/FallbackBackendRenderer.hpp`、`src/renderers/CanvasRenderer.hpp`、`src/renderers/ImageRenderer.hpp`、`src/managers/ImageManager.*`、相关内部接口/组件、`tests/unittest/CMakeLists.txt` 和相关测试。
- 禁止文件：公共头 `include/ui/**` 的依赖扩张、无关模块、构建拓扑重构；禁止改代码。
- 验收标准：规划含条目号、显式文件清单、行为、测试、风险、限制和下一步。
- 升级条件：图片 CPU 解码需公共 API 变化、新依赖或跨生命周期重构；现有批次结构无法区分 SDF 图元。

### #2 代码工厂
- 目标：按规划落地圆角、非白纹理告警、图片 bitmap、Canvas 圆/线方向与窄测。
- 依据：工作包 #1 规划文档的明确条目。
- 允许文件：由规划显式列出的 `src/renderers/**`、`src/managers/ImageManager.*`、`tests/unittest/test_FallbackRenderer.cpp`、`tests/unittest/CMakeLists.txt`。
- 禁止文件：`include/ui/**`、无关源码/测试、第三方、架构文档、PM 文档、Git 操作；禁止新依赖/新抽象/风格润色。
- 验收标准：不静默降级；warning 每实例节流/去重；窄测覆盖几何/图片/告警可测行为；交付报告列明变更文件与静态自检。
- 升级条件：需改变公共 API/批次协议或 SDL API 无法可靠实现；静态诊断无法收敛。

### #3 中文注释
- 目标：仅对本轮变更补齐或纠正中文 Doxygen/关键降级说明。
- 依据：工作包 #2 交付报告。
- 允许文件：工作包 #2 实际变更的源码与测试文件。
- 禁止文件：业务逻辑、接口语义、构建行为、无关文档、Git 提交/推送。
- 验收标准：注释与事实一致，输出注释变更表和“未执行 Git”状态。
- 升级条件：发现注释所述行为与代码不一致且需改逻辑。

### #4 测试构建闭环
- 目标：configure/build/窄测/相关测试/门禁闭环。
- 依据：工作包 #2/#3 交付结果。
- target/tests：`ui_fallback_renderer_tests`（若规划采用独立目标）、`ui_fallback_lifecycle_tests`、`ui_unit_tests`、`ui_public_headers_self_contained_check`、`ui_architecture_boundary_check`；CPU 构建使用 `UI_FORCE_CPU_RENDER=ON` 的独立 build 目录或等价 preset。
- 报告期望：阶段命令、结果、失败摘要、问题日志新增条数、报告路径。
- 失败策略：测试/CMake 笔误可在已授权测试范围内自修；业务/API/公共边界失败升级回代码工厂；不执行 Git。
- 验收标准：相关阶段全绿，或报告与问题日志完整。
- 升级条件：同一阶段修复两次仍失败、环境阻塞、需越界源码修改。

## 调度时间线
- 2026-08-14 完成路由判定：请求明确要求端到端规划、实现、中文注释、验证，采用 Full PM Path。
- 2026-08-14 完成第一轮相关代码、测试与 CMake 实读，确认部分修复与剩余缺陷。

## 待用户决策
- [ ] 当前无；若完整图片支持要求公共 API 或大范围生命周期改造，将升级确认。

## 结论
- 状态：进行中
- 关键产物：本协调记录
- 下一工作包：#1 影响分析与实施规划
