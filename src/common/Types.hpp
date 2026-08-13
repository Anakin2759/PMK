/**
 * ************************************************************************
 *
 * @file Types.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-13
 * @version 0.1
 * @brief UI模块核心类型定义
 *
 * 使用Eigen向量类型替代ImVec，提供统一的数学类型支持。
 * 包含颜色、向量、矩阵等基础类型定义和转换工具。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include "common/EigenConversions.hpp"
#include "ui/Color.hpp"      // IWYU pragma: export -- legacy aggregate include
#include "ui/MathTypes.hpp"  // IWYU pragma: export -- legacy aggregate include

namespace ui
{

// ===================== 基础向量类型 =====================

/**
 * @brief 3D向量类型
 */
using Vec3 = Eigen::Vector3f;

/**
 * @brief 2x2矩阵类型
 */
using Mat2 = Eigen::Matrix2f;

/**
 * @brief 3x3矩阵类型（用于2D变换）
 */
using Mat3 = Eigen::Matrix3f;

/**
 * @brief 4x4矩阵类型（用于3D变换）
 */
using Mat4 = Eigen::Matrix4f;

/**
 * @brief 仿射变换类型（2D）
 */
using Transform2D = Eigen::Affine2f;

/**
 * @brief 仿射变换类型（3D）
 */
using Transform3D = Eigen::Affine3f;

// ===================== 边距/内边距类型 =====================

/**
 * @brief 边距结构体（Top, Right, Bottom, Left顺序）
 */
struct EdgeInsets
{
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
    float left = 0.0F;

    constexpr EdgeInsets() = default;
    constexpr explicit EdgeInsets(float all) : top(all), right(all), bottom(all), left(all)
    {
    }
    constexpr EdgeInsets(float vertical, float horizontal)
        : top(vertical), right(horizontal), bottom(vertical), left(horizontal)
    {
    }
    constexpr EdgeInsets(float topVal, float rightVal, float bottomVal, float leftVal)
        : top(topVal), right(rightVal), bottom(bottomVal), left(leftVal)
    {
    }

    /**
     * @brief 从Vec4构造（Top, Right, Bottom, Left）
     */
    explicit EdgeInsets(const Vec4& vec) : top(vec.x()), right(vec.y()), bottom(vec.z()), left(vec.w())
    {
    }

    /**
     * @brief 转换为Vec4
     */
    [[nodiscard]] Vec4 toVec4() const
    {
        return Vec4(top, right, bottom, left);
    }

    [[nodiscard]] float horizontal() const
    {
        return left + right;
    }
    [[nodiscard]] float vertical() const
    {
        return top + bottom;
    }
};

// ===================== 工具函数 =====================

/**
 * @brief 创建Vec2
 */
inline Vec2 MakeVec2(float vecX, float vecY)
{
    return {vecX, vecY};
}

/**
 * @brief 创建Vec4
 */
inline Vec4 MakeVec4(float vecX, float vecY, float vecZ, float vecW)
{
    return Vec4(vecX, vecY, vecZ, vecW);
}

/**
 * @brief 线性插值
 */
inline Vec2 Lerp(const Vec2& from, const Vec2& dest, float alpha)
{
    return from + ((dest - from) * alpha);
}

inline float Lerp(float from, float dest, float alpha)
{
    return from + ((dest - from) * alpha);
}

/**
 * @brief 2D旋转矩阵
 */
inline Mat2 Rotation2D(float angleRadians)
{
    const float cosA = std::cos(angleRadians);
    const float sinA = std::sin(angleRadians);
    Mat2 mat;
    mat << cosA, -sinA, sinA, cosA;
    return mat;
}

/**
 * @brief 2D缩放矩阵
 */
inline Mat2 Scale2D(float scaleX, float scaleY)
{
    Mat2 mat;
    mat << scaleX, 0, 0, scaleY;
    return mat;
}

/**
 * @brief 创建2D仿射变换
 */
inline Transform2D MakeTransform2D(const Vec2& translation, float rotation = 0.0F, const Vec2& scale = Vec2(1, 1))
{
    Transform2D transform = Transform2D::Identity();
    transform.translate(detail::eigen::ToEigen(translation));
    transform.rotate(rotation);
    transform.scale(detail::eigen::ToEigen(scale));
    return transform;
}

// ===================== 通用回调包装 =====================

/**
 * @brief 零开销单态回调包装器
 *
 * 直接以模板参数存储可调用对象，无类型擦除、无堆分配、无虚调用。
 * operator() 通过 std::invoke 完美转发参数，支持任意返回类型。
 *
 * 适用场景：局部传参、模板上下文、性能敏感路径。
 * 不适用于异构容器/结构体字段（需类型擦除时改用 VoidCallback）。
 *
 * 用法：
 *   auto cb = make_callback([](int x) { return x * 2; });
 *   int result = cb(21); // 42
 */
template <typename Func>
class CallbackWrapper
{
   public:
    explicit CallbackWrapper(Func func) : m_callback(std::move(func))
    {
    }

    ~CallbackWrapper() = default;

    CallbackWrapper(CallbackWrapper&&) noexcept = default;
    CallbackWrapper& operator=(CallbackWrapper&&) noexcept = default;

    CallbackWrapper(const CallbackWrapper&) = default;
    CallbackWrapper& operator=(const CallbackWrapper&) = default;

    template <typename... Args>
        requires std::invocable<Func, Args...>
    decltype(auto) operator()(Args&&... args)
    {
        return std::invoke(m_callback, std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::invocable<const Func, Args...>
    decltype(auto) operator()(Args&&... args) const
    {
        return std::invoke(m_callback, std::forward<Args>(args)...);
    }

   private:
    Func m_callback;
};

/// 推导辅助，免写模板参数：auto cb = MakeCallback([]{ ... });
template <typename Func>
auto MakeCallback(Func&& func)
{
    return CallbackWrapper<std::decay_t<Func>>(std::forward<Func>(func));
}

/// 用于结构体字段/容器的类型擦除无参回调别名（需要异构存储时使用）
using VoidCallback = std::function<void()>;

}  // namespace ui
