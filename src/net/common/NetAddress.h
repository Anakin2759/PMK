/**
 * ************************************************************************
 *
 * @file NetAddress.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-19
 * @version 0.1
 * @brief 网络地址抽象，隔离 ASIO 依赖
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <string>
#include <cstdint>
#include <memory>

// 前向声明，避免在头文件中包含 ASIO
namespace asio::ip
{
class udp;
template <typename Protocol>
class basic_endpoint;
using udp_endpoint = basic_endpoint<udp>;
} // namespace asio::ip

/**
 * @brief 网络地址封装，隔离底层传输层实现细节
 */
class NetAddress
{
public:
    /**
     * @brief 默认构造函数
     */
    NetAddress();

    /**
     * @brief 从 IP 地址和端口构造
     * @param ip IP 地址字符串（IPv4 或 IPv6）
     * @param port 端口号
     */
    NetAddress(const std::string& ip, uint16_t port);

    /**
     * @brief 从 ASIO endpoint 构造（内部使用）
     */
    explicit NetAddress(const asio::ip::udp_endpoint& endpoint);

    ~NetAddress();

    // 支持拷贝和移动
    NetAddress(const NetAddress& other);
    NetAddress& operator=(const NetAddress& other);
    NetAddress(NetAddress&& other) noexcept;
    NetAddress& operator=(NetAddress&& other) noexcept;

    /**
     * @brief 获取 IP 地址字符串
     */
    [[nodiscard]] std::string ip() const;

    /**
     * @brief 获取端口号
     */
    [[nodiscard]] uint16_t port() const;

    /**
     * @brief 获取地址的字符串表示（格式：ip:port）
     */
    [[nodiscard]] std::string toString() const;

    /**
     * @brief 转换为 ASIO endpoint（内部使用）
     */
    [[nodiscard]] const asio::ip::udp_endpoint& toAsioEndpoint() const;

    /**
     * @brief 比较运算符
     */
    bool operator==(const NetAddress& other) const;
    bool operator!=(const NetAddress& other) const;

    /**
     * @brief 支持 std::hash（用于 unordered_map）
     */
    [[nodiscard]] size_t hash() const noexcept;

private:
    // Pimpl 模式：隐藏 ASIO 实现
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// std::hash 特化
namespace std
{
template <>
struct hash<NetAddress>
{
    size_t operator()(const NetAddress& addr) const noexcept { return addr.hash(); }
};
} // namespace std
