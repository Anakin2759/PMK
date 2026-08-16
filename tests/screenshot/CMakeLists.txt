# P1-5 截图回归测试目标
# offscreen + software 确定性渲染，读回帧与 tests/golden 基准比较。
# 与单元测试隔离（RUN_SERIAL + 独立 LABEL "screenshot"），不阻塞默认 ui_tests。

add_ui_test_executable(ui_screenshot_tests
    test_ScreenshotCapture.cpp
    test_VisualRegression.cpp
    ScreenshotCapture.cpp
)
configure_ui_test_target(ui_screenshot_tests)

target_include_directories(ui_screenshot_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

# golden 基准目录（仓库内）与截图输出目录（构建产物）
target_compile_definitions(ui_screenshot_tests PRIVATE
    UI_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../golden"
    UI_SCREENSHOT_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/screenshot-output"
)

include(GoogleTest)
gtest_discover_tests(ui_screenshot_tests
    DISCOVERY_TIMEOUT 60
    PROPERTIES
    LABELS "screenshot"
    ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
    RUN_SERIAL TRUE
    RESOURCE_LOCK ui_sdl_video
    TIMEOUT 120
)
