# VMP-ui 架构重构执行批次

> 日期：2026-06-30  
> 输入来源：用户确认“直接执行”；承接 `docs/architecture/change-architecture-evaluation-20260630.md`。  
> 已确认决策：不考虑兼容，旧线路可直接移除；截图回归 Windows/Linux 同步；同线程多窗口优先，同时考虑跨线程独立 runtime。  
> 作用范围：`include/ui.hpp`、`src/api`、`src/detail`、`src/common`、`src/core`、`src/services`、`src/systems`、`src/renderers`、`tests/unittest`、`tools/check_architecture_boundaries.py`、`example/ui_demo`。

## 1. 执行原则

- 不做长期 deprecated 兼容层；确认替代路径后直接删除旧入口。
- 每个批次必须保持可构建，避免一次性大爆炸。
- P0 先处理边界和 runtime 归属，不先补新控件。
- 同线程多窗口作为第一验收场景；跨线程独立 runtime 在同一套归属模型上扩展。
- Windows/Linux 截图回归同步建设，但允许先落最小基线工具，再逐步扩大样例覆盖。

## 2. 推荐执行批次

| 批次 | 优先级 | 目标 | 文件范围 | 完成标准 |
|---|---:|---|---|---|
| B0 | P0 | 加严架构门禁 | `tools/check_architecture_boundaries.py` | 禁止新增 `RuntimeFacade::current()`、`.raw()`、公开 API 头 EnTT/Runtime include；baseline 只减不增。 |
| B1 | P0 | 清理旧线路与未接入实现 | `include/ui.hpp`、`src/api`、`src/detail`、`src/CMakeLists.txt` | 未接入 `detail/*.cpp` 分类处理：删除废弃文件或接入新路径；旧兼容入口从 umbrella header 移除。 |
| B2 | P0 | 设计并落地 runtime-aware 句柄骨架 | `src/api/Entity.hpp`、`src/core/UiRuntime.*`、`src/core/Application.*` | `Entity`/`Window` 句柄携带 runtime token；跨 runtime 比较和校验有明确失败路径。 |
| B3 | P0 | Factory 改为显式 runtime/window 创建 | `src/api/Factory.*`、`src/detail/Factory.*`、`example/ui_demo` | 主创建入口不依赖隐式 current runtime；示例完成迁移。 |
| B4 | P0 | 同线程多窗口验收 | `src/core/PlatformWindow.*`、`src/core/WindowEntityLookup.*`、`src/systems/*`、`tests/unittest` | 单 runtime 多窗口输入、布局、渲染目标、关闭事件互不串扰。 |
| B5 | P0 | System 依赖注入路径 | `src/core/SystemManager.*`、`src/systems/TimerSystem.*`、`src/systems/ThemeSystem.*`、`src/systems/HitTestSystem.*` | 先迁低风险系统，迁一个减少一个 `RuntimeFacade::current()` baseline。 |
| B6 | P1 | 拆分 `StateSystem` 根能力 | `src/systems/StateSystem.*`、新增 `src/services/FocusManager.*`、`src/services/OverlayManager.*` | Focus、Overlay、Scroll/Slider 从输入状态协调中分离；DropDown 接入 Overlay。 |
| B7 | P1 | Windows/Linux 截图回归最小闭环 | `tests`、`tools`、`example/ui_demo` | 固定字体、DPI、窗口尺寸；生成基准图、实际图、差异图；Windows/Linux 分平台阈值。 |
| B8 | P1 | 渲染数据边界 | `src/renderers/*`、`src/systems/render/*`、`src/interface/IRenderer.hpp` | 开始用 `RenderItem`/`DrawCommand` 承接截图回归和后端绘制，减少 renderer 直接查 registry。 |

## 3. 立即派发任务

| 序号 | 任务 | 依赖 | 产出 |
|---:|---|---|---|
| 1 | 修改架构边界脚本，新增硬规则并保留现有 baseline | 无 | `tools/check_architecture_boundaries.py` 更新；边界检查可独立运行。 |
| 2 | 盘点 `src/detail/*.cpp` 未进入 `UI_SOURCES` 的文件 | 任务 1 | 删除清单、迁移清单、保留理由。 |
| 3 | 新增公开头隔离测试 | 任务 1 | `include/ui.hpp` 不泄漏 EnTT/Runtime 的测试。 |
| 4 | 设计 `RuntimeToken` / `Entity` / `Window` 最小结构 | 无 | 头文件骨架与单元测试。 |
| 5 | 改造最小 Factory 路径：Application/Window/Button | 任务 4 | 示例能用新入口创建应用、窗口、按钮。 |
| 6 | 添加同线程多窗口冒烟测试 | 任务 5 | 两个窗口创建、显示、关闭、事件路由基础测试。 |

