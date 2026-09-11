
#include "Base/Platform.h"
#include "Report.h"
#include "Base/Strict.h"

#include "Base/Exceptions.h"

int Report_::Size(const String_& axis) const
{
	REQUIRE(axes_.count(axis), "No axis '" + axis + "'");
	const int which = axes_.find(axis)->second;
	return strides_[which + 1] / strides_[which];
}

double& Report_::operator[](const Report::Address_& loc)
{
	assert(loc.locs_.size() == strides_.size() - 1);
	return vals_[std::inner_product(loc.locs_.begin(), loc.locs_.end(), strides_.begin(), 0)];
}

const double& Report_::operator[](const Report::Address_& loc) const
{
	assert(loc.locs_.size() == strides_.size() - 1);
	return vals_[std::inner_product(loc.locs_.begin(), loc.locs_.end(), strides_.begin(), 0)];
}

void Report_::AddBookkeeping
	(const String_& title,
	 const Vector_<String_>& values)
{
	assert(values.size() == Size("TRADE"));
	bookkeeping_.push_back(make_pair(title, values));
}

