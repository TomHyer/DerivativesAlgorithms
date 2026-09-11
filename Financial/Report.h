
// risk report object

#pragma once

#include <deque>
#include <map>
#include "Base/Vectors.h"
#include "Base/Algorithms.h"
#include "Base/Storable.h"

namespace Report
{
	struct Address_
	{
		std::map<String_, int> axes_;
		Vector_<int> locs_;
		Address_(const std::map<String_, int>& axes) : axes_(axes), locs_(axes.size()) {}
		int& operator[](const String_& axis);
	};
}

class Report_ : public Storable_
{
	std::map<String_, int> axes_;   // lookup location
	Vector_<int> strides_;      // back() is whole size
	std::deque<double> vals_;
	Vector_<pair<String_, Vector_<String_> > > bookkeeping_;

public:
	Vector_<String_> Axes() const { return Keys(axes_); }
	int Size(const String_& axis) const;

	typedef Report::Address_ Address_;
	Address_ MakeAddress() const;
	double& operator[](const Address_& loc);
	const double& operator[](const Address_& loc) const;

	void AddBookkeeping
		(const String_& title,
		 const Vector_<String_>& values);
	// ...
};