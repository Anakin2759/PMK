#include "Table.hpp"

#include "Scale.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "common/Policies.hpp"
#include "common/Tags.hpp"
#include "core/RuntimeFacade.hpp"
#include "detail/EntityCast.hpp"
#include "common/Types.hpp"
#include "common/components/Data.hpp"
#include "common/components/Layout.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"

namespace ui::table
{

namespace
{

[[nodiscard]] Registry& CurrentRegistry()
{
    return RuntimeFacade::current().registry();
}

} // namespace

void SetColumns(ui::entity entity, int count, std::vector<std::string> headers)
{
    auto& info = CurrentRegistry().get_or_emplace<components::TableInfo>(entity);
    info.columnCount = count;
    info.headers = std::move(headers);
    // 调整已有行的列数
    for (auto& row : info.cells)
    {
        row.resize(static_cast<size_t>(count));
    }
}

void SetColumnWidths(ui::entity entity, std::vector<float> widths)
{
    auto& info = CurrentRegistry().get_or_emplace<components::TableInfo>(entity);
    if (info.columnSizing == policies::TableColumnSizing::FIXED
        || info.columnSizing == policies::TableColumnSizing::ADAPTIVE)
    {
        for (auto& width : widths)
        {
            width = scale::Metric(width);
        }
    }
    info.columnWidths = std::move(widths);
}

void AddRow(ui::entity entity, std::vector<std::string> texts)
{
    auto& info = CurrentRegistry().get_or_emplace<components::TableInfo>(entity);
    std::vector<components::TableCell> row;
    row.resize(static_cast<size_t>(info.columnCount));
    for (int columnIndex = 0; columnIndex < info.columnCount && std::cmp_less(columnIndex, texts.size()); ++columnIndex)
    {
        row.at(static_cast<size_t>(columnIndex)).text = std::move(texts.at(static_cast<size_t>(columnIndex)));
    }
    info.cells.push_back(std::move(row));
}

void SetCell(ui::entity entity, int row, int col, std::string text)
{
    auto* info = CurrentRegistry().try_get<components::TableInfo>(entity);
    if (info == nullptr) return;
    if (row < 0 || std::cmp_greater_equal(row, info->cells.size())) return;
    if (col < 0 || col >= info->columnCount) return;
    info->cells.at(static_cast<size_t>(row)).at(static_cast<size_t>(col)).text = std::move(text);
}

void SetCellColor(ui::entity entity, int row, int col, Color textColor, Color bgColor)
{
    auto* info = CurrentRegistry().try_get<components::TableInfo>(entity);
    if (info == nullptr) return;
    if (row < 0 || std::cmp_greater_equal(row, info->cells.size())) return;
    if (col < 0 || col >= info->columnCount) return;
    auto& cell = info->cells.at(static_cast<size_t>(row)).at(static_cast<size_t>(col));
    cell.textColor = textColor;
    cell.bgColor = bgColor;
}

void ClearRows(ui::entity entity)
{
    auto* info = CurrentRegistry().try_get<components::TableInfo>(entity);
    if (info != nullptr)
    {
        info->cells.clear();
        info->selectedRow = -1;
    }
}

void SetSelectedRow(ui::entity entity, int row)
{
    auto& info = CurrentRegistry().get_or_emplace<components::TableInfo>(entity);
    info.selectedRow = row;
}

void SetHeaderTextColor(ui::entity entity, Color color)
{
    auto& info = CurrentRegistry().get_or_emplace<components::TableInfo>(entity);
    info.headerTextColor = color;
}

void SetColumnSizing(ui::entity entity, policies::TableColumnSizing sizing)
{
    CurrentRegistry().get_or_emplace<components::TableInfo>(entity).columnSizing = sizing;
}

void SetMinColumnWidths(ui::entity entity, std::vector<float> minWidths)
{
    for (auto& width : minWidths)
    {
        width = scale::Metric(width);
    }
    CurrentRegistry().get_or_emplace<components::TableInfo>(entity).minColumnWidths = std::move(minWidths);
}

void SetMinRowHeight(ui::entity entity, float height)
{
    CurrentRegistry().get_or_emplace<components::TableInfo>(entity).minRowHeight =
        std::max(0.0F, scale::Metric(height));
}

void SetRowHeight(ui::entity entity, float height)
{
    CurrentRegistry().get_or_emplace<components::TableInfo>(entity).rowHeight = std::max(0.0F, scale::Metric(height));
}

void SetCellWidget(ui::entity tableEntity, int row, int col, ui::entity widgetEntity)
{
    auto& reg = CurrentRegistry();
    const entt::entity tableInternal = detail::ToInternal(tableEntity);
    const entt::entity widgetInternal = detail::ToInternal(widgetEntity);
    auto* info = reg.try_get<components::TableInfo>(tableInternal);
    if (info == nullptr) return;
    if (row < 0 || std::cmp_greater_equal(row, info->cells.size())) return;
    if (col < 0 || col >= info->columnCount) return;
    if (!reg.valid(widgetInternal)) return;

    auto& cell = info->cells.at(static_cast<size_t>(row)).at(static_cast<size_t>(col));

    // 替换旧实体：从 Hierarchy 中移除旧 widget
    if (cell.cellEntity != entt::null && cell.cellEntity != widgetInternal && reg.valid(cell.cellEntity))
    {
        auto* parentHierarchy = reg.try_get<components::Hierarchy>(tableInternal);
        if (parentHierarchy != nullptr)
        {
            auto& children = parentHierarchy->children;
            std::erase(children, cell.cellEntity);
        }
        if (auto* childHierarchy = reg.try_get<components::Hierarchy>(cell.cellEntity))
        {
            childHierarchy->parent = entt::null;
        }
    }

    cell.cellEntity = widgetInternal;

    // 标记为 TableCellWidget，使其跳过 Yoga 布局
    reg.emplace_or_replace<components::TableCellWidgetTag>(widgetInternal);

    // 加入表格的 Hierarchy（若不已存在）
    auto& parentHierarchy = reg.get_or_emplace<components::Hierarchy>(tableInternal);
    const bool alreadyChild =
        std::ranges::find(parentHierarchy.children, widgetInternal) != parentHierarchy.children.end();
    if (!alreadyChild)
    {
        auto& childHierarchy = reg.get_or_emplace<components::Hierarchy>(widgetInternal);
        childHierarchy.parent = tableInternal;
        reg.remove<components::RootTag>(widgetInternal);
        parentHierarchy.children.push_back(widgetInternal);
    }
}

} // namespace ui::table
