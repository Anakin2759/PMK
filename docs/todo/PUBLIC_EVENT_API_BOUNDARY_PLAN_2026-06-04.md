# 公开事件 API 与内部 ECS 事件隔离改造规划

> 状态：DONE（基础公开事件 API 边界）  
> 最后复核：2026-08-27  
> 责任范围：公开事件 API、EventBridge、Runtime 生命周期  
> 说明：基础边界、Runtime 归属和连接生命周期已完成；未来若开放跨线程注册/派发，另建同步协议规划。

> 日期：2026-06-04  
> 背景：`src/common/Events.hpp` 属于内部 ECS 事件定义，不作为对外 API 暴露；对外实体句柄 `ui::entity` 固定为 `uint32_t`。需要在 `src/api/` 新增公开事件 API 文件，支持自定义事件注册、事件回调注册、触发事件，并严格隔离公开 API 与内部 EnTT / Dispatcher 实现。

## 1. 当前边界判断

### 1.1 已确认原则

- `src/common/Events.hpp`：内部事件定义，仅供 `systems`、`core`、内部实现使用。
- `src/common/EntityTypes.hpp`：公开实体句柄定义，`ui::entity = uint32_t`。
- `EnTT::EnTT`：必须保持 `ui` 的 `PRIVATE` 依赖，不通过 `ui` 目标向外传播。
- `include/ui.hpp`：不应要求外部用户具备 EnTT include path。
- `src/api/`：公开 API 层，不得暴露 `entt::entity`、`entt::dispatcher`、`entt::sink` 等类型。

### 1.2 当前问题

当前 `src/common/Events.hpp` 仍大量使用 `entt::entity`。但用户明确该文件不对外暴露，因此无需把它强行改成纯公开事件头。

更合理的方向是：

```text
内部 ECS 事件层
  src/common/Events.hpp
  使用 entt::entity，面向系统和运行时

公开事件 API 层
  src/api/Event.hpp / Event.cpp
  使用 ui::entity / uint32_t / 公开 payload，面向用户

桥接层
  src/systems/EventSystem.hpp/.cpp 或 src/detail/EventBridge.hpp/.cpp
  负责公开事件与内部 Dispatcher 的转换、分发和生命周期管理
```

## 2. 改造目标

完成后应满足：

- 外部用户可以注册自定义事件类型。
- 外部用户可以注册事件回调。
- 外部用户可以触发事件。
- 外部用户可以选择立即触发或入队延迟触发。
- 外部事件 payload 不暴露 EnTT。
- `src/common/Events.hpp` 不再被 `include/ui.hpp` 直接导出。
- 内部 `systems` 仍可继续使用 `src/common/Events.hpp`。
- 如需要跨运行时隔离，事件注册和回调绑定到当前 `UiRuntimeScope`。

## 3. 推荐文件结构

```text
src/api/
  Event.hpp          // 公开事件 API 声明
  Event.cpp          // 公开 API 实现，转发到 detail/EventBridge 或 EventSystem

src/detail/
  EntityCast.hpp     // ui::entity <-> entt::entity
  EventBridge.hpp    // 公开事件桥接内部实现声明
  EventBridge.cpp    // 公开事件桥接内部实现

src/systems/
  EventSystem.hpp    // 可选：运行时级自定义事件系统
  EventSystem.cpp    // 可选：统一管理连接、队列、生命周期
```

若短期想少动系统层，先实现 `detail::EventBridge` 即可；当自定义事件需要运行时生命周期、批量清理、跨线程投递时，再提升为 `EventSystem`。

## 4. 公开 API 设计

### 4.1 公开事件 ID

建议定义：

```cpp
namespace ui::event
{

using EventId = std::uint32_t;
inline constexpr EventId INVALID_EVENT_ID = 0;

} // namespace ui::event
```

事件 ID 来源有两种：

1. 用户显式传入字符串名，内部 hash/注册为 ID。
2. 用户直接传入数值 ID。

推荐优先字符串名：

```cpp
EventId RegisterEvent(std::string_view name);
```

原因：可读性更好，便于日志和调试。

### 4.2 公开事件 payload

第一版建议使用稳定、简单的公开结构：

```cpp
struct EventPayload
{
    ui::entity source = ui::null_entity;
    ui::entity target = ui::null_entity;
    std::string name;
    std::string text;
    std::int64_t intValue = 0;
    double floatValue = 0.0;
};
```

优点：

- 不暴露模板复杂度。
- 不暴露 EnTT。
- ABI/头文件依赖简单。
- 足够覆盖 UI 层常见自定义事件。

后续可扩展 `std::variant` 或 typed event，但第一版不建议一上来做过度泛型。

### 4.3 公开回调类型

```cpp
using EventCallback = std::move_only_function<void(const EventPayload&)>;
```

