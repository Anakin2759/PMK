/**
 * ************************************************************************
 *
 * @file PeekConv.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-16
 * @version 0.1
 * @brief 从数据片段中提取 KCP 会话标识 conv 的函数
 * 
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <cstdint>
#include <span>
#include <cstring>
#include <bit>
inline uint32_t peekConv(std::span<const uint8_t> data)
{
    if (data.size() < 4) [[unlikely]]
    {
        return 0;
    }

    uint32_t conv;
    std::memcpy(&conv, data.data(), sizeof(uint32_t));

    // 如果当前原生机器是 大端序，则需要翻转字节以匹配 KCP 的小端序
    if constexpr (std::endian::native == std::endian::big)
    {
        return std::byteswap(conv); // C++23 提供的标准字节交换
    }
    return conv;
}