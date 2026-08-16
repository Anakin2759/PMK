/**
 * ************************************************************************
 *
 * @file InteractionRecorder.hpp
 * @brief P1-5 交互录制骨架：SDL 事件录制（EventWatch）与回放（PushEvent）
 *
 * 文件格式（零依赖二进制，版本 1）：
 *   magic "VMPUIREC"（8 字节） | u32 version | u64 recordCount
 *   每条记录：u64 deltaNs（相对上一事件） | SDL_Event（128 字节 POD）
 *
 * 录制：仿 PlatformWindowSystem 挂 SDL_AddEventWatch，只记录输入类事件
 *       （鼠标/键盘/文本/退出），跳过窗口管理事件。
 * 回放：按 delta 节流（sleep），把 windowID 重映射为目标窗口后 SDL_PushEvent。
 *
 * ************************************************************************
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace ui::tests::interaction
{

/// 录制文件魔数与版本
inline constexpr std::string_view kInteractionRecordMagic = "VMPUIREC";
inline constexpr std::uint32_t kInteractionRecordVersion = 1;

/// 单条录制记录：相对上一事件的时间增量 + 完整 SDL_Event（128 字节 POD）
struct RecordedEvent
{
    std::uint64_t deltaNs = 0;
    SDL_Event event{};
};

/**
 * @brief SDL 输入事件录制器。
 *
 * 用法：
 *   InteractionRecorder recorder;
 *   recorder.Begin();   // 挂 EventWatch（录制期间所有输入事件入队）
 *   ...运行 UI...
 *   recorder.End();     // 卸 EventWatch
 *   recorder.Save(path, error);
 */
class InteractionRecorder
{
   public:
    InteractionRecorder() = default;
    ~InteractionRecorder();

    InteractionRecorder(const InteractionRecorder&) = delete;
    InteractionRecorder& operator=(const InteractionRecorder&) = delete;

    /// 开始录制：挂 SDL_AddEventWatch
    void Begin();
    /// 停止录制：卸 SDL_RemoveEventWatch
    void End();

    /// 已录制事件数
    [[nodiscard]] std::size_t Count() const noexcept;

    /// 把已录制事件保存为二进制文件
    [[nodiscard]] bool Save(const std::string& path, std::string& error) const;

    /// 测试辅助：直接追加一条事件（不经过 EventWatch）
    void AddEvent(const SDL_Event& event);

    /// 是否只记录指定类型（默认记录全部输入类事件）
    void SetTargetType(std::uint32_t eventType);

   private:
    static bool SDLCALL EventWatch(void* userdata, SDL_Event* event);
    void record(const SDL_Event& event);

    std::vector<RecordedEvent> m_records;
    std::uint64_t m_lastTimestampNs = 0;
    bool m_recording = false;
    std::uint32_t m_targetType = SDL_EVENT_LAST;  // SDL_EVENT_LAST = 不过滤
};

/**
 * @brief 从文件加载录制记录。
 */
[[nodiscard]] bool LoadInteractionRecording(const std::string& path, std::vector<RecordedEvent>& outRecords,
                                            std::string& error);

/**
 * @brief 回放器：按 delta 节流并注入 SDL 事件队列。
 *
 * 非阻塞：调用方负责在事件循环外驱动（或本类按 delta sleep）。
 */
class InteractionReplay
{
   public:
    /// 把录制事件按 delta 节流推入 SDL 队列；windowID 重映射到 targetWindowID（0 = 不重映射）。
    void PushAll(const std::vector<RecordedEvent>& records, SDL_WindowID targetWindowID) const;

    /// 不节流、立即推入（用于确定性测试）。
    void PushAllImmediate(const std::vector<RecordedEvent>& records, SDL_WindowID targetWindowID) const;
};

}  // namespace ui::tests::interaction
