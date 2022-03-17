
// higher-level date functions (e.g., those that might throw)
#include "Export.h"

class String_;
class Date_;

namespace Date
{
	BASE_EXPORT Date_ FromString(const String_& src);	// tries our best to recognize the string -- rejects both mm/dd/yyyy and dd/mm/yyyy due to ambiguity
	BASE_EXPORT int MonthFromFutureCode(char code);
}
