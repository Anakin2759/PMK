/**
 * ************************************************************************
 *
 * @file WindowsMacroShield.hpp
 * @brief 隔离会与 UI 公共 API 名称冲突的 Windows SDK 通用宏。
 *
 * Windows SDK 将 CreateWindow/CreateDialog 定义为通用名称宏。公共 Factory
 * API 使用同名 C++ 函数，因此在 Windows 上先完成 SDK 头的幂等包含，再移除
 * 通用名称宏。显式的 CreateWindowA/W 等 Win32 API 名称保持可用。
 *
 * ************************************************************************
 */
#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateDialog
#undef CreateDialog
#endif
#endif