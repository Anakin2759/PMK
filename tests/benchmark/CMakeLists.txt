# GPU 窗口生命周期压力基准（Google Benchmark）
# 与单元测试分离，避免 100 次真实窗口创建/销毁阻塞 ui_unit_tests。

add_executable(ui_gpu_lifecycle_benchmark
    gpu_window_lifecycle_benchmark.cpp
)
target_compile_features(ui_gpu_lifecycle_benchmark PRIVATE cxx_std_23)
target_compile_definitions(ui_gpu_lifecycle_benchmark PRIVATE
    SDL_MAIN_HANDLED
)
target_include_directories(ui_gpu_lifecycle_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(ui_gpu_lifecycle_benchmark PRIVATE
    EnTT::EnTT
    Eigen3::Eigen
    SDL3::SDL3
    spdlog::spdlog_header_only
    ui
    benchmark::benchmark_main
)

target_compile_options(ui_gpu_lifecycle_benchmark PRIVATE
    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc>

    # Clang-cl
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>
)
