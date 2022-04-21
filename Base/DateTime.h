
// compact datetime -- stores dates as in class Date_, plus times with ~1 second resolution

#pragma once

#include "Date.h"

class BASE_EXPORT DateTime_
{
	Date_ date_;
	uint16_t frac_;
public:
	DateTime_() : frac_(0) {}	// midnight on invalid date
	explicit DateTime_(const Date_& date, double frac = 0.0);
	explicit DateTime_(long long msec);	// convert from unix-era time
	DateTime_(const Date_& date, int hour, int minute = 0, int second = 0);

	bool operator==(const DateTime_&) const = default;
	auto operator<=>(const DateTime_&) const = default;

	inline Date_ Date() const { return date_; }
	inline double Frac() const { return 0.0000152587890625 * frac_; }
};

BASE_EXPORT double operator-(const DateTime_& lhs, const DateTime_& rhs);

namespace DateTime
{
	BASE_EXPORT int Hour(const DateTime_& dt);
	BASE_EXPORT int Minute(const DateTime_& dt);
	BASE_EXPORT String_ TimeString(const DateTime_& dt);
	inline String_ ToString(const DateTime_& dt) { return Date::ToString(dt.Date()) + " " + TimeString(dt); }
	BASE_EXPORT DateTime_ Minimum();
	BASE_EXPORT long long MSec(const DateTime_& dt);
}

// support use in interpolators
inline double NumericValueOf(const DateTime_& src) { return NumericValueOf(src.Date()) + src.Frac(); }
