# ThreadPool 去留决策 — 结论：完全移除

> 日期：2026-08-09
> 输入来源：2026-08-09 架构评审（原文档当前未保留；S1/M1/M6）+ 用户决策（"证明没必要则完全移除"）+ 本次只读实证
> 作用范围：`src/utils/ThreadPool.hpp`、`src/utils/MpmcQueue.hpp`、`src/utils/WorkStealingDeque.hpp`、`src/utils/Singleton.hpp`、`src/core/UiRuntime.hpp`、`src/CMakeLists.txt`、`tests/unittest/CMakeLists.txt`、`tests/benchmark/`
> 本次评估结论已由本文完整保留；不依赖未保留的原始评审或规划文件

---

## 1. 结论（一句话）

**移除**：`ThreadPool` 及其专属依赖（`MpmcQueue.hpp`、`WorkStealingDeque.hpp`）与 `UI_ENABLE_MULTITHREAD` 选项全部移除；`MpscQueue.hpp` 保留（EventLoop 在用）；`Singleton.hpp` 作为独立死代码一并移除；`tests/benchmark/` 空目录一并清理。

## 2. 评估问题逐条回答

### 2.1 必要性证据链：不存在"必须跨线程且适合 ThreadPool"的场景

| 候选异步场景 | 实证 | 结论 |
|---|---|---|
| UI 状态（registry/dispatcher/组件） | `entt::registry` 官方非线程安全；评审 §5 确认仅主循环线程访问 | 不可跨线程，ThreadPool 无使用价值 |
| 帧管线 | `FrameTick` 单一入口（`src/core/TaskChain.hpp`），串行执行 | 无并行需求 |
| 图片解码 | `ImageManager::loadTexture`（`src/managers/ImageManager.cpp:61-125`）同步懒加载，`stbi_load` + `SDL_UploadToGPUTexture` 均需 GPU 上下文与主线程状态；调用方仅在渲染路径（`RenderFrame.cpp:332`） | 同步即可，无"必须异步"调用方 |
| 字体光栅化 | `TextTextureCache::getOrUpload`（`src/managers/TextTextureCache.cpp:46-145`）同步执行，产物体上传 GPU | 同上 |
| SVG 解码（未来项） | `docs/todo/SVG_SUPPORT_PLAN.md:236` 明确"**不在首阶段引入异步复杂度**：先保持当前同步懒加载行为"；`:263` 异步解码仅作远期准备 | 既有规划已排除首阶段异步 |

**跨线程通道已存在且够用**：`EventLoop::Post`（`src/utils/EventLoop.hpp`，MPSC 队列，任意线程可投递，单线程消费）。未来纯 CPU 模式允许做“计算/渲染分离并发”，但不恢复通用 ThreadPool：由专用、受控的 CPU 计算 worker 执行纯数据计算，完成后通过 `EventLoop::Post` 将结果交回主线程；主线程负责 ECS/UI 状态、渲染命令组织、GPU 资源创建与上传。

### 2.2 ThreadPool 当前是"负债"而非"能力"

- **零调用点**：全 `src/` 无一处 `Submit()`（唯一命中是 `api/Text.cpp:61 SetOnSubmit` UI 回调，与本线程池无关）；`UiRuntime::m_threadPool` 私有无访问器，不可达。
- **宏名不副实**：`UI_ENABLE_MULTITHREAD` 定义后（`src/CMakeLists.txt:518-523`）实际被 `ThreadPool.hpp` 内部的 `#if UI_ENABLE_MULTITHREAD` 消费（评审计为"无人消费"不精确，但**可观测行为**不变——零调用点意味着开关 ON 只会 spawn 空转线程）；开关 ON 还叠加 S3 TLS 雷区（worker 线程 `s_current==nullptr`）。
- **无测试**：`tests/unittest/CMakeLists.txt:44` 引用 `test_ThreadPool.cpp`，**该文件不存在** → `ENABLE_BUILD_TESTS=ON` 时 CMake configure 直接失败（悬空引用）。
- **无 benchmark**：`tests/benchmark/` 空目录，未接入任何 CMake。
- **已背债**：当前工作区 ThreadPool 处于"持锁 notify + MpmcQueue 无锁"的矛盾状态（P0-A 修复引入持锁通知，违背无锁承诺，且无任何收益场景）。

**判定：100% 负债。**

### 2.3 移除影响面：完全隔离，零波及

- 唯一消费者：`src/core/UiRuntime.hpp`（include 第 18 行 + `m_threadPool` 成员 + 构造初始化）。
- Include 图（已实测）：
  - `MpmcQueue.hpp` ← 仅 `ThreadPool.hpp`
  - `WorkStealingDeque.hpp` ← 仅 `ThreadPool.hpp`
  - `Singleton.hpp` ← **无任何 include**
  - `MpscQueue.hpp` ← `EventLoop.hpp`（**活代码，保留**）
- `include/ui/`（公开 API）：**零引用** ThreadPool/队列/宏 → 公共边界不受影响。
- `example/`：零引用。
- 测试：仅 `tests/unittest/CMakeLists.txt:44` 一行悬空引用（本就指向不存在的文件）。

