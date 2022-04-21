
// Utilities for Cell and Table

#pragma once

#include <bitset>
#include "Cell.h"

namespace Cell
{
	template<class T_> bool IsType(const Cell_& c) { return std::holds_alternative<T_>(c.val_); }
	template<int> bool IsType(const Cell_& c) { return IsInt(c); }

	template<typename... OK_> struct TypeCheck_
	{
		bool operator()(const Cell_& src) const 
		{ 
			return (IsType<OK_>(src) || ...);
		}
	};

	BASE_EXPORT String_ CoerceToString(const Cell_& src);
}
