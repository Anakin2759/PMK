/**
 * ************************************************************************
 *
 * @file NetworkMessageSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 基于UDP的网络消息发送/接收系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <asio.hpp>
#include <memory>
#include "src/server/context/GameContext.h"
#include "src/server/events/NetWorkEvents.h"
#include "src/shared/messages/MessageDispatcher.h"
#include "src/shared/messages/request/SendMessageRequest.h"
#include "src/shared/messages/response/SendMessageToChatResponse.h"
#include "src/net/protocol/FrameCodec.h"

class NetworkMessageSystem
{
public:
    explicit NetworkMessageSystem(GameContext& context) : m_context(&context) { registerMessageHandlers(); };

    void registerEvents()
    {
        // 注册网络消息处理事件
        m_context->dispatcher.sink<events::NetworkMessageReceived>()
            .connect<&NetworkMessageSystem::onNetworkMessageReceived>(this);
    };
    void unregisterEvents()
    {
        // 注销网络消息处理事件
        m_context->dispatcher.sink<events::NetworkMessageReceived>()
            .disconnect<&NetworkMessageSystem::onNetworkMessageReceived>(this);
    };

private:
    MessageDispatcher m_messageDispatcher;

    void registerMessageHandlers()
    {
        // 注册聊天消息处理器
        m_messageDispatcher.registerHandler<SendMessageRequest>(
            [this](const SendMessageRequest& req) -> std::expected<std::vector<uint8_t>, MessageError>
            {
                m_context->logger->info("收到聊天消息 [频道{}]: {}", req.channelId, req.content);

                // 构造响应 (回显)
                SendMessageToChatResponse resp;
                resp.sender = 0; // System or User ID
                resp.chatMessage = "Server Echo: " + req.content;

                return resp.serialize();
            });
    }

    void onNetworkMessageReceived(const events::NetworkMessageReceived& event)
    {
        // 使用 FrameCodec 解码帧
        auto decodeResult = decodeFrame(event.payload);

        if (!decodeResult)
        {
            m_context->logger->warn("数据包解码失败 (来自连接 {})", event.connectionId);
            return;
        }

        auto [cmdId, payload] = *decodeResult;

        // 分发消息
        auto result = m_messageDispatcher.dispatch(cmdId, payload);

        if (result)
        {
            // 这里的 result 是 Handler 返回的响应数据 (std::vector<uint8_t>)
            // 实际应用中，我们需要通过 NetworkManager/Server 发送回客户端
            // 由于 NetworkMessageSystem 目前没有直接持有 Server 实例，我们这里仅做逻辑处理
            // 或者触发一个 "SendNetworkPacket" 事件
            m_context->logger->info("消息处理成功，生成响应 {} 字节", result->size());

            // TODO: m_context->dispatcher.trigger<events::SendNetworkPacket>({event.connectionId, *result});
        }
        else
        {
            // m_context->logger->warn("消息处理失败或无响应: Error {}", (int)result.error());
            // 注意: dispatch 返回 void if no handler? No, dispatch returns expected.
            // Wait, MessageDispatcher dispatch return type needs check.
        }
    }

    void StartNetworkService() {}
    void StopNetworkService() {}

    GameContext* m_context;
};