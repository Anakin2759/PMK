# EnTT 私有化与公开 API 边界修复规划

> 日期：2026-06-04  
> 背景：项目规则明确“不通过公开 API 暴露 EnTT”。当前 `EnTT::EnTT` 已恢复为 `ui` 的 `PRIVATE` 依赖，但 `include/ui.hpp` 的公开头链路仍间接包含 `entt/entt.hpp`，导致 `ui_public_header_check` 构建失败。

## 1. 当前问题

当前失败链路：

```text
include/ui.hpp
  -> src/common/Events.hpp
  -> src/common/EntityTypes.hpp
  -> #include <entt/entt.hpp>
```

核心冲突：

- `ui` 不应 `PUBLIC` 暴露 `EnTT::EnTT`。
- 但公开头 `EntityTypes.hpp` 仍依赖 EnTT。
- 因此公开头独立编译时找不到 `entt/entt.hpp`。

当前 `src/common/EntityTypes.hpp` 仍处于过渡态：

```cpp
#include <entt/entt.hpp>

namespace ui
{
using entity = entt::entity;
inline constexpr entity null_entity = entt::null;
}
```

这与“不暴露 EnTT”的边界规则冲突。

## 2. 修改目标

最终目标：

- `EnTT::EnTT` 保持 `PRIVATE`。
- `include/ui.hpp` 可独立编译，不需要 EnTT include path。
- `src/api/**/*.hpp` 不出现 `entt::entity`。
- `src/api/**/*.hpp` 不直接或间接要求下游用户具备 EnTT include path。
- 内部实现、系统层、`detail` 层仍可使用 `entt::entity`。
- 公开用户只接触 `ui::entity`。

## 3. 总体策略

采用“公开句柄 + 内部转换”的方式：

```text
公开 API / include/ui.hpp
  使用 ui::entity = std::uint32_t

内部实现 / src/detail / src/systems
  使用 entt::entity

边界转换
  ui::entity <-> entt::entity
```

## 4. 第一阶段：修复公开实体类型

### 4.1 修改 `src/common/EntityTypes.hpp`

目标：完全移除 EnTT 依赖。

建议改为：

```cpp
#pragma once

#include <cstdint>

namespace ui
{

using entity = std::uint32_t;
inline constexpr entity null_entity = 0xffffffffU;

} // namespace ui
```

注意：这里先不包含任何 EnTT 头。

### 4.2 新增 `src/detail/EntityCast.hpp`

职责：只在内部边界使用。

```cpp
#pragma once

#include <entt/entt.hpp>

#include "common/EntityTypes.hpp"

namespace ui::detail
{

static_assert(static_cast<ui::entity>(entt::null) == ui::null_entity);

[[nodiscard]] inline entt::entity ToInternal(ui::entity entity) noexcept
{
    return static_cast<entt::entity>(entity);
}

[[nodiscard]] inline ui::entity ToPublic(entt::entity entity) noexcept
{
    return static_cast<ui::entity>(entity);
}

} // namespace ui::detail
```

该文件允许包含 EnTT，但不得被 `include/ui.hpp` 或公开 API 头直接包含。

## 5. 第二阶段：修复公开事件类型

当前 `src/common/Events.hpp` 是公开头链路的一部分，但内部大量字段仍是 `entt::entity`。

短期建议：将公开事件结构中的实体字段统一改为 `ui::entity`。

示例：

```cpp
struct ClickEvent
{
    ui::entity entity = ui::null_entity;
};
```

不要在 `Events.hpp` 中包含 EnTT。

需要检查字段：

- `ClickEvent`
- `HoverEvent`
- `UnhoverEvent`
- `DragStartEvent`
- `DragMoveEvent`
- `DragEndEvent`
- `DragDroppedEvent`
- `MousePressEvent`
- `MouseReleaseEvent`
- `HitPointerMove`
- 其他含 `entt::entity` 的事件

内部系统收到这些事件后，在需要访问 `Registry` 时用 `ui::detail::ToInternal(event.entity)` 转换。

## 6. 第三阶段：API 头文件去 EnTT

检查范围：