如果需要可复制回调，也可用 `std::function`。项目已有 C++23，可优先 `std::move_only_function`。

### 4.4 连接句柄

公开 API 不应返回 `entt::scoped_connection`。

建议定义轻量 RAII 句柄：

```cpp
class EventConnection
{
public:
    EventConnection() noexcept = default;
    EventConnection(EventConnection&&) noexcept;
    EventConnection& operator=(EventConnection&&) noexcept;
    ~EventConnection();

    void Disconnect() noexcept;
    [[nodiscard]] bool Connected() const noexcept;

private:
    std::uint64_t m_token = 0;
};
```

内部 token 由 `EventBridge/EventSystem` 管理。

### 4.5 公开 API 函数

建议第一版提供：

```cpp
namespace ui::event
{

EventId RegisterEvent(std::string_view name);
bool IsEventRegistered(EventId id);
bool IsEventRegistered(std::string_view name);

EventConnection On(EventId id, EventCallback callback);
EventConnection On(std::string_view name, EventCallback callback);

void Trigger(EventId id, EventPayload payload = {});
void Trigger(std::string_view name, EventPayload payload = {});

void Enqueue(EventId id, EventPayload payload = {});
void Enqueue(std::string_view name, EventPayload payload = {});

void DispatchQueued(); // 可选；通常由 runtime 每帧调度

} // namespace ui::event
```

命名与项目公开 API 风格保持 PascalCase。

## 5. 内部事件桥接设计

### 5.1 内部自定义事件类型

在 `src/detail/EventBridge.hpp` 或 `src/common/InternalEvents.hpp` 中定义内部事件：

```cpp
namespace ui::detail::events
{

struct CustomEvent
{
    using is_event_tag = void;
    ui::event::EventId id = ui::event::INVALID_EVENT_ID;
    ui::event::EventPayload payload;
};

} // namespace ui::detail::events
```

注意：该内部事件不必放入 `src/common/Events.hpp`。这样可避免让内部 ECS 主事件文件继续变大。

### 5.2 立即触发

`ui::event::Trigger()` 内部可以：

```cpp
RuntimeFacade::current().trigger(detail::events::CustomEvent{id, std::move(payload)});
```

但实际回调表不一定要走 EnTT sink。更直接的做法是 `EventBridge` 自己维护回调表并同步调用。

建议第一版：公开自定义事件由 `EventBridge` 自己管理，不混入 EnTT dispatcher 的类型系统。

原因：

- 自定义事件是动态 ID，EnTT dispatcher 更适合静态 C++ 类型事件。
- 动态事件若强塞 EnTT，只能变成一个 `CustomEvent` 类型，再自己按 ID 分发。
- 既然最终仍要按 ID 分发，不如直接在桥接层做。

### 5.3 队列触发

`EventBridge` 持有队列：

```cpp
struct QueuedCustomEvent
{
    EventId id;
    EventPayload payload;
};
```

`Enqueue()` 放入当前 runtime 的事件队列；`DispatchQueued()` 在帧开始或指定阶段派发。

## 6. 是否需要 EventSystem

### 6.1 不需要 EventSystem 的短期方案

如果只需要用户手动调用：

```cpp
ui::event::DispatchQueued();
```

则可以只做 `detail::EventBridge`，不新增系统。

优点：改动小。

缺点：队列派发点不统一，用户可能忘记调用。

### 6.2 推荐中期方案：新增 EventSystem

新增 `systems::EventSystem`，注册到 `SystemManager` 的 `LOGIC` 或 `FRAME` 阶段。

职责：

- 管理当前 runtime 的自定义事件注册表。
- 管理 callback token 生命周期。
- 在每帧固定阶段派发 queued 自定义事件。
- 提供运行时隔离：不同 `UiRuntimeScope` 的事件回调互不串线。

建议阶段：

- `LOGIC`：适合业务逻辑事件。
- `FRAME`：适合帧尾通知。

第一版建议放在 `LOGIC`，并在 `UpdateEvent` 后或 `QueuedTask` 后派发。

## 7. 运行时存储设计

避免全局静态表跨 runtime 污染。

建议在 `UiRuntime` 或 `Registry::ctx()` 中挂上下文：

```cpp
namespace ui::event::detail
{

struct EventRegistryContext
{
    std::unordered_map<std::string, EventId> idsByName;
    std::unordered_map<EventId, std::string> namesById;
    std::unordered_map<EventId, std::vector<CallbackSlot>> callbacks;
    std::vector<QueuedCustomEvent> queue;
    std::uint64_t nextToken = 1;
    EventId nextEventId = 1;
};

} // namespace ui::event::detail
```

通过 `RuntimeFacade::current().ensureContext<EventRegistryContext>()` 获取。

这样能自然支持多 runtime 隔离。

## 8. Entity 转换规则

公开 payload 中所有实体字段都是 `ui::entity`。

