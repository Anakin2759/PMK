#include "Client.h"
#include <asio.hpp>
#include <chrono>
#include <memory>
#include <utility>

// Pimpl 实现
struct Client::Impl
{
    asio::io_context ioc;

    Impl() : ioc() {}
};

Client::Client(IUdpTransport& transport) : KcpEndpoint(transport), m_impl(std::make_unique<Impl>()) {}

Client::~Client() = default;

std::shared_ptr<KcpSession> Client::connect(uint32_t conv, const NetAddress& server_addr)
{
    auto [iter, inserted] = m_sessions.try_emplace(conv);
    if (inserted)
    {
        iter->second = createSession(conv, server_addr);
        m_lastActive[conv] = std::chrono::steady_clock::now();
        onSession(conv, iter->second);
    }
    return iter->second;
}

std::shared_ptr<KcpSession> Client::createSession(uint32_t conv, const NetAddress& peer)
{
    return std::make_shared<KcpSession>(conv, m_transport, peer, m_impl->ioc.get_executor());
}

uint32_t Client::selectConv([[maybe_unused]] const NetAddress& from, std::span<const uint8_t> data)
{
    return peekConv(data);
}
