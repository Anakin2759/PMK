/**
 * ************************************************************************
 *
 * @file KcpSession.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 会话层管理 KCP 连接
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "KcpSession.h"
#include <asio.hpp>
#include <asio/experimental/channel.hpp>
#include <ikcp.h>
#include <system_error>
#include <atomic>
#include <queue>
#include <mutex>

namespace
{
constexpr size_t CHANNEL_CAPACITY = 64;
constexpr int KCP_UPDATE_INTERVAL_MS = 10;
constexpr int KCP_MIN_RTO_MS = 10;
constexpr size_t RECV_BUFFER_SIZE = 2048;
} // namespace

// Pimpl 实现
struct KcpSession::Impl
{
    ikcpcb* kcp{nullptr};
    IUdpTransport& transport;
    NetAddress peer;
    asio::experimental::channel<asio::any_io_executor, void(std::error_code, Packet)> channel;
    std::atomic<size_t> droppedPackets{0};
    std::atomic<bool> closed{false};

    Impl(uint32_t conv, IUdpTransport& trans, const NetAddress& peerAddr, const asio::any_io_executor& exec)
        : kcp(ikcp_create(conv, this)), transport(trans), peer(peerAddr), channel(exec, CHANNEL_CAPACITY)
    {
        if (kcp != nullptr)
        {
            kcp->output = &Impl::kcpOutput;
            ikcp_nodelay(kcp, 1, KCP_UPDATE_INTERVAL_MS, 2, 1);
            kcp->rx_minrto = KCP_MIN_RTO_MS;
        }
    }

    ~Impl()
    {
        if (kcp != nullptr)
        {
            ikcp_release(kcp);
        }
    }

    static int kcpOutput(const char* buf, int len, ikcpcb* /*kcp*/, void* user)
    {
        auto* self = static_cast<Impl*>(user);
        if (self == nullptr || buf == nullptr || len <= 0)
        {
            return -1;
        }

        if (self->closed.load(std::memory_order_acquire))
        {
            return 0;
        }

        self->transport.send(self->peer,
                             std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(len)));
        return 0;
    }

    // 协程接收
    asio::awaitable<std::expected<Packet, std::error_code>> recvCoro()
    {
        try
        {
            if (closed.load(std::memory_order_acquire))
            {
                co_return std::unexpected(std::make_error_code(std::errc::operation_canceled));
            }
            Packet data = co_await channel.async_receive(asio::use_awaitable);
            co_return data;
        }
        catch (const std::system_error& e)
        {
            co_return std::unexpected(e.code());
        }
    }
};

KcpSession::KcpSession(uint32_t conv,
                       IUdpTransport& transport,
                       const NetAddress& peer,
                       const asio::any_io_executor& exec)
    : m_impl(std::make_unique<Impl>(conv, transport, peer, exec))
{
}

KcpSession::~KcpSession()
{
    close();
}

void KcpSession::input(std::span<const uint8_t> data)
{
    if (m_impl->kcp == nullptr || m_impl->closed.load(std::memory_order_acquire))
    {
        return;
    }

    ikcp_input(m_impl->kcp, reinterpret_cast<const char*>(data.data()), static_cast<long>(data.size()));

    // 提取完整包并推入通道
    std::vector<uint8_t> recvBuffer(RECV_BUFFER_SIZE);
    int bytesReceived = 0;
    while ((bytesReceived = ikcp_recv(
                m_impl->kcp, reinterpret_cast<char*>(recvBuffer.data()), static_cast<int>(recvBuffer.size()))) > 0)
    {
        recvBuffer.resize(static_cast<size_t>(bytesReceived));
        if (!m_impl->channel.try_send(std::error_code{}, recvBuffer))
        {
            m_impl->droppedPackets.fetch_add(1, std::memory_order_relaxed);
        }
        recvBuffer.clear();
        recvBuffer.resize(RECV_BUFFER_SIZE);
    }
}

void KcpSession::recvAsync(RecvCallback callback)
{
    auto self = shared_from_this();
    asio::co_spawn(
        m_impl->channel.get_executor(),
        [this, callback = std::move(callback)]() -> asio::awaitable<void>
        {
            auto result = co_await m_impl->recvCoro();
            callback(std::move(result));
        },
        asio::detached);
}

void KcpSession::send(std::span<const uint8_t> data)
{
    if (m_impl->kcp == nullptr || m_impl->closed.load(std::memory_order_acquire))
    {
        return;
    }
    ikcp_send(m_impl->kcp, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
}

void KcpSession::update(uint32_t now)
{
    if (m_impl->kcp != nullptr && !m_impl->closed.load(std::memory_order_acquire))
    {
        ikcp_update(m_impl->kcp, now);
    }
}

uint32_t KcpSession::check(uint32_t now) const
{
    if (m_impl->kcp == nullptr)
    {
        return now;
    }
    return ikcp_check(m_impl->kcp, now);
}

void KcpSession::close()
{
    bool expected = false;
    if (m_impl->closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
    {
        m_impl->channel.close();
    }
}

size_t KcpSession::droppedPackets() const noexcept
{
    return m_impl->droppedPackets.load(std::memory_order_relaxed);
}