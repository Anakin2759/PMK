
#pragma once

// 定义 _WIN32 或 __linux__ 等平台宏检查，这里以 Windows 和 GCC 为例
#ifdef _WIN32
#ifdef MYLIB_BUILD_DLL                    // 这个宏由你的 CMake 或项目文件在编译 DLL 时定义
#define VMP_UI_API __declspec(dllexport)  // 编译 DLL 时：导出
#else
#define VMP_UI_API __declspec(dllimport)  // 使用 DLL 时：导入
#endif
#else
// Linux / macOS 环境
#if __GNUC__ >= 4
#define VMP_UI_API __attribute__((visibility("default")))  // 标记为可见（导出）
#else
#define VMP_UI_API
#endif
#endif