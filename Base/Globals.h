
// global state; e.g., dates which define "now"

#pragma once

#include "Environment.h"

class String_;
struct Cell_;
class Date_;

namespace Global
{
	class BASE_EXPORT Store_ : noncopyable
	{
	public:
		virtual ~Store_();
		virtual void Set(const String_& name, const Cell_& value) = 0;
		virtual Cell_ Get(const String_& name) = 0;
	};

	Store_& TheStore();
	void SetTheStore(Store_* orphan);	// we take over the memory
}

namespace XGLOBAL
{
	template<class T_> class ScopedOverride_
	{
		typedef void(*Setter_)(const T_&);
		T_ saved_;
		Setter_ setFunc_;
	public:
		ScopedOverride_(Setter_ set_func, const T_& saved_val) : saved_(saved_val), setFunc_(set_func) {}
		~ScopedOverride_() { setFunc_(saved_); }
	};
}
