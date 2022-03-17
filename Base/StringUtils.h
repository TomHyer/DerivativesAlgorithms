
// higher-level string functionality (e.g., functions which may throw exceptions)

#pragma once

#include "Strings.h"

namespace String
{
	BASE_EXPORT bool ToBool(const String_& src);	// we are fairly generous, allow y/n, t/f, 1/0 as well as true/false
	BASE_EXPORT Vector_<bool> ToBoolVector(const String_& src);
}

