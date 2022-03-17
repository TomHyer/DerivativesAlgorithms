
// Utilities for Cell and Table

#pragma once

#include <bitset>
#include "Cell.h"

namespace Cell
{
	struct BASE_EXPORT TypeCheck_
	{
	   std::bitset<static_cast<int>(Type_::N_TYPES)> ret_;

	   TypeCheck_ Add(const Type_& bit) const
	   {
		  TypeCheck_ ret(*this); ret.ret_.set(static_cast<int>(bit)); return ret;
	   }
	   TypeCheck_ String() const { return Add(Type_::STRING); }
	   TypeCheck_ Number() const { return Add(Type_::NUMBER); }
	   TypeCheck_ Date() const { return Add(Type_::DATE); }
	   TypeCheck_ DateTime() const { return Add(Type_::DATETIME); }
	   TypeCheck_ Boolean() const { return Add(Type_::BOOLEAN); }
	   TypeCheck_ Empty() const { return Add(Type_::EMPTY); }

	   bool operator()(const Cell_& c) const
	   {
		  return ret_[static_cast<int>(c.type_)];
	   }
	};

	BASE_EXPORT String_ CoerceToString(const Cell_& src);
}

