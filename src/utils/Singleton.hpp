/**
 * ************************************************************************
 *
 * @file Singleton.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-04-13
 * @version 0.1
 * @brief 单例模板，提供一个线程安全的单例实现，适用于需要全局访问且只允许一个实例的类，如日志记录器、配置管理器等
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
namespace ui::utils{
template <typename T>
class Singleton
{
public:
    /** @brief 获取单例引用；C++11 保证局部静态变量初始化的线程安全性。 */
    static T& getInstance()
    {
        static T instance;
        return instance;
    }

    /** @brief 禁用拷贝构造。 */
    Singleton(const Singleton&) = delete;
    /** @brief 禁用拷贝赋值。 */
    Singleton& operator=(const Singleton&) = delete;
    /** @brief 禁用移动构造。 */
    Singleton(Singleton&&) = delete;
    /** @brief 禁用移动赋值。 */
    Singleton& operator=(Singleton&&) = delete;

private:

    Singleton() = default;
};
}