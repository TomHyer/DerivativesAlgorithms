#pragma once

#include <ranges>
#include "Vectors.h"

template<class E_> using RangeIter = const E_*;
template<class E_> using MutableIter = E_*;

template<class I_> class Range_ : public std::ranges::subrange<I_>
{
public:
	Range_() : std::ranges::subrange<I_>(nullptr, nullptr) {}
	Range_(I_ begin, I_ end) : std::ranges::subrange<I_>(begin, end) {}

	template<class C_> bool Overlaps(const C_& other) const
	{
		return !other.empty() && &other.front() < this->end() && !(&other.back() < this->begin());
	}
};
