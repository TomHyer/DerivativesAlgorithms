
#include "Platform.h"
#include "Cell.h"
#include "Strict.h"

#include "Vectors.h"
#include "Exceptions.h"
#include "Numerics.h"
#include "StringUtils.h"

namespace
{
	struct ToString_
	{
		String_ operator()(double d) const { int ii = AsInt(d);  return ii == d ? String::FromInt(ii) : String::FromDouble(d); }
		String_ operator()(bool b) const { return b ? "TRUE" : "FALSE"; }
		String_ operator()(const String_& s) const { return s; }
		String_ operator()(const Date_& dt) const { return Date::ToString(dt); }
		String_ operator()(const DateTime_& dt) const { return DateTime::ToString(dt); }
		String_ operator()(std::monostate) const { return String_(); }
		template<class E_> String_ operator()(E_) const { THROW("Unrecognized cell type"); }
	};

	struct ToDouble_
	{
		double operator()(double d) const { return d; }
		double operator()(bool b) const { return b ? 1 : 0; }
		template<class E_> double operator()(E_) const { THROW("Cell must contain a numeric value"); }
	};

	struct ToInt_
	{
		int operator()(double d) const { int ii = AsInt(d);  REQUIRE(ii == d, "Call contains non-integer number"); return ii; }
		int operator()(bool b) const { return b ? 1 : 0; }
		template<class E_> int operator()(E_) const { THROW("Cell must contain an integer value"); }
	};

	struct IsInt_
	{
		bool operator()(double d) const { int ii = AsInt(d);  return ii == d; }
		template<class E_> bool operator()(E_) const { return false; }
	};
}
String_ Cell::ToString(const Cell_& src)
{
	return src.Visit(ToString_());
}

double Cell::ToDouble(const Cell_& src)
{
	return src.Visit(ToDouble_());
}

bool Cell::IsInt(const Cell_& src)
{
	return src.Visit(IsInt_());
}

int Cell::ToInt(const Cell_& src)
{
	return src.Visit(ToInt_());
}

bool Cell::ToBool(const Cell_& src)
{
	if (auto p = std::get_if<bool>(&src.val_))
		return *p;
	THROW("Cell must contain a boolean value");
}

Date_ Cell::ToDate(const Cell_& src)
{
	if (auto p = std::get_if<Date_>(&src.val_))
		return *p;
	THROW("Cell must contain a date value");
}

DateTime_ Cell::ToDateTime(const Cell_& src)
{
	if (auto p = std::get_if<DateTime_>(&src.val_))
		return *p;
	THROW("Cell must contain a timepoint value");
}

Vector_<bool> Cell::ToBoolVector(const Cell_& src)
{
	if (auto p = std::get_if<bool>(&src.val_))
		return Vector_<bool>(1, *p);
	if (auto p = std::get_if<String_>(&src.val_))
		return String::ToBoolVector(*p);
	THROW("Cell is not convertible to vector of booleans");
}

Cell_ Cell::FromBoolVector(const Vector_<bool>& src)
{
	String_ temp;
	for (auto b : src)
		temp.push_back(b ? 'T' : 'F');
	return Cell_(temp);
}

bool operator==(const Cell_& lhs, const String_& rhs)
{
	if (auto p = std::get_if<String_>(&lhs.val_))
		return *p == rhs;
	return false;
}
