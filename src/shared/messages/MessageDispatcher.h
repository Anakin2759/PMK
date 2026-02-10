/**
 * ************************************************************************
 *
 * @file MessageDispatcher.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 消息分发器，用于注册和分发网络消息处理器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "MessageBase.h"
#include "src/net/protocol/FrameCodec.h"
#include <unordered_map>
#include <functional>
#include <span>
#include <expected>
#include <memory>

/**
 * @brief 消息处理器基类
 */
class IMessageHandler
{
public:
    virtual ~IMessageHandler() = default;

    /**
     * @brief 处理消息
     * @param payload 消息载荷数据
     * @return 成功返回响应数据，失败返回错误
     */
    virtual std::expected<std::vector<uint8_t>, MessageError> handle(std::span<const uint8_t> payload) = 0;
};

/**
 * @brief 类型化消息处理器（CRTP）
 * @tparam MessageType 消息类型
 * @tparam Handler 处理函数类型
 */
template <typename MessageType, typename Handler>
class MessageHandler : public IMessageHandler
{
public:
    explicit MessageHandler(Handler&& handler) : m_handler(std::forward<Handler>(handler)) {}

    std::expected<std::vector<uint8_t>, MessageError> handle(std::span<const uint8_t> payload) override
    {
        // 反序列化消息
        auto msgResult = MessageType::deserialize(payload);
        if (!msgResult)
        {
            return std::unexpected(msgResult.error());
        }

        // 调用处理函数
        return m_handler(*msgResult);
    }

private:
    Handler m_handler;
};

/**
 * @brief 消息分发器
 *
 * 使用示例：
 * MessageDispatcher dispatcher;
 *
 * // 注册消息处理器
 * dispatcher.registerHandler<CreateRoomRequest>([](const CreateRoomRequest& req) {
 *     // 处理逻辑
 *     auto resp = CreateRoomResponse::createSuccess(123);
 *     std::vector<uint8_t> buffer(256);
 *     auto data = resp.serialize(buffer);
 *     return std::vector<uint8_t>(data->begin(), data->end());
 * });
 *
 * // 分发收到的帧
 * auto frame = decodeFrame(receivedData);
 * if (frame) {
 *     auto [cmd, payload] = *frame;
 *     auto response = dispatcher.dispatch(cmd, payload);
 * }
 */
class MessageDispatcher
{
public:
    /**
     * @brief 注册消息处理器
     * @tparam MessageType 消息类型（必须有 CMD_ID 常量）
     * @tparam Handler 处理函数类型，签名为：std::expected<std::vector<uint8_t>, MessageError>(const MessageType&)
     * @param handler 处理函数
     */
    template <typename MessageType, typename Handler>
    void registerHandler(Handler&& handler)
    {
        constexpr uint16_t cmdId = MessageType::CMD_ID;
        m_handlers[cmdId] = std::make_unique<MessageHandler<MessageType, Handler>>(std::forward<Handler>(handler));
    }

    /**
     * @brief 分发消息到对应的处理器
     * @param cmdId 命令ID
     * @param payload 消息载荷
     * @return 成功返回响应数据，失败返回错误
     */
    std::expected<std::vector<uint8_t>, MessageError> dispatch(uint16_t cmdId, std::span<const uint8_t> payload)
    {
        auto it = m_handlers.find(cmdId);
        if (it == m_handlers.end())
        {
            return std::unexpected(MessageError::InvalidFormat); // 未注册的命令
        }

        return it->second->handle(payload);
    }

    /**
     * @brief 检查是否已注册处理器
     * @param cmdId 命令ID
     * @return true 如果已注册
     */
    bool hasHandler(uint16_t cmdId) const { return m_handlers.find(cmdId) != m_handlers.end(); }

    /**
     * @brief 注销处理器
     * @param cmdId 命令ID
     */
    void unregisterHandler(uint16_t cmdId) { m_handlers.erase(cmdId); }

    /**
     * @brief 清空所有处理器
     */
    void clear() { m_handlers.clear(); }

private:
    std::unordered_map<uint16_t, std::unique_ptr<IMessageHandler>> m_handlers;
};

/**
 * @brief 消息编码辅助函数
 * @tparam MessageType 消息类型
 * @param message 消息对象
 * @return 成功返回完整的帧数据（包含 FrameHeader），失败返回错误
 */
template <typename MessageType>
std::expected<std::vector<uint8_t>, MessageError> encodeMessage(const MessageType& message)
{
    std::vector<uint8_t> payloadBuffer(1024);

    // 序列化消息
    auto payloadResult = message.serialize(payloadBuffer);
    if (!payloadResult)
    {
        return std::unexpected(payloadResult.error());
    }

    // 编码为帧
    std::vector<uint8_t> frameBuffer(2048);
    auto frameResult = encodeFrame(frameBuffer, MessageType::CMD_ID, *payloadResult);
    if (!frameResult)
    {
        return std::unexpected(MessageError::SerializeFailed);
    }

    return std::vector<uint8_t>(frameResult->begin(), frameResult->end());
}

/**
 * @brief 消息解码辅助函数
 * @tparam MessageType 消息类型
 * @param frameData 完整的帧数据（包含 FrameHeader）
 * @return 成功返回消息对象，失败返回错误
 */
template <typename MessageType>
std::expected<MessageType, MessageError> decodeMessage(std::span<const uint8_t> frameData)
{
    // 解码帧
    auto frameResult = decodeFrame(frameData);
    if (!frameResult)
    {
        return std::unexpected(MessageError::InvalidFormat);
    }

    auto [cmd, payload] = *frameResult;

    // 验证命令ID
    if (cmd != MessageType::CMD_ID)
    {
        return std::unexpected(MessageError::InvalidFormat);
    }

    // 反序列化消息
    return MessageType::deserialize(payload);
}
