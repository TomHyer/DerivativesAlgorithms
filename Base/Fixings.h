
// historical fixings available per index

#pragma once

#include <map>
#include "Storable.h"

class BASE_EXPORT Fixings_ : public Storable_
{
public:
	typedef std::map<DateTime_, double> vals_t;
	const vals_t vals_;

	Fixings_
		(const String_& index_name,
		 const vals_t& vals = vals_t())
	:
	Storable_("Fixings", index_name), vals_(vals) {}
};

