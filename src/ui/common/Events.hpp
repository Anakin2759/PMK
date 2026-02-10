/**
 * ************************************************************************
 *
 * @file Events.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Optimized)
 * @version 0.2
 * @brief UI ECS 事件定义
 *
 * 事件分为两类：
 * 1. 紧急事件 [IMMEDIATE] - 使用 Dispatcher::Trigger() 立即执行
 *    适用场景：需要即时响应的用户交互、渲染指令、退出请求等
 *
 * 2. 缓冲区事件 [BUFFERED] - 使用 Dispatcher::Enqueue() 加入队列
 *    适用场景：可以延迟到下一帧处理的事件，如输入事件、状态变更等
 *    在事件循环每帧开始时通过 dispatcher.update() 统一处理
 *
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <entt/entt.hpp>
#include <string>
#include "Types.hpp"
#ifdef CreateWindow
#undef CreateWindow
#endif
namespace ui::events
{

// =====================================================================
// A. 核心 ECS / 生命周期事件 (由 Application/Engine 触发)
// =====================================================================

/**
 * @brief 在 Application 完成底层初始化 (SDL/ECS根实体) 后触发
 * [BUFFERED] 使用 enqueue
 */
struct ApplicationReadyEvent
{
    using is_event_tag = void;
    entt::entity rootEntity;
};

/**
 * @brief 图形上下文设置事件
 * [BUFFERED] 使用 enqueue - 在下一帧开始时统一处理
 */
struct WindowGraphicsContextSetEvent
{
    using is_event_tag = void;
    entt::entity entity;
};

struct WindowGraphicsContextUnsetEvent
{
    using is_event_tag = void;
    entt::entity entity;
};

// =====================================================================
// C. 通用 UI 交互事件 (由 InteractionSystem 触发)
//    交互事件为 [IMMEDIATE] 类型，使用 trigger 立即响应用户操作
// =====================================================================

/**
 * @brief 请求退出事件
 * [IMMEDIATE] 使用 trigger - 需立即停止事件循环
 */
struct QuitRequested
{
    using is_event_tag = void;
};

/**
 * @brief 窗口尺寸变化事件
 * [IMMEDIATE] 使用 trigger
 */
struct WindowResized
{
    using UIFlag = void;
    using is_event_tag = void;
    int width;
    int height;
};

/**
 * @brief 窗口像素尺寸变化事件
 * [IMMEDIATE] 使用 trigger
 */
struct WindowPixelSizeChanged
{
    using is_event_tag = void;
    uint32_t windowID;
    int width;
    int height;
};

/**
 * @brief 窗口位置变化事件
 * [IMMEDIATE] 使用 trigger
 */
struct WindowMoved
{
    using is_event_tag = void;
    uint32_t windowID;
    int x;
    int y;
};

/**
 * @brief 点击事件 - 鼠标按下并在同一实体上释放
 * [IMMEDIATE] 使用 trigger
 */
struct ClickEvent
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 取消悬浮事件
 * [IMMEDIATE] 使用 trigger
 */
struct UnhoverEvent
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 悬浮事件
 * [IMMEDIATE] 使用 trigger
 */
struct HoverEvent
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 鼠标按下事件
 * [IMMEDIATE] 使用 trigger
 */
struct MousePressEvent
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 鼠标释放事件
 * [IMMEDIATE] 使用 trigger
 */
struct MouseReleaseEvent
{
    using is_event_tag = void;
    entt::entity entity;
};
/**
 * @brief 文本内容改变事件 (TextEdit/Input)
 * [BUFFERED] 使用 enqueue
 */
struct ValueChangedText
{
    using is_event_tag = void;
    entt::entity entity;
    std::string newText;
};

/**
 * @brief 选择索引改变事件 (Dropdown/List)
 * [BUFFERED] 使用 enqueue
 */
struct ValueChangedSelection
{
    using is_event_tag = void;
    entt::entity entity;
    int selectedIndex;
};

/**
 * @brief 发送处理函数到事件循环
 * [BUFFERED] 使用 enqueue - 在事件循环中执行回调
 */
struct SendHandlerToEventLoop
{
    using is_event_tag = void;
    std::move_only_function<void()> handler;
};

/**
 * @brief 通用更新事件
 * [BUFFERED] 使用 enqueue
 */
struct UpdateEvent
{
    using is_event_tag = void;
};

struct CreateWindow
{
    using is_event_tag = void;
    std::string title;
    std::string alias;
};
/**
 * @brief 关闭窗口事件
 * [IMMEDIATE] 使用 trigger
 */
struct CloseWindow
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 渲染更新事件 - 每帧渲染时触发
 * [IMMEDIATE] 使用 trigger - 需立即执行渲染
 */
struct UpdateRendering
{
    using is_event_tag = void;
};

/**
 * @brief 布局更新事件 - 每帧渲染前触发
 * [IMMEDIATE] 使用 trigger - 需在渲染前立即完成布局
 */
struct UpdateLayout
{
    using is_event_tag = void;
};

/**
 * @brief 帧结束事件 - 每帧渲染后触发
 * [IMMEDIATE] 使用 trigger - 用于批量应用状态更新
 */
struct EndFrame
{
    using is_event_tag = void;
};

struct UpdateTimer
{
    using is_event_tag = void;
};

struct QueuedTask
{
    using is_event_tag = void;
    std::move_only_function<void()> func;
    uint32_t intervalMs = 0; // 延迟执行时间
    uint32_t remainingMs = 0;
    bool singleShoot = false;
    uint8_t frameSlot = 0;
    bool quitAfterExecute = false;
};

// =====================================================================

// 原始指针移动事件（由底层输入系统转发）
// [BUFFERED] 使用 enqueue — 由输入系统记录原始位置/相对位移并转发
struct RawPointerMove
{
    using is_event_tag = void;
    Vec2 position;
    Vec2 delta; // 相对位移
    uint32_t windowID;
};

// 原始指针按键事件（按下/抬起）
// [BUFFERED] 使用 enqueue
struct RawPointerButton
{
    using is_event_tag = void;
    Vec2 position;
    uint32_t windowID;
    bool pressed;   // true = down, false = up
    uint8_t button; // SDL_BUTTON_LEFT, etc.
};

// 原始滚轮事件
// [BUFFERED] 使用 enqueue
struct RawPointerWheel
{
    using is_event_tag = void;
    Vec2 position;     // 鼠标位置（采样时）
    Vec2 delta;        // 滚轮增量 (x, y)
    uint32_t windowID; // Added windowID to match definition in InteractionSystem usage
};

// =====================================================================
// D. 命中测试后的中间事件 (由 HitTestSystem 触发)
//    包含原始数据 + 命中的实体信息，供StateSystem消费
// =====================================================================

// 命中测试后的指针移动
// [BUFFERED] 使用 enqueue
struct HitPointerMove
{
    using is_event_tag = void;
    RawPointerMove raw;
    entt::entity hitEntity; // 鼠标当前悬停的实体 (可能为 null)
};

// 命中测试后的按键事件
// [BUFFERED] 使用 enqueue
struct HitPointerButton
{
    using is_event_tag = void;
    RawPointerButton raw;
    entt::entity hitEntity; // 按键发生位置的实体 (可能为 null)
};

// 命中测试后的滚轮事件
// [BUFFERED] 使用 enqueue
struct HitPointerWheel
{
    using is_event_tag = void;
    RawPointerWheel raw;
    entt::entity hitEntity; // 滚轮发生位置的实体 (可能为 null)
};

} // namespace ui::events