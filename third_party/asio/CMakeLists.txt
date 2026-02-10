cmake_minimum_required(VERSION 3.15)
project(asio VERSION 1.36.0 LANGUAGES CXX)

add_library(asio INTERFACE)
add_library(asio::asio ALIAS asio)
find_package(Threads)
if(Threads_FOUND)
    target_link_libraries(asio INTERFACE Threads::Threads)
endif()

target_include_directories(asio
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)