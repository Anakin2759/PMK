/**
 * ************************************************************************
 *
 * @file AsioUdpTransport.cpp
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

#include "AsioUdpTransport.h"
#include <asio.hpp>
#include <array>

// Pimpl 实现
struct AsioUdpTransport::Impl
{
    asio::ip::udp::socket socket;
    std::array<uint8_t, 2048> recv_buffer{};

    Impl(const asio::any_io_executor& exec, uint16_t port)
    
        : socket(exec, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
    {
    }

    // 内部协程：接收循环
    asio::awaitable<void> recvLoop(std::function<void(const NetAddress&, std::span<const uint8_t>)> handler)
    {
        asio::ip::udp::endpoint from;
        for (;;)
        {
            asio::error_code ec;
            std::size_t n = co_await socket.async_receive_from(
                asio::buffer(recv_buffer), from, asio::redirect_error(asio::use_awaitable, ec));
            if (ec)
            {
                co_return;
            }

            NetAddress addr(from);
            handler(addr, std::span<const uint8_t>(recv_buffer.data(), n));
        }
    }
};

AsioUdpTransport::AsioUdpTransport(const asio::any_io_executor& exec, uint16_t port)
    : m_impl(std::make_unique<Impl>(exec, port))
{
}

AsioUdpTransport::~AsioUdpTransport() = default;

void AsioUdpTransport::send(const NetAddress& address, std::span<const uint8_t> data)
{
    asio::error_code ec;
    m_impl->socket.send_to(asio::buffer(data.data(), data.size()), address.toAsioEndpoint(), 0, ec);
}

uint16_t AsioUdpTransport::localPort() const
{
    return m_impl->socket.local_endpoint().port();
}

void AsioUdpTransport::stop()
{
    asio::error_code ec;
    m_impl->socket.close(ec);
}

void AsioUdpTransport::startRecvLoop(std::function<void(const NetAddress&, std::span<const uint8_t>)> handler)
{
    asio::co_spawn(m_impl->socket.get_executor(), m_impl->recvLoop(std::move(handler)), asio::detached);
}