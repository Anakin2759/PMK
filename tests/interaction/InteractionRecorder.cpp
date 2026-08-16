/**
 * ************************************************************************
 *
 * @file InteractionRecorder.cpp
 * @brief P1-5 交互录制骨架实现
 *
 * 文件格式（版本 1）：
 *   "VMPUIREC" (8B) | u32 version | u64 recordCount
 *   每条：u64 deltaNs | SDL_Event (128B)
 *
 * SDL_Event 为 128 字节 POD union（SDL_COMPILE_TIME_ASSERT 保证），
 * 直接整存整取对象表示，回放 memcpy 回 SDL_Event 字段即一致。
 *
 * ************************************************************************
 */
#include "InteractionRecorder.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <thread>

namespace ui::tests::interaction
{
namespace
{

/// 需要录制的输入事件类型（与 InteractionSystem 消费的事件对应）
[[nodiscard]] bool IsRecordableInputEvent(std::uint32_t type)
{
    switch (type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_QUIT:
            return true;
        default:
            return false;
    }
}

/// 事件子结构中 windowID 的公共字节偏移（type,reserved,timestamp 之后）。
/// SDL3 各输入事件子结构（key/text/motion/button）的 windowID 均位于偏移 16。
constexpr std::size_t kWindowIdByteOffset = 16;

/// 事件是否携带 windowID（鼠标/键盘/文本事件有，Quit 无）
[[nodiscard]] bool EventHasWindowId(std::uint32_t type)
{
    switch (type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT:
            return true;
        default:
            return false;
    }
}

}  // namespace

InteractionRecorder::~InteractionRecorder()
{
    End();
}

void InteractionRecorder::Begin()
{
    if (m_recording)
    {
        return;
    }
    m_recording = true;
    m_records.clear();
    m_lastTimestampNs = 0;
    SDL_AddEventWatch(&InteractionRecorder::EventWatch, this);
}

void InteractionRecorder::End()
{
    if (!m_recording)
    {
        return;
    }
    m_recording = false;
    SDL_RemoveEventWatch(&InteractionRecorder::EventWatch, this);
}

std::size_t InteractionRecorder::Count() const noexcept
{
    return m_records.size();
}

bool SDLCALL InteractionRecorder::EventWatch(void* userdata, SDL_Event* event)
{
    auto* recorder = static_cast<InteractionRecorder*>(userdata);
    recorder->record(*event);
    return true;  // 不吞事件，继续正常分发
}

void InteractionRecorder::record(const SDL_Event& event)
{
    if (!m_recording)
    {
        return;
    }
    if (m_targetType != SDL_EVENT_LAST && event.type != m_targetType)
    {
        return;
    }
    if (!IsRecordableInputEvent(event.type))
    {
        return;
    }

    const std::uint64_t timestampNs = event.common.timestamp;
    const std::uint64_t deltaNs = (m_lastTimestampNs != 0 && timestampNs >= m_lastTimestampNs)
                                      ? timestampNs - m_lastTimestampNs
                                      : 0;
    m_lastTimestampNs = timestampNs;

    RecordedEvent record;
    record.deltaNs = deltaNs;
    record.event = event;
    m_records.push_back(record);
}

void InteractionRecorder::AddEvent(const SDL_Event& event)
{
    // 测试辅助：直接追加一条事件（不经过 EventWatch，也不要求 Begin() 已录制）
    if (!IsRecordableInputEvent(event.type))
    {
        return;
    }
    RecordedEvent record;
    record.deltaNs = 0;
    record.event = event;
    m_records.push_back(record);
}

void InteractionRecorder::SetTargetType(std::uint32_t eventType)
{
    m_targetType = eventType;
}

bool InteractionRecorder::Save(const std::string& path, std::string& error) const
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        error = "cannot open output file: " + path;
        return false;
    }

    // 文件头
    stream.write(kInteractionRecordMagic.data(), static_cast<std::streamsize>(kInteractionRecordMagic.size()));
    const std::uint32_t version = kInteractionRecordVersion;
    stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
    const std::uint64_t count = m_records.size();
    stream.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // 每条记录
    for (const RecordedEvent& record : m_records)
    {
        stream.write(reinterpret_cast<const char*>(&record.deltaNs), sizeof(record.deltaNs));
        static_assert(sizeof(SDL_Event) == 128, "SDL_Event must be 128-byte POD");
        stream.write(reinterpret_cast<const char*>(&record.event), sizeof(SDL_Event));
    }

    stream.flush();
    if (!stream)
    {
        error = "write failed: " + path;
        return false;
    }
    return true;
}

bool LoadInteractionRecording(const std::string& path, std::vector<RecordedEvent>& outRecords, std::string& error)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        error = "cannot open input file: " + path;
        return false;
    }

    char magic[8]{};
    stream.read(magic, static_cast<std::streamsize>(kInteractionRecordMagic.size()));
    if (std::memcmp(magic, kInteractionRecordMagic.data(), kInteractionRecordMagic.size()) != 0)
    {
        error = "invalid magic: not an interaction recording";
        return false;
    }

    std::uint32_t version = 0;
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != kInteractionRecordVersion)
    {
        error = "unsupported version: " + std::to_string(version);
        return false;
    }

    std::uint64_t count = 0;
    stream.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count > (1024U * 1024U))
    {
        error = "record count too large: " + std::to_string(count);
        return false;
    }

    outRecords.clear();
    outRecords.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i)
    {
        RecordedEvent record;
        stream.read(reinterpret_cast<char*>(&record.deltaNs), sizeof(record.deltaNs));
        stream.read(reinterpret_cast<char*>(&record.event), sizeof(SDL_Event));
        if (!stream)
        {
            error = "truncated file at record " + std::to_string(i);
            return false;
        }
        outRecords.push_back(record);
    }
    return true;
}

void InteractionReplay::PushAll(const std::vector<RecordedEvent>& records, SDL_WindowID targetWindowID) const
{
    for (const RecordedEvent& record : records)
    {
        if (record.deltaNs > 0)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(record.deltaNs));
        }

        SDL_Event event = record.event;
        if (targetWindowID != 0 && EventHasWindowId(event.type))
        {
            // windowID 公共偏移：type,reserved,timestamp 之后
            auto* windowIdPtr = reinterpret_cast<std::uint32_t*>(
                reinterpret_cast<char*>(&event) + kWindowIdByteOffset);  // NOLINT
            *windowIdPtr = targetWindowID;
        }
        SDL_PushEvent(&event);
    }
}

void InteractionReplay::PushAllImmediate(const std::vector<RecordedEvent>& records, SDL_WindowID targetWindowID) const
{
    for (const RecordedEvent& record : records)
    {
        SDL_Event event = record.event;
        if (targetWindowID != 0 && EventHasWindowId(event.type))
        {
            auto* windowIdPtr = reinterpret_cast<std::uint32_t*>(
                reinterpret_cast<char*>(&event) + kWindowIdByteOffset);  // NOLINT
            *windowIdPtr = targetWindowID;
        }
        SDL_PushEvent(&event);
    }
}

}  // namespace ui::tests::interaction
