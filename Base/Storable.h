
#pragma once

#include "Strings.h"

namespace Archive
{
	class Store_;
}

class BASE_EXPORT Storable_ : noncopyable
{
public:
	const String_ type_;
	const String_ name_;
	virtual ~Storable_();
	Storable_(const char* type, const String_& name) : type_(type), name_(name) {}
	// support actual storage of these things
	virtual void Write(Archive::Store_& dst) const = 0;
};