如果内部系统需要使用实体访问 Registry：

```cpp
entt::entity internal = ui::detail::ToInternal(payload.source);
```

禁止在公开事件 API 中出现：

- `entt::entity`
- `entt::dispatcher`
- `entt::sink`
- `entt::scoped_connection`

## 9. `include/ui.hpp` 调整

当前 `include/ui.hpp` 不应导出 `src/common/Events.hpp`。

建议改为：

```cpp
#include "../src/api/Event.hpp"
```

并移除：

```cpp
#include "../src/common/Events.hpp"
```

如果存在用户依赖内部事件类型，这属于破坏边界的历史用法，应迁移到 `ui::event` 公开 API。

## 10. 分阶段执行计划

### 阶段 A：公开头边界止血

目标：让 `include/ui.hpp` 不再间接依赖 EnTT。

- [ ] 确认 `EntityTypes.hpp` 为 `uint32_t`，不含 EnTT。
- [ ] 从 `include/ui.hpp` 移除 `src/common/Events.hpp`。
- [ ] 新增 `src/api/Event.hpp`，只暴露公开事件 API。
- [ ] 在 `include/ui.hpp` 中加入 `src/api/Event.hpp`。
- [ ] 保持 `src/common/Events.hpp` 仅内部使用。
- [ ] 构建 `ui_public_header_check`。

### 阶段 B：实现基础自定义事件 API

- [ ] 新增 `src/api/Event.cpp`。
- [ ] 新增 `src/detail/EventBridge.hpp/.cpp`。
- [ ] 实现 `RegisterEvent()`。
- [ ] 实现 `On()` 返回 `EventConnection`。
- [ ] 实现 `Trigger()` 同步调用回调。
- [ ] 实现 `Enqueue()` 与 `DispatchQueued()`。
- [ ] 使用 `RuntimeFacade::current().ensureContext<EventRegistryContext>()` 做 runtime 隔离。

### 阶段 C：接入 EventSystem（建议）

- [ ] 新增 `src/systems/EventSystem.hpp/.cpp`。
- [ ] 在 `SystemManager` 中注册 `EventSystem`。
- [ ] `EventSystem` 订阅 `UpdateEvent` 或专用帧阶段事件。
- [ ] 每帧调用 `DispatchQueued()`。
- [ ] 增加重复注册/注销与 runtime 隔离测试。

### 阶段 D：测试补齐

- [ ] 新增 `tests/unittest/test_PublicEventApi.cpp`。
- [ ] 测试注册事件名得到稳定 ID。
- [ ] 测试 `On()` 回调能收到 payload。
- [ ] 测试 `EventConnection::Disconnect()` 后不再触发。
- [ ] 测试 `Enqueue()` 不立即触发，`DispatchQueued()` 后触发。
- [ ] 测试不同 `UiRuntimeScope` 下回调隔离。
- [ ] 测试 `include/ui.hpp` 不需要 EnTT include path。

## 11. CMake 调整

需要加入：

```text
src/api/Event.cpp
src/api/Event.hpp
src/detail/EventBridge.cpp
src/detail/EventBridge.hpp
```

若新增 `EventSystem`，还需加入：

```text
src/systems/EventSystem.cpp
src/systems/EventSystem.hpp
```

`EnTT::EnTT` 继续保持：

```cmake
target_link_libraries(ui PRIVATE EnTT::EnTT)
```

不要把 EnTT 放回 `PUBLIC`。

## 12. API 示例

目标用法：

```cpp
auto id = ui::event::RegisterEvent("login.completed");

auto connection = ui::event::On(id, [](const ui::event::EventPayload& payload) {
    // payload.source 是 ui::entity / uint32_t
});

ui::event::Trigger(id, {.source = buttonEntity, .text = "ok"});
ui::event::Enqueue("login.completed", {.source = buttonEntity});
```

## 13. 验收标准

- `include/ui.hpp` 不包含 `src/common/Events.hpp`。
- `include/ui.hpp` 独立编译不需要 EnTT include path。
- `src/api/Event.hpp` 不包含 EnTT。
- `ui::entity` 对外是 `uint32_t`。
- 自定义事件支持注册、注册回调、触发、入队触发、断开回调。
- 多 `UiRuntimeScope` 下事件表和回调不串线。
- `EnTT::EnTT` 保持 `PRIVATE`。
- `ui_tests` 构建通过。
- `ctest -L "unit|ecs|api"` 通过。

## 14. 不做事项

- 不把 `src/common/Events.hpp` 作为公开 API 导出。
- 不在 `src/api/Event.hpp` 中包含 EnTT。
- 不返回 `entt::connection` / `entt::scoped_connection`。
- 不用 `void*` 暴露内部事件对象。
- 不把动态自定义事件强行建模为大量 EnTT 静态事件类型。
