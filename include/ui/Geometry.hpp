#pragma once

namespace ui
{

/**
 * @brief 不依赖渲染后端和数学库的轴对齐矩形值类型。
 */
struct GeometryRect
{
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    /**
     * @brief 判断点是否位于矩形闭区间内。
     */
    [[nodiscard]] constexpr bool Contains(float pointX, float pointY) const noexcept
    {
        return pointX >= x && pointX <= x + width && pointY >= y && pointY <= y + height;
    }
};

/**
 * @brief 纵向滚动条的公开几何快照。
 *
 * 该类型仅描述一次计算结果，不是 ECS 组件，也不依赖 EnTT 或 Eigen。
 */
struct VerticalScrollbarGeometry
{
    bool visible = false;
    GeometryRect containerRect;
    GeometryRect viewportRect;
    GeometryRect trackRect;
    GeometryRect thumbRect;
    float viewportHeight = 0.0F;
    float contentHeight = 0.0F;
    float trackHeight = 0.0F;
    float thumbHeight = 0.0F;
    float maxScroll = 0.0F;
};

} // namespace ui
