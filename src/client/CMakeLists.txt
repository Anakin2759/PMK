cmake_minimum_required(VERSION 3.20)

set(CLIENT_NAME PestManKillClient)

# 客户端源文件
set(CLIENT_SOURCES
    main.cpp
)

# 客户端头文件
set(CLIENT_HEADERS
    )

# 创建客户端可执行文件
add_executable(${CLIENT_NAME} ${CLIENT_SOURCES} ${CLIENT_HEADERS})
target_compile_features(ui INTERFACE cxx_std_23)
target_compile_definitions(ui INTERFACE 
    $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN>
)
# 编译选项
target_compile_options(${CLIENT_NAME} PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RelWithDebInfo>>:-Wall -O2 -g -DNDEBUG>

    # Clang (non-MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:RelWithDebInfo>>:-Wall -O2 -g -DNDEBUG>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Z7 /EHsc /utf-8 /bigobj>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc /utf-8 /bigobj>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/O2 /Z7 /DNDEBUG /EHsc /utf-8 /bigobj>

    # Clang-cl (MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4 /bigobj>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG /bigobj>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:RelWithDebInfo>>:/EHsc /O2 /Zi /DNDEBUG /bigobj>
)

# 包含目录
target_include_directories(${CLIENT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

# 链接库
target_link_libraries(${CLIENT_NAME} PRIVATE 
    # 核心依赖
    mimalloc-static 
    utils
    shared
    nlohmann_json::nlohmann_json
    # Abseil
    absl::base 
    absl::memory 
    absl::strings
    absl::hash
    absl::flat_hash_map
    absl::raw_hash_set

    # UI 模块
    ui
    
)






