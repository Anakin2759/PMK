/**
 * ************************************************************************
 *
 * @file test_InteractionRecorder.cpp
 * @brief P1-5 交互录制骨架测试
 *
 * 1. RoundTrip：构造事件序列 → 保存 → 读回 → 字段逐项一致
 * 2. PushAndPoll（offscreen）：回放注入 → SDL_PollEvent 取出 → 类型/数量一致
 *
 * ************************************************************************
 */
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "InteractionRecorder.hpp"

namespace ui::tests::interaction
{
namespace
{

/// 构造一条带典型字段的鼠标按下事件
SDL_Event MakeMouseButtonDown(SDL_WindowID windowId, float x, float y)
{
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = windowId;
    event.button.which = 1;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = true;
    event.button.x = x;
    event.button.y = y;
    return event;
}

/// 构造一条带典型字段的键盘按下事件
SDL_Event MakeKeyDown(SDL_WindowID windowId, SDL_Keycode key, SDL_Keymod mod)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = windowId;
    event.key.key = key;
    event.key.mod = mod;
    event.key.down = true;
    return event;
}

TEST(InteractionRecorderTest, RoundTripPreservesFields)
{
    const std::vector<SDL_Event> sourceEvents = {
        MakeMouseButtonDown(42, 10.0F, 20.0F),
        MakeKeyDown(42, SDLK_RETURN, SDL_KMOD_LCTRL),
    };

    InteractionRecorder recorder;
    for (const SDL_Event& event : sourceEvents)
    {
        recorder.AddEvent(event);
    }
    ASSERT_EQ(recorder.Count(), sourceEvents.size());

    const auto outputDir = std::filesystem::temp_directory_path() / "vmp_interaction_test";
    std::filesystem::create_directories(outputDir);
    const std::string filePath = (outputDir / "recording.bin").string();

    std::string error;
    ASSERT_TRUE(recorder.Save(filePath, error)) << error;

    std::vector<RecordedEvent> loaded;
    ASSERT_TRUE(LoadInteractionRecording(filePath, loaded, error)) << error;
    ASSERT_EQ(loaded.size(), sourceEvents.size());

    for (std::size_t i = 0; i < sourceEvents.size(); ++i)
    {
        EXPECT_EQ(loaded[i].event.type, sourceEvents[i].type);
        // 逐字节比较（整存整取的 POD 对象表示）
        EXPECT_EQ(std::memcmp(&loaded[i].event, &sourceEvents[i], sizeof(SDL_Event)), 0)
            << "record " << i << " bytes differ";
    }
}

TEST(InteractionRecorderTest, PushAndPollOnOffscreenWindow)
{
    ASSERT_EQ(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO, 0U);
    ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_VIDEO)) << SDL_GetError();

    SDL_Window* window = SDL_CreateWindow("interaction-replay", 200, 120, SDL_WINDOW_HIDDEN);
    ASSERT_NE(window, nullptr) << SDL_GetError();
    const SDL_WindowID windowId = SDL_GetWindowID(window);

    // 录制 3 个事件（注意：windowID 会被回放重映射为目标窗口）
    InteractionRecorder recorder;
    recorder.AddEvent(MakeMouseButtonDown(1, 30.0F, 40.0F));
    recorder.AddEvent(MakeKeyDown(1, SDLK_TAB, SDL_KMOD_NONE));
    recorder.AddEvent(MakeMouseButtonDown(1, 50.0F, 60.0F));

    // 经文件 round-trip 获得 records，验证回放注入
    const auto outputDir = std::filesystem::temp_directory_path() / "vmp_interaction_test";
    std::filesystem::create_directories(outputDir);
    const std::string filePath = (outputDir / "push_poll.bin").string();
    std::string error;
    ASSERT_TRUE(recorder.Save(filePath, error)) << error;
    std::vector<RecordedEvent> records;
    ASSERT_TRUE(LoadInteractionRecording(filePath, records, error)) << error;
    ASSERT_EQ(records.size(), 3U);

    // 清空队列（排除窗口创建等环境事件），再回放注入
    SDL_Event dummy{};
    while (SDL_PollEvent(&dummy))
    {
    }

    InteractionReplay replay;
    replay.PushAllImmediate(records, windowId);

    // Poll 出全部事件并统计
    std::vector<SDL_Event> polled;
    SDL_Event ev{};
    while (SDL_PollEvent(&ev))
    {
        polled.push_back(ev);
    }

    ASSERT_EQ(polled.size(), 3U);
    EXPECT_EQ(polled[0].type, SDL_EVENT_MOUSE_BUTTON_DOWN);
    EXPECT_EQ(polled[0].button.windowID, windowId);
    EXPECT_EQ(polled[1].type, SDL_EVENT_KEY_DOWN);
    EXPECT_EQ(polled[1].key.windowID, windowId);
    EXPECT_EQ(polled[2].type, SDL_EVENT_MOUSE_BUTTON_DOWN);
    EXPECT_EQ(polled[2].button.x, 50.0F);

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

}  // namespace
}  // namespace ui::tests::interaction
