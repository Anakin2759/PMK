#include "Table.hpp"

#include <string>
#include <utility>
#include <vector>

#include "helper/Helper.hpp"

namespace ui::table
{

void SetColumns(ui::entity entity, int count, std::vector<std::string> headers)
{
    detail::table::SetColumns(detail::ToInternal(entity), count, std::move(headers));
}

void SetColumnWidths(ui::entity entity, std::vector<float> widths)
{
    detail::table::SetColumnWidths(detail::ToInternal(entity), std::move(widths));
}

void AddRow(ui::entity entity, std::vector<std::string> texts)
{
    detail::table::AddRow(detail::ToInternal(entity), std::move(texts));
}

void SetCell(ui::entity entity, int row, int col, std::string text)
{
    detail::table::SetCell(detail::ToInternal(entity), row, col, std::move(text));
}

void SetCellColor(ui::entity entity, int row, int col, Color textColor, Color bgColor)
{
    detail::table::SetCellColor(detail::ToInternal(entity), row, col, textColor, bgColor);
}

void ClearRows(ui::entity entity)
{
    detail::table::ClearRows(detail::ToInternal(entity));
}

void SetSelectedRow(ui::entity entity, int row)
{
    detail::table::SetSelectedRow(detail::ToInternal(entity), row);
}

void SetHeaderTextColor(ui::entity entity, Color color)
{
    detail::table::SetHeaderTextColor(detail::ToInternal(entity), color);
}

void SetColumnSizing(ui::entity entity, policies::TableColumnSizing sizing)
{
    detail::table::SetColumnSizing(detail::ToInternal(entity), sizing);
}

void SetMinColumnWidths(ui::entity entity, std::vector<float> minWidths)
{
    detail::table::SetMinColumnWidths(detail::ToInternal(entity), std::move(minWidths));
}

void SetMinRowHeight(ui::entity entity, float height)
{
    detail::table::SetMinRowHeight(detail::ToInternal(entity), height);
}

void SetRowHeight(ui::entity entity, float height)
{
    detail::table::SetRowHeight(detail::ToInternal(entity), height);
}

void SetCellWidget(ui::entity tableEntity, int row, int col, ui::entity widgetEntity)
{
    detail::table::SetCellWidget(detail::ToInternal(tableEntity), row, col, detail::ToInternal(widgetEntity));
}

} // namespace ui::table
