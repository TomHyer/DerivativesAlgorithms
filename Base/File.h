
#pragma once

#include "Strings.h"
template<class T_> class Vector_;

namespace File
{
	BASE_EXPORT void Read
		(const String_& filename,
		 Vector_<String_>* dst);
}
