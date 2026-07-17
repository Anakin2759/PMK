# UI module tests

function(configure_ui_test_target target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_23)
    target_compile_definitions(${target_name} PRIVATE
        SDL_MAIN_HANDLED
    )
    target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(${target_name} PRIVATE
        EnTT::EnTT
        SDL3::SDL3
        ui
        GTest::gmock
        GTest::gmock_main
        GTest::gtest_main
    )

    target_compile_options(${target_name} PRIVATE
        # GCC
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

        # Clang
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

        # MSVC
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc>

        # Clang-cl
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>
    )
endfunction()

add_executable(ui_unit_tests
    test_BatchManager.cpp
    test_TextUtils.cpp
    test_ResourceProvider.cpp
    test_result.cpp
    test_UiRuntime.cpp
    test_SystemManager.cpp
    test_ApplicationLifecycle.cpp
    test_DeviceClaimState.cpp
    test_GPUTextureOwner.cpp
    test_GpuInitializationTransaction.cpp
)
configure_ui_test_target(ui_unit_tests)

add_executable(ui_ecs_tests
    test_MainWindow.cpp
    test_TweenSystem.cpp
    test_TaskChain.cpp
    test_Utils.cpp
    test_Visibility.cpp
    test_Hierarchy.cpp
    test_DragDrop.cpp
    test_ThemeSystem_Button.cpp
    test_ThemeSystem_Input.cpp
    test_ThemeSystem_DropDown.cpp
    test_ThemeSystem_Window.cpp
)
configure_ui_test_target(ui_ecs_tests)

add_executable(ui_api_tests
    test_UmbrellaHeader.cpp
    test_PublicLeafHeaders.cpp
    test_PublicEventApi.cpp
)
configure_ui_test_target(ui_api_tests)

add_executable(ui_fallback_lifecycle_tests
    test_FallbackWindowLifecycle.cpp
)
configure_ui_test_target(ui_fallback_lifecycle_tests)

# Backward-compatible aggregate target name for existing workflows.
add_custom_target(ui_tests DEPENDS ui_unit_tests ui_ecs_tests ui_api_tests ui_fallback_lifecycle_tests)

set_source_files_properties(${CMAKE_SOURCE_DIR}/include/ui.hpp PROPERTIES LANGUAGE CXX)
add_library(ui_public_header_check OBJECT ${CMAKE_SOURCE_DIR}/include/ui.hpp)
target_compile_features(ui_public_header_check PRIVATE cxx_std_23)
target_link_libraries(ui_public_header_check PRIVATE ui)
target_compile_options(ui_public_header_check PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:/clang:-Wno-pragma-once-outside-header>
)
add_dependencies(ui_api_tests ui_public_header_check)

include(GoogleTest)
gtest_discover_tests(ui_unit_tests PROPERTIES LABELS "unit")
gtest_discover_tests(ui_ecs_tests PROPERTIES LABELS "ecs")
gtest_discover_tests(ui_api_tests PROPERTIES LABELS "api")
gtest_discover_tests(ui_fallback_lifecycle_tests PROPERTIES
    LABELS "integration;sdl;lifecycle"
    ENVIRONMENT "SDL_VIDEODRIVER=offscreen"
    RUN_SERIAL TRUE
    RESOURCE_LOCK ui_sdl_video
    TIMEOUT 60
)
