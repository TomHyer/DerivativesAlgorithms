
// public interface to date functions

#include "__Platform.h"
#include "Strict.h"

namespace
{
/*IF--------------------------------------------------------------------------
public DateTime_Text
	Converts a datetime to a string representation (mostly for testing)
&inputs
t is time
	To be represented as string
&outputs
s is string
secs is integer
	Number of seconds in the unix epoch
-IF---------------------------------------------------------------------------*/

   void DateTime_Text(const DateTime_& t, String_* s, int* secs)
   {
	   *s = DateTime::ToString(t);
	   *secs = static_cast<int>(DateTime::MSec(t) / 1000);
   }
}  // leave local

#include "MG_DateTime_Text_public.inc"
