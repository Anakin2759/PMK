#pragma once

#include <locale>

#ifdef _WIN32
#include <windows.h>
#endif
namespace utils::functions
{
inline void SetConsoleToUTF8()
{
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, "C"); // NOLINT
    // 强制 C++ I/O 流语言环境为 "C" (英文)
    std::locale::global(std::locale("C"));
#endif
}
} // namespace utils::functions