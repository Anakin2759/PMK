/**
 * ************************************************************************
 *
 * @file NetAddress.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-19
 * @version 0.1
 * @brief NetAddress 实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "NetAddress.h"
#include <asio.hpp>

// Pimpl 实现
struct NetAddress::Impl
{
    asio::ip::udp::endpoint endpoint;

    Impl() = default;

    explicit Impl(const asio::ip::udp::endpoint& ep) : endpoint(ep) {}

    Impl(const std::string& ip, uint16_t port)
    {
        asio::error_code ec;
        auto addr = asio::ip::make_address(ip, ec);
        if (!ec)
        {
            endpoint = asio::ip::udp::endpoint(addr, port);
        }
        else
        {
            // 默认为 IPv4 的 0.0.0.0
            endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), port);
        }
    }
};

NetAddress::NetAddress() : m_impl(std::make_unique<Impl>()) {}

NetAddress::NetAddress(const std::string& ip, uint16_t port) : m_impl(std::make_unique<Impl>(ip, port)) {}

NetAddress::NetAddress(const asio::ip::udp::endpoint& endpoint) : m_impl(std::make_unique<Impl>(endpoint)) {}

NetAddress::~NetAddress() = default;

NetAddress::NetAddress(const NetAddress& other) : m_impl(std::make_unique<Impl>(other.m_impl->endpoint)) {}

NetAddress& NetAddress::operator=(const NetAddress& other)
{
    if (this != &other)
    {
        m_impl = std::make_unique<Impl>(other.m_impl->endpoint);
    }
    return *this;
}

NetAddress::NetAddress(NetAddress&& other) noexcept = default;
NetAddress& NetAddress::operator=(NetAddress&& other) noexcept = default;

std::string NetAddress::ip() const
{
    return m_impl->endpoint.address().to_string();
}

uint16_t NetAddress::port() const
{
    return m_impl->endpoint.port();
}

std::string NetAddress::toString() const
{
    return ip() + ":" + std::to_string(port());
}

const asio::ip::udp::endpoint& NetAddress::toAsioEndpoint() const
{
    return m_impl->endpoint;
}

bool NetAddress::operator==(const NetAddress& other) const
{
    return m_impl->endpoint == other.m_impl->endpoint;
}

bool NetAddress::operator!=(const NetAddress& other) const
{
    return !(*this == other);
}

size_t NetAddress::hash() const noexcept
{
    // 组合 IP 和端口的哈希值
    size_t h1 = std::hash<std::string>{}(m_impl->endpoint.address().to_string());
    size_t h2 = std::hash<uint16_t>{}(m_impl->endpoint.port());
    return h1 ^ (h2 << 1);
}