```text
src/api/*.hpp
src/common/EntityTypes.hpp
src/common/Events.hpp
include/ui.hpp
```

目标：

- 公开头不包含 `<entt/entt.hpp>`。
- 公开头不包含 `entt/entity/*.hpp`。
- 公开函数参数/返回值使用 `ui::entity`。
- 链式 DSL 继续使用 `ui::entity`。

验证命令方向：

```text
搜索 src/api/**/*.hpp 是否出现 entt::entity
搜索 src/api/**/*.hpp 是否 include entt
搜索 include/ui.hpp 间接编译是否需要 EnTT include path
```

## 7. 第四阶段：API `.cpp` 边界转换

公开 API 的 `.cpp` 文件允许包含：

```cpp
#include "detail/EntityCast.hpp"
```

模式：

```cpp
void Show(ui::entity entity)
{
    detail::visibility::Show(detail::ToInternal(entity));
}
```

如果当前 `.cpp` 仍直接实现 ECS 逻辑，可以先不完全拆 `detail`，但函数签名应先对齐公开头。

## 8. 第五阶段：测试同步

### 8.1 API 测试

新增或增强：

- `test_UmbrellaHeader.cpp`：只包含 `ui.hpp`。
- 新增 `test_PublicHeadersNoEntt.cpp`：验证公开头无需 EnTT include path。
- 新增 `test_PublicEntityContract.cpp`：验证 `ui::entity` 是公开整数句柄，`ui::null_entity` 稳定。

### 8.2 内部测试

内部 ECS 测试可以继续包含 EnTT，但应直接链接 `EnTT::EnTT`，不依赖 `ui` 传播。

## 9. 推荐执行顺序

### P0：先恢复构建边界

1. 修改 `EntityTypes.hpp`，移除 EnTT。
2. 新增 `detail/EntityCast.hpp`。
3. 修改 `Events.hpp` 中实体字段为 `ui::entity`。
4. 修复因此产生的系统层编译错误，加转换。
5. 保持 `EnTT::EnTT` 为 `PRIVATE`。
6. 构建 `ui_public_header_check`。

### P1：公开 API 函数签名清理

1. 清理 `src/api/*.hpp` 的 `entt::entity`。
2. `.cpp` 中做 `ToInternal/ToPublic` 转换。
3. 修复 Chains DSL。
4. 跑 `ui_api_tests`。

### P2：内部 detail 分层

1. 将实际 ECS 操作逐步下沉到 `src/detail/*`。
2. `src/api/*.cpp` 变薄封装。
3. 清理 `api` 层对组件头、Registry 的直接依赖。

## 10. 风险点

### 10.1 事件字段类型变化影响面大

`Events.hpp` 被系统层广泛使用。把 `entt::entity` 改为 `ui::entity` 后，系统层需要显式转换。

风险控制：先只改事件字段，保持底层值不变，使用 `static_cast` 转换，不改变运行时语义。

### 10.2 `ui::entity` 与 `entt::entity` 值一致性

必须保留：

```cpp
static_assert(static_cast<ui::entity>(entt::null) == ui::null_entity);
```

否则空实体判断会出现隐蔽错误。

### 10.3 测试内外边界不同

`api` 测试不应包含 EnTT；`ecs` 测试可以包含 EnTT。

不要为了让 API 测试过，把 EnTT 又加回 `PUBLIC`。

## 11. 验收标准

完成后应满足：

- `EnTT::EnTT` 仍在 `ui PRIVATE`。
- `include/ui.hpp` 独立编译通过。
- `src/common/EntityTypes.hpp` 不包含 EnTT。
- `src/common/Events.hpp` 不包含 EnTT。
- `src/api/**/*.hpp` 不包含 EnTT。
- `ui_tests` 构建通过。
- `ctest -L "unit|ecs|api"` 通过。

## 12. 建议不要做的事

- 不要把 `EnTT::EnTT` 改回 `PUBLIC`。
- 不要把 `third_party/entt/single_include` 加回公开 include path。
- 不要在 `include/ui.hpp` 为了通过测试直接包含 EnTT。
- 不要让公开 `ui::entity` 继续 typedef 到 `entt::entity`。
