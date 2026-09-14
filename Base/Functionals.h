
#pragma once

#include <functional>

struct LinearIncrement_
{
	double scale_;
	LinearIncrement_(double s) : scale_(s) {}
	double operator()(double pvs, double incr) const { return pvs + scale_ * incr; }
};
inline LinearIncrement_ LinearIncrement(double scale)
{
	return LinearIncrement_(scale);
}

struct AverageIn_
{
	double newFrac_;
	AverageIn_(double n) : newFrac_(n) {}
	double operator()(double pvs, double nw) { return pvs + newFrac_ * (nw - pvs); }
};
inline AverageIn_ AverageIn(double new_frac)
{
	return AverageIn_(new_frac);
}

template<class T_> const T_& Dereference(const T_* p, const T_& v)
{
	return p ? *p : v;
}

template<class B_, class D_> B_* GetShared(const std::shared_ptr<D_>& src) { return src.get(); }

template<class A_, class R_> std::function<R_(A_)> AsFunctor(R_(*func)(A_)) { return std::function<R_(A_)>(func); }

template<class T_> struct Identity_
{
	const T_& operator()(const T_& arg) const { return arg; }
};

// functor for lookup-in-array, though lambdas are often easier
template<class C_, class XK_ = C_> struct ArrayFunctor_
{
	XK_ val_;
	ArrayFunctor_(const C_& val) : val_(val) {}
	const auto& operator()(int ii) const { return val_[ii]; }
};

template<class C_> ArrayFunctor_<C_, const C_&> XLookupIn(const C_& src)	// returns an ephemeral class containing a reference to src
{
	return ArrayFunctor_<C_, const C_&>(src);
}
template<class C_> ArrayFunctor_<C_> LookupIn(const C_& src)	// takes a copy of src
{
	return ArrayFunctor_<C_, C_>(src);
}

template<class DST_> struct ConstructCast_
{
	template<class SRC_> DST_ operator()(const SRC_& src) const { return DST_(src); }
};