### 2.4 移除 vs 保留量化对比

| 维度 | 移除（推荐） | 保留 + 修复 |
|---|---|---|
| 成本 | ~9 个修改点，全部为删除；`OFF` 模式下 Submit 本就走内联直通，**删除后行为零变化** | 无锁 lost-wakeup 重写（atomic wait/EventCount，高风险）、补单测、补 benchmark、定线程契约、治理 S3 雷区——投入大 |
| 风险 | 近零；顺带修复 `ENABLE_BUILD_TESTS` 悬空引用 | 无锁通知修复本身易引入新 bug；激活后 entt 非线程安全 + TLS 空指针叠加 |
| 收益 | 消除 ~1000 行无人消费并发代码的维护/审查/心智负担；消除误导性开关 | **零**（无调用点，无真实场景收益） |
| 未来演进 | P2-1 用 `EventLoop::Post` + 纯计算 worker，路径更简单且正确 | 与 P2-1 能力重叠，冗余 |

**量化结论：保留的每一分投入都无收益，移除的每一分成本都只是删除。**

## 3. 移除清单（供代码工厂执行）

### 3.1 删除文件（4 个 + 1 个空目录）

| # | 文件 | 依据 |
|---|---|---|
| 1 | `src/utils/ThreadPool.hpp` | 目标本体 |
| 2 | `src/utils/MpmcQueue.hpp` | 仅被 ThreadPool.hpp 引用 |
| 3 | `src/utils/WorkStealingDeque.hpp` | 仅被 ThreadPool.hpp 引用 |
| 4 | `src/utils/Singleton.hpp` | 独立死代码（零 include），独立评估后建议一并移除（与评审 M8 一致） |
| 5 | `tests/benchmark/`（空目录） | 未接入 CMake，纯占位 |

**保留**：`src/utils/MpscQueue.hpp`（EventLoop 使用，禁止删除）。

### 3.2 修改点（源码 3 处 + CMake 4 处）

**`src/core/UiRuntime.hpp` — 3 处**
1. 第 18 行：删除 `#include "utils/ThreadPool.hpp"`
2. 构造初始化列表（第 30 行）：删除 `m_threadPool(std::make_unique<utils::ThreadPool>()),`
3. 私有成员（第 86 行）：删除 `std::unique_ptr<utils::ThreadPool> m_threadPool;`

**`src/CMakeLists.txt` — 3 处**
1. 第 18 行：删除 `option(UI_ENABLE_MULTITHREAD ...)`
2. 第 211 行（UI_HEADERS 列表）：删除 `utils/ThreadPool.hpp`
3. 第 518-523 行：删除整个 `if(UI_ENABLE_MULTITHREAD) ... endif()` 块（含两条 message）

**`tests/unittest/CMakeLists.txt` — 1 处**
1. 第 44 行：删除 `test_ThreadPool.cpp`（**悬空引用，必须删，否则 `ENABLE_BUILD_TESTS=ON` configure 失败**）

### 3.3 文档受影响点

| 文档 | 处理 |
|---|---|
| `.github/copilot-instructions.md` | 当前**无** ThreadPool/多线程提及（已实测），无修改点；其 ASIO 过时声明属 P0-F 范围，不随本次 |
| `THREAD_SAFETY_CONTRACT.md`（P0-C 待办） | 移除后范围收窄为"EventLoop 线程边界 + 主线程独占 UI 状态"，**不写 ThreadPool 章节** |
| `docs/pm/run-20260809-P0-threadpool-retain-hardening.md` | 状态需在流程内更新：P0-A → 取消、P0-G → 取消/改向（见 §4） |
| 2026-08-09 原始评审文档 | 当前未保留；最终 ThreadPool 决策以本文为准 |

## 4. 与既有工作包的关系

| 工作包 | 判定 | 说明 |
|---|---|---|
| **P0-A**（ThreadPool M1 修复 + option 生效） | **取消** | 持锁 notify 问题随文件删除自然消失，无需返工；已做的"持锁 notify"修改一并回滚删除 |
| **P0-B**（EventLoop M1 修复） | **继续** | EventLoop 是唯一真实跨线程通道，`Post`/`WaitForWork` 同类 lost-wakeup 仍在（被 16ms 调度自愈掩盖），必须修复；**不得因 ThreadPool 移除而放松** |
| **P0-C**（线程安全契约文档） | **继续（收窄）** | 只覆盖 EventLoop 线程边界与主线程独占规则 |
| **P0-D**（`current()` 硬化） | **继续（可简化）** | ThreadPool worker 触发场景消失；工作区当前无 NullContext 代码（`current()` 仍为裸解引用，P0-D 未落地）——若方案含 NullContext 占位线程，其依赖可解除；SDL 事件线程/用户线程仍可能无 scope 调用 `current()`，硬化仍必要 |
| **P0-E**（退出期生命周期硬化） | **继续** | 与 ThreadPool 无关，独立成立 |
| **P0-F**（文档与守卫对齐） | **继续** | copilot-instructions.md 无 ThreadPool 提及，无额外改动 |
| **P0-G**（benchmark 基建） | **取消/改向** | 移除后无 benchmark 对象；空 `tests/benchmark/` 随本次清理。未来 P2-1 立项时再建 |

