/**
 * ************************************************************************
 *
 * @file CommandID.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 协议命令 ID 定义（共享层）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>

/**
 * @brief 协议命令 ID 命名空间
 * @note 请求命令使用 0x1000-0x1FFF，响应命令使用 0x2000-0x2FFF
 */
namespace CommandID
{
// ==================== 连接与会话管理 (0x1000-0x10FF) ====================
constexpr uint16_t CONNECTED = 0x1001;  // 客户端连接请求
constexpr uint16_t HEARTBEAT = 0x1002;  // 心跳
constexpr uint16_t DISCONNECT = 0x1003; // 断开连接

// ==================== 房间管理 (0x1100-0x11FF) ====================
constexpr uint16_t CREATE_ROOM_REQ = 0x1100;  // 创建房间请求
constexpr uint16_t CREATE_ROOM_RESP = 0x2100; // 创建房间响应
constexpr uint16_t JOIN_ROOM = 0x1101;        // 加入房间
constexpr uint16_t LEAVE_ROOM = 0x1102;       // 离开房间
constexpr uint16_t ROOM_LIST = 0x1103;        // 房间列表

// ==================== 游戏逻辑 (0x1200-0x12FF) ====================
constexpr uint16_t USE_CARD_REQ = 0x1200;      // 使用卡牌请求
constexpr uint16_t USE_CARD_RESP = 0x2200;     // 使用卡牌响应
constexpr uint16_t DISCARD_CARD_REQ = 0x1201;  // 弃牌请求
constexpr uint16_t DISCARD_CARD_RESP = 0x2201; // 弃牌响应
constexpr uint16_t SETTLEMENT_REQ = 0x1202;    // 结算请求
constexpr uint16_t SETTLEMENT_RESP = 0x2202;   // 结算响应

// ==================== 聊天与社交 (0x1300-0x13FF) ====================
constexpr uint16_t SEND_MESSAGE_REQ = 0x1300;  // 发送消息请求
constexpr uint16_t SEND_MESSAGE_RESP = 0x2300; // 发送消息响应

} // namespace CommandID
