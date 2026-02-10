/**
 * ************************************************************************
 *
 * @file IUdpTransport.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief  UDP 传输层抽象定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../common/NetAddress.h"
#include <span>
#include <cstdint>

class IUdpTransport
{
public:
    IUdpTransport() = default;
    IUdpTransport(const IUdpTransport&) = default;
    IUdpTransport& operator=(const IUdpTransport&) = delete;
    IUdpTransport(IUdpTransport&&) = default;
    IUdpTransport& operator=(IUdpTransport&&) = default;
    virtual ~IUdpTransport() = default;

    virtual void send(const NetAddress& address, std::span<const uint8_t> data) = 0;
};