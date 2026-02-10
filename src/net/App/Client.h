/**
 * ************************************************************************
 *
 * @file Client.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-16
 * @version 0.1
 * @brief  KCP 客户端实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include "KcpEndpoint.h"
#include <memory>
#include <utility>
#include "PeekConv.h"

class Client : public KcpEndpoint
{
public:
    /**
     * @brief 构造函数
     * @param transport UDP 传输层实现
     */
    explicit Client(IUdpTransport& transport);
    ~Client();

    /**
     * @brief 主动连接服务器
     * @param conv 预先分配的会话 ID
     * @param server_addr 服务器的 UDP 地址
     * @return 建立好的会话对象
     */
    std::shared_ptr<KcpSession> connect(uint32_t conv, const NetAddress& server_addr);

    /**
     * @brief 更新所有会话状态
     * @param now_ms 当前时间点
     * @param timeout_sec 会话超时阈值（默认30秒）
     */
    using KcpEndpoint::update;

protected:
    /**
     * @brief 识别逻辑：直接解析包里的 conv
     */
    uint32_t selectConv(const NetAddress&, std::span<const uint8_t> data) override;

    /**
     * @brief 创建 KCP 会话
     */
    std::shared_ptr<KcpSession> createSession(uint32_t conv, const NetAddress& peer) override;

private:
    // Pimpl 声明：隐藏 ASIO 实现细节
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};