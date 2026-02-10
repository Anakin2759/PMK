/**
 * ************************************************************************
 *
 * @file KcpSession.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief KCP 会话层抽象
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include "../transport/IUdpTransport.h"
#include "../common/NetAddress.h"
#include <expected>
#include <span>
#include <memory>
#include <vector>
#include <system_error>
#include <functional>

// 前向声明
namespace asio
{
class any_io_executor;
}

class KcpSession : public std::enable_shared_from_this<KcpSession>
{
public:
    using Packet = std::vector<uint8_t>;
    using RecvCallback = std::function<void(std::expected<Packet, std::error_code>)>;

    /**
     * @brief 构造函数
     * @param conv 会话的 Conv ID
     * @param transport 底层 UDP 传输实现
     * @param peer 对端 UDP 地址
     * @param exec ASIO 执行器（内部使用）
     */
    KcpSession(uint32_t conv, IUdpTransport& transport, const NetAddress& peer, const asio::any_io_executor& exec);

    ~KcpSession();

    // 禁止拷贝和移动
    KcpSession(const KcpSession&) = delete;
    KcpSession& operator=(const KcpSession&) = delete;
    KcpSession(KcpSession&&) = delete;
    KcpSession& operator=(KcpSession&&) = delete;

    /**
     * @brief 供 Endpoint 调用：喂入底层 UDP 数据
     */
    void input(std::span<const uint8_t> data);

    /**
     * @brief 异步接收一个 KCP 完整包（回调模式）
     * @param callback 接收完成回调
     */
    void recvAsync(RecvCallback callback);

    /**
     * @brief 发送数据
     */
    void send(std::span<const uint8_t> data);

    /**
     * @brief 主动关闭会话
     */
    void close();

    /**
     * @brief 获取因通道满而丢弃的包数量
     */
    [[nodiscard]] size_t droppedPackets() const noexcept;

    /**
     * @brief 更新 KCP 状态，需定期调用
     * @param now 当前时间戳（毫秒）
     */
    void update(uint32_t now);

    /**
     * @brief 获取下一次更新的时间点
     */
    [[nodiscard]] uint32_t check(uint32_t now) const;

private:
    // Pimpl 模式：隐藏 KCP 和 ASIO 实现
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};