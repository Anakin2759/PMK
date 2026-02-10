cmake_minimum_required(VERSION 3.15)

add_library(net STATIC
    # Common
    common/NetAddress.cpp
    # Transport
    transport/AsioUdpTransport.cpp
    # Session
    Session/KcpSession.cpp
    # App
    App/Server.cpp
    App/KcpEndpoint.cpp
    App/Client.cpp
)

target_compile_options(net PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g -std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG -std=c++23>

    # Clang (non-MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc>

    # Clang-cl (MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>
)

# 公共包含目录（允许外部使用模块公共头文件）
target_include_directories(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 私有包含目录（内部实现细节）
target_include_directories(net PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/kcp
)

# 所有第三方依赖都改为 PRIVATE，不泄露到外部
target_link_libraries(net PRIVATE
    kcp
    asio::asio
    nlohmann_json::nlohmann_json
)

# 导出头文件列表（用于 IDE 项目视图）
set(NET_HEADERS
    # Common
    common/NetAddress.h
    # Transport
    transport/IUdpTransport.h
    transport/AsioUdpTransport.h
    # Session
    Session/KcpSession.h
    # App
    App/KcpEndpoint.h
    App/Server.h
    App/Client.h
)
target_sources(net PUBLIC ${NET_HEADERS})