
#include "Platform.h"
#include "CellUtils.h"
#include "Strict.h"

String_ Cell::CoerceToString(const Cell_& src)
{
	if (Cell::IsString(src))
		return Cell::ToString(src);
	if (Cell::IsBool(src))
		return Cell::ToBool(src) ? "true" : "false";
	if (Cell::IsInt(src))
		return String::FromInt(Cell::ToInt(src));
	if (Cell::IsDouble(src))
		return String::FromDouble(Cell::ToDouble(src));
	if (Cell::IsDate(src))
		return Date::ToString(Cell::ToDate(src));
	if (Cell::IsDateTime(src))
		return DateTime::ToString(Cell::ToDateTime(src));
	assert(Cell::IsEmpty(src) || !"Unreachable -- bad cell type");
	return String_();
}

