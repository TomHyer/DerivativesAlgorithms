
// compact date -- stores integer dates from 1/1/1970 to 6/5/2149

#pragma once

#include <cstdint>
#include "Strings.h"	// forward declaration only works for non-std classes

class Date_;

namespace Date
{
	BASE_EXPORT short Year(const Date_& dt);
	BASE_EXPORT short Month(const Date_& dt);
	BASE_EXPORT short Day(const Date_& dt);
	BASE_EXPORT short DayOfWeek(const Date_& dt);	// 0 == Sunday
	BASE_EXPORT short DaysInMonth(int year, int month);
	inline bool IsWeekend(const Date_& dt) { return DayOfWeek(dt) % 6 == 0; }

	BASE_EXPORT Date_ FromExcel(int serial);	// converts from serial date with Excel offset (ignores Excel bug for pre-1902 dates)
	BASE_EXPORT int ToExcel(const Date_& dt);	// extracts serial date with Excel offset

	BASE_EXPORT String_ ToString(const Date_& date);	// always yyyy-mm-dd
	// FromString is higher-level functionality, moved to DateUtils
	BASE_EXPORT Date_ Minimum();
	BASE_EXPORT Date_ Maximum();

	BASE_EXPORT Date_ EndOfMonth(const Date_& date);
	BASE_EXPORT Date_ AddMonths(const Date_& date, int n_months, bool preserve_eom = false);
}

class BASE_EXPORT Date_
{
	uint16_t serial_;	// days from start of unix era
	friend Date_ Date::FromExcel(int);
	friend int Date::ToExcel(const Date_&);
public:
	Date_() : serial_(0) {}	// invalid value
	Date_(int yyyy, int mm, int dd);
	Date_(const Date_& src) : serial_(src.serial_) {}

	bool operator==(const Date_&) const = default;
	auto operator<=>(const Date_&) const = default;

	bool IsValid() const { return serial_ > 0; }
	Date_& operator=(const Date_& rhs) { serial_ = rhs.serial_; return *this; }
	Date_ AddDays(int days) const;
	inline Date_& operator++() { ++serial_; return *this; }
	inline Date_& operator--() { --serial_; return *this; }
};

BASE_EXPORT int operator-(const Date_& lhs, const Date_& rhs);

// support interpolation based on date
inline double NumericValueOf(const Date_& src) { return Date::ToExcel(src); }
