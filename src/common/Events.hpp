/**
 * ************************************************************************
 *
 * @file Events.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Optimized)
 * @version 0.2
 * @brief UI ECS 事件定义
 *
 * 当前事件语义分为两类：
 * 1. 立即事件 [IMMEDIATE]
 *    - 通过 Dispatcher::Trigger() 立即执行
 *    - 主要用于常规帧阶段事件（UpdateLayout / UpdateRendering / EndFrame / UpdateTimer）
 *      以及必须即时响应的交互或平台事件
 *
 * 2. 队列事件 [BUFFERED]
 *    - 通过 Dispatcher::Enqueue() 进入队列
 *    - 类型和派发顺序登记在 BufferedEvents.hpp
 *    - 由 FrameTick 在 PollInput 后的内部队列阶段逐类型统一派发
 *    - 主要用于原始输入事件和可延迟到下一帧处理的状态事件
 *
 * 额外说明：
 * - FrameTick 按固定阶段触发 UpdateLayout / UpdateRendering / EndFrame
 * - UpdateLayout / UpdateRendering 的即时补救白名单为空
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
 * @brief 在 Application 完成底层初始化 (SDL/ECS根实体) 后触发（当前未接入生产管线）
 */
struct ApplicationReadyEvent
{
    using is_event_tag = void;  // 事件标签
    entt::entity rootEntity;    // 根实体，包含全局上下文组件
};

/**
 * @brief 图形上下文设置事件
 * 当前主要由 Factory 在窗口创建后立即 Trigger，确保 RenderSystem 能同步认领窗口图形上下文。
 * 因此这里更适合理解为“生命周期事件”，而不是按输入事件那样依赖固定的队列派发。
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
//    交互抽象事件当前按 [IMMEDIATE] 使用，目的是在当前帧内立即推进状态机
// =====================================================================

/**
 * @brief 请求退出事件
 * [BUFFERED] SDL 退出请求进入内部队列；显式退出 API 可立即 trigger
 */
struct QuitRequested
{
    using is_event_tag = void;
};

/**
 * @brief 关闭 DropDown 弹出层请求事件
 * [IMMEDIATE] 使用 trigger - 由 StateSystem 触发，Factory 层处理
 */
struct DropDownCloseRequested
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 打开浮层请求事件
 * [IMMEDIATE] 使用 trigger - 由 Factory/控件触发，OverlaySystem 统一分配 z-order 并入栈
 */
struct OverlayOpenRequest
{
    using is_event_tag = void;
    entt::entity entity = entt::null;  // 浮层根实体
    entt::entity owner = entt::null;   // 触发者（关闭时焦点恢复到它）
};

/**
 * @brief 关闭浮层请求事件
 * [IMMEDIATE] 使用 trigger - 由 OverlaySystem 或控件触发，统一出栈并恢复焦点
 */
struct OverlayCloseRequest
{
    using is_event_tag = void;
    entt::entity entity = entt::null;  // 浮层根实体
};

/**
 * @brief 关闭所有浮层请求事件
 * [IMMEDIATE] 使用 trigger - 从栈顶到栈底依次关闭
 */
struct OverlayCloseAllRequest
{
    using is_event_tag = void;
};

/**
 * @brief 窗口尺寸变化事件
 * [BUFFERED] 由 PlatformWindowSystem enqueue
 */
struct WindowResized
{
    using UIFlag = void;
    using is_event_tag = void;
    int width;
    int height;
};

enum class WindowMetricChangeSource : uint8_t
{
    PIXEL_SIZE_CHANGED,
    RESIZED,
    EXPOSED,
    DISPLAY_SCALE_CHANGED,
};

/**
 * @brief 窗口像素尺寸变化事件
 * [BUFFERED] 由 PlatformWindowSystem enqueue
 */
struct WindowPixelSizeChanged
{
    using is_event_tag = void;
    uint32_t windowID = 0;
    int width = 0;
    int height = 0;
    WindowMetricChangeSource source = WindowMetricChangeSource::PIXEL_SIZE_CHANGED;
};

/**
 * @brief 窗口暴露/重绘请求事件
 * [BUFFERED] 使用 enqueue
 */
struct WindowExposed
{
    using is_event_tag = void;
    uint32_t windowID = 0;
};

/**
 * @brief 窗口位置变化事件
 * [BUFFERED] 由 StateSystem enqueue
 */
struct WindowMoved
{
    using is_event_tag = void;
    uint32_t windowID;
    int x;
    int y;
};

/**
 * @brief 窗口显示缩放变化事件
 * [BUFFERED] 由 StateSystem enqueue
 */
