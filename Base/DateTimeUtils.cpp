
#include "Platform.h"
#include "DateTimeUtils.h"
#include "Strict.h"

#include "Algorithms.h"
#include "Numerics.h"
#include "DateTime.h"
#include "DateUtils.h"
#include "Exceptions.h"

DateTime_ DateTime::FromString(const String_& src)
{
	NOTICE(src);
	auto space = src.find(' ');
	const Date_ date = Date::FromString(src.substr(0, space));
	if (space == String_::npos)
		return DateTime_(date, 0);
	// split remainder on ':'
	Vector_<String_> tParts = String::Split(src.substr(space + 1), ':', true);
	REQUIRE(tParts.size() >= 2 && tParts.size() <= 3, "Expected hh:mm or hh:mm:ss time");
	Vector_<int> hms = Apply(String::ToInt, tParts);
	REQUIRE(*MinElement(hms) >= 0 && hms[0] < 24 && *MaxElement(hms) < 60, "Hour/minute/second out of bounds");
	return DateTime_(date, hms[0], hms[1], hms.size() > 2 ? hms[2] : 0);
}

DateTime_ DateTime::FromExcel(double d)
{
	const int id = AsInt(d);
	return DateTime_(Date::FromExcel(id), d - id);
}
