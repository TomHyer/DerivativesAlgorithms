
// holder for a generic scalar type

#pragma once
#include <variant>
#include "DateTime.h"

struct BASE_EXPORT Cell_
{
	std::variant<bool, double, Date_, DateTime_, String_, std::monostate> val_;

	Cell_() : val_(std::monostate()) {}
	Cell_(const Cell_&) = default;
	explicit Cell_(bool b) : val_(b) {}
	Cell_(double d) : val_(d) {}
	Cell_(const Date_& dt) : val_(dt) {}
	Cell_(const DateTime_& dt) : val_(dt) {}
	Cell_(const String_& s) : val_(s) {}
	Cell_(const char* s) : val_(String_(s))	{}	// otherwise pointer can be converted to boolean
	template<class T_> Cell_(const T_* p) = delete;	// unrecognized pointer type, hides ptr-to-bool conversion

	bool operator==(const Cell_& rhs) const = default;

	void Clear() { val_ = std::monostate(); }
	template<class V_> auto Visit(V_ func) const { return std::visit(func, val_);  }
	Cell_& operator=(bool b) { val_ = b; return *this; }
	Cell_& operator=(int i) { val_ = double(i); return *this; }
	Cell_& operator=(double d) { val_ = d; return *this; }
	Cell_& operator=(const Date_& dt) { val_ = dt; return *this; }
	Cell_& operator=(const DateTime_& dt) { val_ = dt; return *this; }
	Cell_& operator=(const String_& s) { val_ = s; return *this; }
	Cell_& operator=(const char* s) { val_ = String_(s); return *this; }

	Cell_& operator=(const Cell_& rhs) { val_ = rhs.val_; return *this; }
};

BASE_EXPORT bool operator==(const Cell_& lhs, const String_& rhs);
inline bool operator==(const String_& lhs, const Cell_& rhs) { return rhs == lhs; }

namespace Cell
{
	using std::get_if;
	using std::monostate;

	inline bool IsEmpty(const Cell_& cell)
	{
		if (auto p = get_if<String_>(&cell.val_))
			return p->empty();
		return !!get_if<monostate>(&cell.val_);
	}

	inline bool IsString(const Cell_& src) { return !!get_if<String_>(&src.val_); }
	BASE_EXPORT String_ ToString(const Cell_& src);
	inline Cell_ FromString(const String_& src) { return Cell_(src); }

	template<class T_> T_ ToEnum(const Cell_& src) { return T_(ToString(src)); }
	template<class T_> T_ ToEnum(const Cell_& src, const T_& empty_val) { return IsEmpty(src) ? empty_val : ToEnum<T_>(src); }
	template<class T_> Cell_ FromEnum(const T_& src) { return Cell_(String_(src.String())); }

	inline bool IsDouble(const Cell_& src) { return !!get_if<double>(&src.val_); }
	BASE_EXPORT double ToDouble(const Cell_& src);	// throws on non-numeric values
	inline double ToDouble(const Cell_& src, double empty_val) { return IsEmpty(src) ? empty_val : ToDouble(src); }
	inline Cell_ FromDouble(double src) { return Cell_(src); }

	BASE_EXPORT bool IsInt(const Cell_& src);
	BASE_EXPORT int ToInt(const Cell_& src);	// throws on non-integer values
	inline int ToInt(const Cell_& src, int empty_val) { return IsEmpty(src) ? empty_val : ToInt(src); }
	inline Cell_ FromInt(int src) { return Cell_(double(src)); }

	inline bool IsBool(const Cell_& src) { return !!get_if<bool>(&src.val_); }
	BASE_EXPORT bool ToBool(const Cell_& src);	// throws on non-boolean values
	inline Cell_ FromBool(bool src) { return Cell_(src); }

	inline bool IsDate(const Cell_& src) { return !!get_if<Date_>(&src.val_); }
	BASE_EXPORT Date_ ToDate(const Cell_& src);	// throws on non-date values
	inline Cell_ FromDate(const Date_& src) { return Cell_(src); }

	inline bool IsDateTime(const Cell_& src) { return !!get_if<DateTime_>(&src.val_); }
	BASE_EXPORT DateTime_ ToDateTime(const Cell_& src);	// throws on non-time values (including date w/o time)
	inline Cell_ FromDateTime(const DateTime_& src) { return Cell_(src); }

	BASE_EXPORT Vector_<bool> ToBoolVector(const Cell_& src);	// accepts single boolean, or String_
	BASE_EXPORT Cell_ FromBoolVector(const Vector_<bool>& src);
}
