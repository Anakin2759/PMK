# P1-5 交互录制骨架测试目标
# 录制（EventWatch）+ 回放（SDL_PushEvent）工具与 round-trip 测试。
# offscreen 环境（ENVIRONMENT 属性），独立 LABEL "interaction"。

add_ui_test_executable(ui_interaction_tests
    test_InteractionRecorder.cpp
    InteractionRecorder.cpp
)
configure_ui_test_target(ui_interaction_tests)

include(GoogleTest)
gtest_discover_tests(ui_interaction_tests
    DISCOVERY_TIMEOUT 60
    PROPERTIES
    LABELS "interaction"
    ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
    RUN_SERIAL TRUE
    RESOURCE_LOCK ui_sdl_video
    TIMEOUT 120
)
