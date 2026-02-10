#pragma once
#include "KcpEndpoint.h"
#include <bit>
#include <cstring>
#include <utility>
#include <memory>
#include "PeekConv.h"

class Server : public KcpEndpoint
{
public:
    // 构造函数：需要 UDP 传输层和线程池大小
    Server(IUdpTransport& transport, size_t thread_count);
    ~Server();
    // 停止服务器，等待线程池结束
    void stop();

protected:
    /**
     * @brief 识别逻辑：直接解析包里的 conv
     */
    uint32_t selectConv(const NetAddress&, std::span<const uint8_t> data) override;

    /**
     * @brief 创建 KCP 会话
     */
    std::shared_ptr<KcpSession> createSession(uint32_t conv, const NetAddress& peer) override;

    /**
     * @brief 会话创建回调
     * @param conv 会话的 Conv ID
     * @param session 新创建的 KCP 会话
     */
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> session) override;

private:
    // Pimpl 声明：隐藏 ASIO 实现细节
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};