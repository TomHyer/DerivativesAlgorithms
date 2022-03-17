
// higher-level DateTime functionality
	// e.g., functions that might throw

#pragma once

#include "Export.h"

class String_;
class DateTime_;

namespace DateTime
{
	BASE_EXPORT DateTime_ FromString(const String_& src);	// date, space, colon-separated numbers
}

