/**
 * ************************************************************************
 *
 * @file FrameHeader.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief 帧头定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>

// 增加 1 字节对齐，防止编译器插入填充字节导致帧格式与预期不符
#pragma pack(push, 1)

constexpr uint16_t FRAME_MAGIC = 0x55AA;

struct FrameHeader
{
    uint16_t magic = FRAME_MAGIC; // 语义更明确的命名
    uint16_t cmd = 0;             // 指令 ID
    uint16_t length = 0;          // 载荷长度
};
#pragma pack(pop)