## 4. 当前执行进度

| 批次/任务 | 状态 | 已完成内容 | 验证 |
|---|---|---|---|
| B0 加严架构门禁 | 已完成 | `tools/check_architecture_boundaries.py` 新增 stale baseline 检查；baseline 只减不增；收缩已过期 `RuntimeFacade::current()` 与未接入 detail cpp baseline。 | `python tools/check_architecture_boundaries.py --root .` 通过。 |
| B1 清理旧线路与未接入实现 | 已完成 | 删除未接入且已有 `src/api` 对应实现替代的 `src/detail/Factory.cpp`、`Timer.cpp`、`Shortcut.cpp`、`Utils.cpp`、`Log.cpp`；未接入 detail cpp baseline 清零。 | Debug 构建通过。 |
| B2 runtime-aware 句柄骨架 | 已完成 | 新增 `RuntimeToken`、`EntityHandle`、`WindowHandle`、`MakeEntityHandle`、`MakeWindowHandle`、`SameRuntime`；`UiRuntime::token()`；`Application::runtime()`。 | 新增 `UiRuntimeTest.RuntimeTokenIdentifiesRuntimeOwnership`、`EntityAndWindowHandlesCarryRuntimeOwnership`。 |
| B3 Factory 显式 runtime/window 创建 | 部分完成 | 新增 `CreateBaseWidget(UiRuntime&)`、`CreateButton(UiRuntime&)`、`CreateWindow(UiRuntime&)` 最小显式入口，返回 runtime-aware handle。 | 新增 `UiRuntimeTest.ExplicitRuntimeFactoryCreatesOwnedButtonHandle`。 |
| B4 同线程多窗口验收 | 部分完成 | 新增无 SDL 冒烟测试，验证同 runtime 下多个 `WindowHandle` 归属一致但实体 ID 与平台窗口 ID 独立。 | 新增 `UiRuntimeTest.SameRuntimeWindowHandlesKeepIndependentEntityAndPlatformIds`。 |
| B5 System 依赖注入路径 | 大部分完成 | 批量迁移 `TimerSystem`、`ThemeSystem`、`HitTestSystem`、`InteractionSystem`、`TweenSystem`、`TextInputSystem`/`TextEditingService`、`PlatformWindowSystem`、`ShapeRenderer`、`StateSystem`、`ActionSystem` 的 `RuntimeFacade::current()` 使用；`TimerSystem` 静态旧 API 改为显式注入实例方法，`src/systems` 侧 `RuntimeFacade::current()` baseline 清零；API 层 `Utils.cpp` 与 `Image.cpp` 的 `RuntimeFacade::current()` baseline 清零。 | 架构边界检查通过；Debug 构建通过；全量测试 131/131 通过。 |
| 测试构建债务修复 | 已完成 | 补齐测试 include；修复 `entt::null` 与 `ui::entity` 重载歧义；修复 `popupChildren(ui::entity)` 调用。 | `ctest --test-dir build --output-on-failure`：131/131 通过。 |

### 4.1 当前剩余重点

1. B3 尚未完成：需要继续迁移示例和更多 `factory::Create*` 入口到显式 runtime/window 绑定；随后删除旧隐式 current 主入口。
2. B4 尚未完成：当前只有 handle 层无 SDL 冒烟测试，还需要覆盖真实窗口实体、事件路由、关闭/resize/pointer hit 不串扰。
3. B5 剩余集中债务：API 层 `RuntimeFacade::current()` baseline 仅剩 `Factory.cpp` 1 处；`TimerSystem.cpp` 静态旧 API 3 处已清零；`ui::timer` / `ui::utils` / `ui::image` 过渡桥接仍临时复用旧 active singleton，后续应迁到显式 runtime API。
4. B6-B8 尚未开始：Focus/Overlay 拆分、Windows/Linux 截图回归、渲染数据边界。

## 5. 风险控制

| 风险 | 控制方式 |
|---|---|
| 删除旧入口导致调用点大量失败 | 先改示例和测试，再删 umbrella 暴露；构建失败按调用点迁移，不做兼容回退。 |
| runtime token 设计过重 | 先做不可伪造轻量 token，只用于归属校验，不引入大型对象图。 |
| 多窗口状态串扰 | 所有窗口事件必须携带 window/entity 归属；测试优先覆盖关闭、resize、pointer hit。 |
| 跨线程误用 | Runtime 记录 owner thread；非 owner thread UI 写操作直接失败或要求 mailbox 投递。 |
| 截图测试不稳定 | 每平台维护独立基准，不追求跨平台像素完全一致。 |

## 6. 验证命令

```bash
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
```

如未启用测试，先配置：

```bash
cmake -G Ninja -B build -DENABLE_BUILD_TESTS=ON
```

## 7. 待确认问题

暂无。
