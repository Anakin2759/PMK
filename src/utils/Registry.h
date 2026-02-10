#pragma once

#include <entt/entt.hpp>
namespace utils
{
class Registry
{
public:
    // 获取单例实例
    static ::entt::registry& getInstance()
    {
        static ::entt::registry instance;
        return instance;
    }

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

private:
    Registry() = default;
    ~Registry() = default;
};
} // namespace utils