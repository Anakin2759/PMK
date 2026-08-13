/**
 * SystemManager implementation
 */

#include "SystemManager.hpp"
#include "core/UiRuntime.hpp"
#include "utils/Logger.hpp"
#include "utils/Registry.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>
#include <utility>

// 引入所有子系统头文件
#include "systems/RenderSystem.hpp"
#include "systems/TweenSystem.hpp"
#include "systems/InteractionSystem.hpp"
#include "systems/PlatformWindowSystem.hpp"
#include "systems/TextInputSystem.hpp"
#include "systems/HitTestSystem.hpp"
#include "systems/LayoutSystem.hpp"
#include "systems/StateSystem.hpp"  // 保持与 Application.h 中的一致
#include "systems/ActionSystem.hpp"
#include "systems/TimerSystem.hpp"
#include "systems/ShortcutSystem.hpp"
#include "systems/ThemeSystem.hpp"
namespace ui
{
SystemManager::SystemManager(UiRuntime* runtime, bool registerBuiltIns) : m_runtime(runtime)
{
    if (registerBuiltIns)
    {
        registerBuiltInSystems();
    }
    m_runtime->logger().info("[SystemManager] 系统管理器初始化完成，已注册 {} 个系统", m_systems.size());
}

void SystemManager::registerBuiltInSystems()
{
    auto* runtime = m_runtime;
    m_runtime->logger().info("[SystemManager] 正在注册 PlatformWindowSystem...");
    m_systems.emplace_back(systems::PlatformWindowSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 InteractionSystem...");
    m_systems.emplace_back(systems::InteractionSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 TextInputSystem...");
    m_systems.emplace_back(systems::TextInputSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 HitTestSystem...");
    m_systems.emplace_back(systems::HitTestSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 TweenSystem...");
    m_systems.emplace_back(systems::TweenSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 LayoutSystem...");
    m_systems.emplace_back(systems::LayoutSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 RenderSystem...");
    m_systems.emplace_back(systems::RenderSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 StateSystem...");
    m_systems.emplace_back(systems::StateSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 ActionSystem...");
    m_systems.emplace_back(systems::ActionSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 TimerSystem...");
    m_systems.emplace_back(systems::TimerSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 ThemeSystem...");
    m_systems.emplace_back(systems::ThemeSystem{*runtime});

    m_runtime->logger().info("[SystemManager] 正在注册 ShortcutSystem...");
    m_systems.emplace_back(systems::ShortcutSystem{*runtime});
}

SystemManager::~SystemManager()
{
    unregisterAllHandlers();
}

void SystemManager::registerAllHandlers()
{
    if (m_state != State::ASSEMBLING)
    {
        return;
    }

    // OP-22: 按阶段排序，确保事件处理器按 Input→Logic→Animation→Layout→Render→Frame 顺序订阅
    // entt::poly 是 move-only 类型，无法直接用于 stable_sort 比较器；改用索引排序后重组
    std::vector<std::size_t> indices(m_systems.size());
    std::ranges::iota(indices, std::size_t{0});
    std::ranges::stable_sort(indices, [this](std::size_t leftIndex, std::size_t rightIndex)
                             { return m_systems.at(leftIndex)->getPhase() < m_systems.at(rightIndex)->getPhase(); });
    decltype(m_systems) sorted;
    sorted.reserve(m_systems.size());
    for (auto systemIndex : indices)
    {
        sorted.push_back(std::move(m_systems.at(systemIndex)));
    }
    m_systems = std::move(sorted);
    for (auto& system : m_systems)
    {
        system->registerHandlers();
    }
    m_state = State::REGISTERED;
}

void SystemManager::unregisterAllHandlers()
{
    if (m_state != State::REGISTERED)
    {
        return;
    }

    for (auto& system : m_systems)
    {
        system->unregisterHandlers();
    }
    m_state = State::STOPPED;
}

void SystemManager::pollInput()
{
    for (auto& system : m_systems)
    {
        system->pollInput();
    }
}

std::vector<interface::SystemPhase> SystemManager::getSystemPhases()
{
    std::vector<interface::SystemPhase> phases;
    phases.reserve(m_systems.size());
    for (auto& system : m_systems)
    {
        phases.push_back(system->getPhase());
    }
    return phases;
}

bool SystemManager::removeSystem(uint8_t index)
{
    if (index >= m_systems.size())
    {
        return false;
    }

    auto system = m_systems.begin() + index;
    if (m_state == State::REGISTERED)
    {
        (*system)->unregisterHandlers();
    }
    m_systems.erase(system);
    return true;
}

void SystemManager::clear()
{
    m_runtime->registry().clear();
}

}  // namespace ui
