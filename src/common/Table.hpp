#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "Policies.hpp"
#include "components/Data.hpp"

namespace ui::table
{

[[nodiscard]] inline float MinColumnWidthAt(const components::TableInfo& info, int columnIndex)
{
    if (!info.minColumnWidths.empty() && columnIndex < static_cast<int>(info.minColumnWidths.size()))
    {
        return std::max(0.0F, info.minColumnWidths.at(static_cast<size_t>(columnIndex)));
    }
    return 0.0F;
}

[[nodiscard]] inline std::vector<float> ComputeEqualColumnWidths(const components::TableInfo& info, float visibleWidth)
{
    const int columnCount = info.columnCount;
    std::vector<float> widths(static_cast<size_t>(columnCount), 0.0F);
    const float equalWidth = visibleWidth / static_cast<float>(columnCount);
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
    {
        widths.at(static_cast<size_t>(columnIndex)) = std::max(equalWidth, MinColumnWidthAt(info, columnIndex));
    }
    return widths;
}

[[nodiscard]] inline std::vector<float> ComputeFixedColumnWidths(const components::TableInfo& info, float visibleWidth)
{
    const int columnCount = info.columnCount;
    if (info.columnWidths.empty() || !std::cmp_equal(info.columnWidths.size(), columnCount))
    {
        return ComputeEqualColumnWidths(info, visibleWidth);
    }

    std::vector<float> widths(static_cast<size_t>(columnCount), 0.0F);
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
    {
        widths.at(static_cast<size_t>(columnIndex)) =
            std::max(info.columnWidths.at(static_cast<size_t>(columnIndex)), MinColumnWidthAt(info, columnIndex));
    }
    return widths;
}

[[nodiscard]] inline float TotalColumnWeight(const components::TableInfo& info)
{
    float totalWeight = 0.0F;
    if (!info.columnWidths.empty() && std::cmp_equal(info.columnWidths.size(), info.columnCount))
    {
        for (float columnWeight : info.columnWidths)
        {
            totalWeight += std::max(0.0F, columnWeight);
        }
    }
    return totalWeight > 0.0F ? totalWeight : static_cast<float>(info.columnCount);
}

[[nodiscard]] inline std::vector<float> ComputeProportionalColumnWidths(const components::TableInfo& info,
                                                                        float visibleWidth)
{
    const int columnCount = info.columnCount;
    const float totalWeight = TotalColumnWeight(info);
    std::vector<float> widths(static_cast<size_t>(columnCount), 0.0F);
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
    {
        float columnWeight = 1.0F;
        if (!info.columnWidths.empty() && columnIndex < static_cast<int>(info.columnWidths.size()))
        {
            columnWeight = std::max(0.0F, info.columnWidths.at(static_cast<size_t>(columnIndex)));
        }
        const float proportionalWidth = (columnWeight / totalWeight) * visibleWidth;
        widths.at(static_cast<size_t>(columnIndex)) = std::max(proportionalWidth, MinColumnWidthAt(info, columnIndex));
    }
    return widths;
}

[[nodiscard]] inline std::vector<float> ComputeAdaptiveColumnWidths(const components::TableInfo& info,
                                                                    float visibleWidth)
{
    const int columnCount = info.columnCount;
    std::vector<float> widths(static_cast<size_t>(columnCount), 0.0F);
    float fixedTotal = 0.0F;
    int flexCount = 0;
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
    {
        float fixedWidth = 0.0F;
        if (!info.columnWidths.empty() && columnIndex < static_cast<int>(info.columnWidths.size()))
        {
            fixedWidth = info.columnWidths.at(static_cast<size_t>(columnIndex));
        }
        if (fixedWidth > 0.0F)
        {
            widths.at(static_cast<size_t>(columnIndex)) = std::max(fixedWidth, MinColumnWidthAt(info, columnIndex));
            fixedTotal += widths.at(static_cast<size_t>(columnIndex));
        }
        else
        {
            ++flexCount;
        }
    }

    const float remainingWidth = std::max(0.0F, visibleWidth - fixedTotal);
    const float flexWidth = (flexCount > 0) ? (remainingWidth / static_cast<float>(flexCount)) : 0.0F;
    for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex)
    {
        const bool hasFixedWidth = !info.columnWidths.empty()
                                && columnIndex < static_cast<int>(info.columnWidths.size())
                                && info.columnWidths.at(static_cast<size_t>(columnIndex)) > 0.0F;
        if (!hasFixedWidth)
        {
            widths.at(static_cast<size_t>(columnIndex)) = std::max(flexWidth, MinColumnWidthAt(info, columnIndex));
        }
    }
    return widths;
}

[[nodiscard]] inline std::vector<float> ComputeColumnWidths(const components::TableInfo& info, float tableWidth)
{
    const int columnCount = info.columnCount;
    if (columnCount <= 0)
    {
        return {};
    }
    const float visibleWidth = std::max(0.0F, tableWidth);

    switch (info.columnSizing)
    {
        case policies::TableColumnSizing::FIXED:
            return ComputeFixedColumnWidths(info, visibleWidth);
        case policies::TableColumnSizing::PROPORTIONAL:
            return ComputeProportionalColumnWidths(info, visibleWidth);
        case policies::TableColumnSizing::ADAPTIVE:
            return ComputeAdaptiveColumnWidths(info, visibleWidth);
        case policies::TableColumnSizing::EQUAL:
        default:
            return ComputeEqualColumnWidths(info, visibleWidth);
    }
}

} // namespace ui::table