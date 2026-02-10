/**
 * ************************************************************************
 *
 * @file NetWorkEvents.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 网络相关事件定义
    包含网络消息接收、登录、登出等事件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace events
{
struct NetworkMessageReceived
{
    // std::string message; // Deprecated
    std::vector<uint8_t> payload;
    uint32_t connectionId; // 标识发送者
};

struct Login
{
};

struct LoginSuccess
{
    uint32_t playerEntity; // 玩家实体ID
};

struct Logout
{
    uint32_t playerEntity; // 玩家实体ID
};

struct CreateRoom
{
    uint32_t owner; // 房主ID
};

struct JoinRoom
{
};

} // namespace events