#include "ui/api/Table.hpp"

#include <string>
#include <utility>
#include <vector>

#include "helper/Helper.hpp"

namespace ui::table
{

void SetColumns(UiRuntime& runtime, ui::entity entity, int count, std::vector<std::string> headers)
{
    detail::table::SetColumns(runtime.registry(), detail::ToInternal(entity), count, std::move(headers));
}


void SetColumnWidths(UiRuntime& runtime, ui::entity entity, std::vector<float> widths)
{
    detail::table::SetColumnWidths(runtime.registry(), detail::ToInternal(entity), std::move(widths));
}


void AddRow(UiRuntime& runtime, ui::entity entity, std::vector<std::string> texts)
{
    detail::table::AddRow(runtime.registry(), detail::ToInternal(entity), std::move(texts));
}


void SetCell(UiRuntime& runtime, ui::entity entity, int row, int col, std::string text)
{
    detail::table::SetCell(runtime.registry(), detail::ToInternal(entity), row, col, std::move(text));
}


void SetCellColor(UiRuntime& runtime, ui::entity entity, int row, int col, Color textColor, Color bgColor)
{
    detail::table::SetCellColor(runtime.registry(), detail::ToInternal(entity), row, col, textColor, bgColor);
}


void ClearRows(UiRuntime& runtime, ui::entity entity)
{
    detail::table::ClearRows(runtime.registry(), detail::ToInternal(entity));
}


void SetSelectedRow(UiRuntime& runtime, ui::entity entity, int row)
{
    detail::table::SetSelectedRow(runtime.registry(), detail::ToInternal(entity), row);
}


void SetHeaderTextColor(UiRuntime& runtime, ui::entity entity, Color color)
{
    detail::table::SetHeaderTextColor(runtime.registry(), detail::ToInternal(entity), color);
}


void SetColumnSizing(UiRuntime& runtime, ui::entity entity, policies::TableColumnSizing sizing)
{
    detail::table::SetColumnSizing(runtime.registry(), detail::ToInternal(entity), sizing);
}


void SetMinColumnWidths(UiRuntime& runtime, ui::entity entity, std::vector<float> minColumnWidths)
{
    detail::table::SetMinColumnWidths(
        runtime.registry(), detail::ToInternal(entity), std::move(minColumnWidths));
}


void SetMinRowHeight(UiRuntime& runtime, ui::entity entity, float height)
{
    detail::table::SetMinRowHeight(runtime.registry(), detail::ToInternal(entity), height);
}


void SetRowHeight(UiRuntime& runtime, ui::entity entity, float height)
{
    detail::table::SetRowHeight(runtime.registry(), detail::ToInternal(entity), height);
}


void SetCellWidget(
    UiRuntime& runtime, ui::entity tableEntity, int row, int col, ui::entity widgetEntity)
{
    detail::table::SetCellWidget(
        runtime.registry(),
        detail::ToInternal(tableEntity),
        row,
        col,
        detail::ToInternal(widgetEntity));
}


}  // namespace ui::table
