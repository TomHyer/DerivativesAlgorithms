
// makes objects convertible-to-bool as long as TruthValueOf() can be called on them

#pragma once

template<class T_> struct Boolean_
{
	T_ val_;
	Boolean_(const T_& val) : val_(val) {}
	T_& operator=(const T_& rhs) { return val_ = rhs; }
	operator bool() const { return TruthValueOf(val_); }
	operator const T_&() const { return val_; }
};