## 5. 遗留事项：CPU 计算/渲染分离并发（P2-1）

若未来出现真实 CPU 瓶颈（如图片解码、字体光栅化、SVG 解析或 CPU 布局准备）：

1. **不加 ThreadPool**：禁止恢复通用工作窃取池、全局线程池或隐式多线程开关。
2. **并发边界**：worker 只处理拥有明确所有权的纯 CPU 数据；禁止访问 `UiRuntime::current()`、`entt::registry`、Dispatcher、SDL GPU 对象和 renderer 状态。
3. **结果回传**：worker 产出不可变结果（如 RGBA、位图、字形轮廓、布局中间数据），通过 `EventLoop::Post` 回主线程；主线程验证任务代次和资源生命周期后，创建或更新 GPU 资源并写入 UI 状态。
4. **模式开关**：仅允许专用的 CPU 异步策略（候选名 `UI_ENABLE_CPU_ASYNC`），不恢复通用 `UI_ENABLE_MULTITHREAD`；默认保持同步语义。
5. **并发度**：首版固定为单个专用 worker 或调用方显式拥有的 `std::jthread`，不引入 work-stealing；只有基准证明单 worker 不足，才评估受控多 worker。
6. **触发门槛**：必须先有 profile/benchmark 证明同步 CPU 工作占用明确帧预算，并且异步版本在吞吐、P95 延迟或输入响应上有可重复收益；未复现瓶颈时只保留同步实现。

## 6. 遗留风险

1. **EventLoop 并发修改**：此前尝试通过 `submission_mutex_` 和条件变量通知修复 M1，但该方案使 EventLoop 提交/消费路径带锁，违背无锁路径约束；现已还原。**2026-08-16 已按方案 B（C++20 `atomic_wait/notify` 事件计数）修复 lost-wakeup**，保留无锁提交/通知路径，单任务 ping 20 轮无超时（详见 `EVENTLOOP_SYNC_FIX_REVIEW_2026-08-14.md`）。
2. **git 历史保留**：删除的文件仍可从历史恢复，未来不应以"历史里有"作为复活理由。
3. **P0-D 仍需落地**：`current()` 裸解引用 251 处调用点仍是 UB 扩散面，与 ThreadPool 无关。
4. **`ENABLE_BUILD_TESTS=ON` 恢复**：本次修复悬空引用后，测试构建应可恢复；建议移除后运行一次 `cmake -B build -DENABLE_BUILD_TESTS=ON` 验证。

## 6.1 用户修订决策（2026-08-10）

### 无锁实现优先，未经实测不得改动

- `src/utils/MpscQueue.hpp` 保持无锁实现。
- `src/utils/EventLoop.hpp` 的 `Post()`、队列消费和状态通知路径不得因“理论上的 lost-wakeup”直接引入互斥锁。
- 当前已还原 `submission_mutex_` 以及 `WakeUpOne()`/`WakeUpAll()` 的持锁通知改动，恢复原始无锁路径。
- 任何后续并发修复必须先提供可重复的实测证据：测试配置、线程数量、任务数量、超时阈值、失败次数和复现日志。
- 只有明确复现后，才进入方案评审；方案必须同时报告吞吐、延迟、CPU 占用和无锁语义影响。
- 未复现时只允许增加观测、压力测试和文档，不允许改变并发同步结构。

## 6.2 EventLoop 压测结论（2026-08-14）

P0-4 观测已获得可重复失败证据，允许进入同步方案评审，但本节不直接修改无锁实现。

| 场景 | 配置 | 结果 |
|---|---|---|
| 单任务 ping | 队列容量 64、1000 次、单消费者、单任务超时 200ms | 首次运行第 5 次迭代超时 1 次；重复运行第 36 次迭代超时 1 次 |
| 多生产者突发排空 | 4 个生产者、总计 100000 个任务、队列容量 4096、`Exit(drain=true)` | 通过，任务全部执行 |

判定：单任务 ping 的超时已在独立重复运行中再次出现，符合 lost-wakeup 候选的可重复性；多生产者入队和 drain 语义暂未发现丢任务。下一步应提交同步方案评审，比较 `condition_variable` 正确性修复与 C++20 `atomic_wait/notify`，并同时测量吞吐、P95 延迟、空闲 CPU 和无锁语义影响。在方案评审完成前，不直接修改 `Post`、消费或通知路径。

## 7. 待确认问题

1. `Singleton.hpp` 是否随本次一并删除？（建议是；它是独立死代码，评审 M8 已判定可删，与 ThreadPool 无耦合）
2. `docs/pm/run-20260809-P0-threadpool-retain-hardening.md` 的状态更新（P0-A 取消、P0-G 取消/改向）由谁在流程内落地？（本决策文档不修改既有文档正文）
