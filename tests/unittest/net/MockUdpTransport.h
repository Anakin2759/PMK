/**
 * ************************************************************************
 *
 * @file MockUdpTransport.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief 模拟 UDP 传输实现（用于单元测试）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include "src/net/transport/IUdpTransport.h"
#include <vector>
#include <mutex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

/**
 * @brief Mock UDP 传输层，用于测试
 */
class MockUdpTransport : public IUdpTransport
{
public:
    struct Packet
    {
        asio::ip::udp::endpoint to;
        std::vector<uint8_t> data;
    };

    void send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data) override
    {
        std::lock_guard lock(m_mutex);
        m_packets.push_back({to, std::vector<uint8_t>(data.begin(), data.end())});
        m_sendCount++;
    }

    // 测试辅助方法
    [[nodiscard]] size_t getSendCount() const
    {
        std::lock_guard lock(m_mutex);
        return m_sendCount;
    }

    [[nodiscard]] std::vector<Packet> getPackets() const
    {
        std::lock_guard lock(m_mutex);
        return m_packets;
    }

    void clearPackets()
    {
        std::lock_guard lock(m_mutex);
        m_packets.clear();
        m_sendCount = 0;
    }

    [[nodiscard]] bool hasPacketTo(const asio::ip::udp::endpoint& ep) const
    {
        std::lock_guard lock(m_mutex);
        return std::ranges::any_of(m_packets, [&](const Packet& p) { return p.to == ep; });
    }

private:
    mutable std::mutex m_mutex;
    std::vector<Packet> m_packets;
    size_t m_sendCount = 0;
};
