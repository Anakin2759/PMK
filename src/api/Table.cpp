#include "ui/api/Table.hpp"

#include <string>
#include <utility>
#include <vector>

#include "helper/Helper.hpp"

namespace ui::table
{

namespace
{
Registry& CurrentRegistry()
{
    return UiRuntime::current().registry();
}
}  // namespace

void SetColumns(ui::entity entity, int count, std::vector<std::string> headers)
{
    detail::table::SetColumns(CurrentRegistry(), detail::ToInternal(entity), count, std::move(headers));
}


void SetColumnWidths(ui::entity entity, std::vector<float> widths)
{
    detail::table::SetColumnWidths(CurrentRegistry(), detail::ToInternal(entity), std::move(widths));
}


void AddRow(ui::entity entity, std::vector<std::string> texts)
{
    detail::table::AddRow(CurrentRegistry(), detail::ToInternal(entity), std::move(texts));
}


void SetCell(ui::entity entity, int row, int col, std::string text)
{
    detail::table::SetCell(CurrentRegistry(), detail::ToInternal(entity), row, col, std::move(text));
}


void SetCellColor(ui::entity entity, int row, int col, Color textColor, Color bgColor)
{
    detail::table::SetCellColor(CurrentRegistry(), detail::ToInternal(entity), row, col, textColor, bgColor);
}


void ClearRows(ui::entity entity)
{
    detail::table::ClearRows(CurrentRegistry(), detail::ToInternal(entity));
}


void SetSelectedRow(ui::entity entity, int row)
{
    detail::table::SetSelectedRow(CurrentRegistry(), detail::ToInternal(entity), row);
}


void SetHeaderTextColor(ui::entity entity, Color color)
{
    detail::table::SetHeaderTextColor(CurrentRegistry(), detail::ToInternal(entity), color);
}


void SetColumnSizing(ui::entity entity, policies::TableColumnSizing sizing)
{
    detail::table::SetColumnSizing(CurrentRegistry(), detail::ToInternal(entity), sizing);
}


void SetMinColumnWidths(ui::entity entity, std::vector<float> minColumnWidths)
{
    detail::table::SetMinColumnWidths(
        CurrentRegistry(), detail::ToInternal(entity), std::move(minColumnWidths));
}


void SetMinRowHeight(ui::entity entity, float height)
{
    detail::table::SetMinRowHeight(CurrentRegistry(), detail::ToInternal(entity), height);
}


void SetRowHeight(ui::entity entity, float height)
{
    detail::table::SetRowHeight(CurrentRegistry(), detail::ToInternal(entity), height);
}


void SetCellWidget(ui::entity tableEntity, int row, int col, ui::entity widgetEntity)
{
    detail::table::SetCellWidget(
        CurrentRegistry(),
        detail::ToInternal(tableEntity),
        row,
        col,
        detail::ToInternal(widgetEntity));
}


}  // namespace ui::table
