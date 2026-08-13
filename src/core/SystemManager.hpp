/**
 * ************************************************************************
 *
 * @file SystemManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief UI系统管理器 - 基于ECS架构
 *
 * 负责管理所有UI相关的ECS系统，并按正确的顺序调用它们的更新流程。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <entt/entt.hpp>
#include <utility>
#include <vector>

// 引入系统接口
#include "interface/ISystem.hpp"

namespace ui
{

class UiRuntime;

/**
 * @brief UI系统管理器：定义ECS系统的执行流程
 * 使用 entt::poly 实现系统的动态管理
 */
class SystemManager
{
   public:
    /**
     * @brief SystemManager 生命周期状态。
     */
    enum class State : uint8_t
    {
        ASSEMBLING,  ///< 允许装配 System，尚未连接事件处理器。
        REGISTERED,  ///< 所有 System 已连接事件处理器。
        STOPPED,     ///< 已完成注销，不允许再次装配或注册。
    };

    // 构造函数：初始化所有子系统（注入 Registry 和 Dispatcher 以替代全局单例访问）
    explicit SystemManager(UiRuntime* runtime, bool registerBuiltIns = true);

    ~SystemManager();

    // 禁用拷贝和移动
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
    SystemManager(SystemManager&&) = delete;
    SystemManager& operator=(SystemManager&&) = delete;

    /**
     * @brief 注册所有系统的事件处理器
     */
    void registerAllHandlers();

    /**
     * @brief 注销所有系统的事件处理器
     */
    void unregisterAllHandlers();

    /**
     * @brief 调用所有系统的 pollInput()
     * 实际执行者为 InteractionSystem（SDL 轮询），其予系统为 no-op。
     */
    void pollInput();

    /**
     * @brief 动态添加系统
     * @tparam T 系统类型
     * @param system 系统实例
     */
    template <typename T>
    [[nodiscard]] bool addSystem(T&& system)
    {
        if (m_state != State::ASSEMBLING)
        {
            return false;
        }
        m_systems.emplace_back(std::forward<T>(system));
        return true;
    }

    /**
     * @brief 在事件处理器注册前注入系统，供测试或可选系统装配使用。
     * @tparam T 系统类型
     * @param system 系统实例
     * @note 必须在 registerAllHandlers() 之前调用；注册后追加系统不会自动订阅事件。
     */
    template <typename T>
    [[nodiscard]] bool addSystemBeforeRegister(T&& system)
    {
        return addSystem(std::forward<T>(system));
    }

    /**
     * @brief 移除指定索引的系统
     * @param index 系统索引
     */
    [[nodiscard]] bool removeSystem(uint8_t index);
    /**
     * @brief 获取系统数量
     */
    [[nodiscard]] size_t getSystemCount() const
    {
        return m_systems.size();
    }

    /**
     * @brief 获取当前生命周期状态。
     */
    [[nodiscard]] State getState() const noexcept
    {
        return m_state;
    }

    /**
     * @brief 返回当前 System 的阶段快照，供诊断和契约测试使用。
     */
    [[nodiscard]] std::vector<interface::SystemPhase> getSystemPhases();

    /**
     * @brief 清空所有UI元素 携带uitag的组件
     */
    void clear();  // NOLINT(readability-convert-member-functions-to-static)
   private:
    /**
     * @brief 注册框架内建系统。
     */
    void registerBuiltInSystems();

    // 使用 entt::poly 动态管理所有系统
    std::vector<entt::poly<interface::ISystem>> m_systems;
    UiRuntime* m_runtime = nullptr;
    State m_state = State::ASSEMBLING;
};
}  // namespace ui