struct WindowDisplayScaleChanged
{
    using is_event_tag = void;
    uint32_t windowID = 0;
    float oldScale = 1.0F;
    float newScale = 1.0F;
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

struct TableCellClicked
{
    using is_event_tag = void;
    entt::entity table;
    int row;
    int col;
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
 * @brief 拖拽开始事件
 * [IMMEDIATE] 使用 trigger
 */
struct DragStartEvent
{
    using is_event_tag = void;
    entt::entity source = entt::null;
};

/**
 * @brief 拖拽移动事件
 * [IMMEDIATE] 使用 trigger
 */
struct DragMoveEvent
{
    using is_event_tag = void;
    entt::entity source = entt::null;
    Vec2 delta;
    entt::entity hoverTarget = entt::null;
};

/**
 * @brief 拖拽结束事件
 * [IMMEDIATE] 使用 trigger
 */
struct DragEndEvent
{
    using is_event_tag = void;
    entt::entity source = entt::null;
    entt::entity dropTarget = entt::null;
};

/**
 * @brief 拖拽放置事件
 * [IMMEDIATE] 使用 trigger
 */
struct DragDroppedEvent
{
    using is_event_tag = void;
    entt::entity source = entt::null;
    entt::entity target = entt::null;
};
/**
 * @brief 文本内容改变事件 (TextEdit/Input，当前未接入生产管线)
 */
struct ValueChangedText
{
    using is_event_tag = void;
    entt::entity entity;
    std::string newText;
};

/**
 * @brief 选择索引改变事件 (Dropdown/List，当前未接入生产管线)
 */
struct ValueChangedSelection
{
    using is_event_tag = void;
    entt::entity entity;
    int selectedIndex;
};

/**
 * @brief 发送处理函数到事件循环（遗留声明，当前未接入生产管线）
 */
struct SendHandlerToEventLoop
{
    using is_event_tag = void;
    VoidCallback handler;
};

/**
 * @brief 通用更新事件
 * [IMMEDIATE] 由 FrameTick 在 Logic 阶段 trigger
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
 * [BUFFERED] 使用 enqueue
 */
struct CloseWindow
{
    using is_event_tag = void;
    entt::entity entity;
};

/**
 * @brief 渲染更新事件
 * [IMMEDIATE] 由 FrameTick 在每帧 Render 阶段唯一触发
 */
struct UpdateRendering
{
    using is_event_tag = void;
};

/**
 * @brief 布局更新事件
 * [IMMEDIATE] 由 FrameTick 在每帧 Layout 阶段唯一触发
 */
struct UpdateLayout
{
    using is_event_tag = void;
};

/**
 * @brief 帧结束事件
 * [IMMEDIATE] 由 FrameTick 在固定帧末尾触发，用于批量应用状态更新
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
    VoidCallback func;
    uint32_t intervalMs = 0;  // 延迟执行时间
    uint32_t remainingMs = 0;
    bool singleShoot = false;
    uint8_t frameSlot = 0;
    bool quitAfterExecute = false;
};

// =====================================================================

// 原始指针移动事件（由底层输入系统转发）
// [BUFFERED] 由 InteractionSystem 记录原始位置/相对位移后入队，
// 再由 FrameTick 的内部队列阶段统一派发到命中测试和状态系统
struct RawPointerMove
{
    using is_event_tag = void;
    Vec2 position;  // SDL window coordinates; HitTest consumes this same space.
    Vec2 delta;     // 相对位移
    uint32_t windowID;
};

// 原始指针按键事件（按下/抬起）
// [BUFFERED] 由 InteractionSystem 入队，避免在 SDL 轮询点直接推进后续状态链
struct RawPointerButton
{
    using is_event_tag = void;
    Vec2 position;  // SDL window coordinates
    uint32_t windowID;
    bool pressed;    // true = down, false = up
    uint8_t button;  // SDL_BUTTON_LEFT, etc.
};

// 原始滚轮事件
// [BUFFERED] 由 InteractionSystem 入队，统一进入下一帧输入处理阶段
struct RawPointerWheel
{
    using is_event_tag = void;
    Vec2 position;      // 鼠标位置（SDL window coordinates）
    Vec2 delta;         // 滚轮增量 (x, y)
    uint32_t windowID;  // Added windowID to match definition in InteractionSystem usage
};

// 原始文本输入事件
// [IMMEDIATE] 由 InteractionSystem 在输入泵阶段直接触发，保证文本编辑仍保持当前帧可见反馈
struct RawTextInput
{
    using is_event_tag = void;
    std::string text;
};

// 原始键盘事件
// [IMMEDIATE] 由 InteractionSystem 在输入泵阶段直接触发，供 TextInputSystem 独立处理编辑命令与重复按键策略
struct RawKeyInput
{
    using is_event_tag = void;
    int32_t key;
    bool pressed;
    bool repeat;
    uint16_t modifiers;
};

// 焦点切换请求
// [IMMEDIATE] 由 FocusNavigationSystem 计算目标后触发，StateSystem 订阅并复用 setFocus/clearFocus 逻辑
struct FocusChangeRequest
{
    using is_event_tag = void;
    entt::entity target = entt::null;  // 目标实体；entt::null 表示清除当前焦点
};

// =====================================================================
// D. 命中测试后的中间事件 (由 HitTestSystem 触发)
//    包含原始数据 + 命中的实体信息，供StateSystem消费
//    这里保持为稳定外部事件契约；StateSystem 内部如何拆分 helper 不影响这些结构
// =====================================================================

// 命中测试后的指针移动
// [BUFFERED] 由 HitTestSystem 入队，供 StateSystem / ActionSystem 在队列阶段消费
struct HitPointerMove
{
    using is_event_tag = void;
    RawPointerMove raw{};
    entt::entity hitEntity = entt::null;  // 鼠标当前悬停的实体 (可能为 null)
};

// 命中测试后的按键事件
// [BUFFERED] 由 HitTestSystem 入队，避免命中测试与状态修改在同一调用栈中耦合过深
struct HitPointerButton
{
    using is_event_tag = void;
    RawPointerButton raw{};
    entt::entity hitEntity = entt::null;  // 按键发生位置的实体 (可能为 null)
};

// 命中测试后的滚轮事件
// [BUFFERED] 由 HitTestSystem 入队，供后续滚动和状态逻辑统一消费
struct HitPointerWheel
{
    using is_event_tag = void;
    RawPointerWheel raw{};
    entt::entity hitEntity = entt::null;  // 滚轮发生位置的实体 (可能为 null)
};

/**
 * @brief 键盘重复驱动事件
 * [IMMEDIATE] 由 FrameTick 在每帧 Logic 阶段触发，
 * TextInputSystem 订阅后执行 doProcessKeyRepeat()，
 * 替代原有 static s_current 路由，支持多 UiRuntime 独立实例。
 */
struct TickKeyRepeat
{
    using is_event_tag = void;
};

}  // namespace ui::events