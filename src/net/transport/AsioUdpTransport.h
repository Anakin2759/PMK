/**
 * ************************************************************************
 *
 * @file AsioUdpTransport.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief ASIO UDP 层实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "IUdpTransport.h"
#include <span>
#include <array>
#include <functional>
#include <memory>

// 前向声明
namespace asio
{
class any_io_executor;
}

class AsioUdpTransport final : public IUdpTransport
{
public:
    AsioUdpTransport(const asio::any_io_executor& exec, uint16_t port);
    ~AsioUdpTransport();

    /**
     * @brief 发送数据包
     * @param address 目标地址
     * @param data 数据内容
     */
    void send(const NetAddress& address, std::span<const uint8_t> data) override;

    /**
     * @brief 获取本地绑定端口
     */
    [[nodiscard]] uint16_t localPort() const;

    /**
     * @brief 停止传输
     */
    void stop();

    /**
     * @brief 启动接收循环（回调模式，隔离 ASIO 协程）
     * @param handler 处理函数：void(const NetAddress&, std::span<const uint8_t>)
     */
    void startRecvLoop(std::function<void(const NetAddress&, std::span<const uint8_t>)> handler);

private:
    // Pimpl 模式：隐藏 ASIO 实现
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};