cmake_minimum_required(VERSION 3.31)
project(vmpui_install_consumer LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(VMPUI CONFIG REQUIRED)

add_executable(vmpui_install_consumer main.cpp)
target_compile_features(vmpui_install_consumer PRIVATE cxx_std_23)
target_link_libraries(vmpui_install_consumer PRIVATE VMPUI::ui)