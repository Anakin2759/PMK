/**
 * ************************************************************************
 *
 * @file KcpEndpoint.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief KCP 端点定义
    外部使用网络模块的接口基类
    管理多个 KCP 会话，处理输入输出分发，并支持会话超时清理
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../Session/KcpSession.h"
#include "../common/NetAddress.h"
#include <unordered_map>
#include <chrono>
#include <utility>

class KcpEndpoint
{
protected:
    explicit KcpEndpoint(IUdpTransport& transport) : m_transport(transport) {}

public:
    virtual ~KcpEndpoint() = default;

    /**
     * @brief 处理收到的 UDP 包并分发给对应的会话
     */
    void input(const NetAddress& from, std::span<const uint8_t> data)
    {
        if (data.size() < 4) [[unlikely]]
        {
            return;
        }

        uint32_t conv = selectConv(from, data);

        // 优化：try_emplace 避免了重复查找和冗余构造
        auto [iter, inserted] = m_sessions.try_emplace(conv);

        if (inserted) [[unlikely]]
        {
            iter->second = createSession(conv, from);
            onSession(conv, iter->second); // 传递 shared_ptr 保证生命周期
        }

        iter->second->input(data);
        m_lastActive[conv] = std::chrono::steady_clock::now(); // 更新活跃时间
    }

    /**
     * @brief 更新所有会话状态，并清理超时会话
     * @param now_ms 当前时间点
     * @param timeout_sec 会话超时阈值（默认30秒）
     */
    void update(uint32_t now_ms, std::chrono::seconds timeout_sec = std::chrono::seconds(30))
    {
        auto now_tp = std::chrono::steady_clock::now();
        for (auto iter = m_sessions.begin(); iter != m_sessions.end();)
        {
            auto& [conv, sess] = *iter;

            // 驱动 KCP
            sess->update(now_ms);

            // 检查超时清理
            auto last_iter = m_lastActive.find(conv);
            if (last_iter == m_lastActive.end())
            {
                m_lastActive.emplace(conv, now_tp);
                ++iter;
                continue;
            }

            if (now_tp - last_iter->second > timeout_sec)
            {
                sess->close();
                onSessionClosed(conv);
                m_lastActive.erase(conv);
                iter = m_sessions.erase(iter); // 安全删除
            }
            else
            {
                ++iter;
            }
        }
    }

protected:
    /**
     * @brief 创建 KCP 会话（由子类实现，提供所需的 executor）
     */
    virtual std::shared_ptr<KcpSession> createSession(uint32_t conv, const NetAddress& peer) = 0;

    /**
     * @brief 选择 KCP Conv ID 的方法
     * @return uint32_t 选中的 Conv ID
     */
    virtual uint32_t selectConv(const NetAddress&, std::span<const uint8_t>) = 0;

    /**
     * @brief 会话创建回调
     * @param conv 会话的 Conv ID
     * @param session 新创建的 KCP 会话
     */
    virtual void onSession([[maybe_unused]] uint32_t conv, [[maybe_unused]] std::shared_ptr<KcpSession> session) {}

    /**
     * @brief 会话关闭回调
     * @param conv 会话的 Conv ID
     */
    virtual void onSessionClosed([[maybe_unused]] uint32_t conv) {}

protected:
    IUdpTransport& m_transport;
    std::unordered_map<uint32_t, std::shared_ptr<KcpSession>> m_sessions;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> m_lastActive;
};