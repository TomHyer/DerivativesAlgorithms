
#include "Platform.h"
#include "Dictionary.h"
#include "Strict.h"

#include "Exceptions.h"

void Dictionary_::Insert(const String_& key, const Cell_& val)
{
	const String_& k = String::Condensed(key);
	REQUIRE(!val_.count(k), "Duplicate key '" + k + "'");
	val_.insert(make_pair(k, val));
}

const Cell_& Dictionary_::At(const String_& key, bool optional) const
{
	static const Cell_ EMPTY;
	const String_& k = String::Condensed(key);
	if (auto p = val_.find(k); p != val_.end())
		return p->second;
	REQUIRE(optional, "No value for key '" + k + "'");
	return EMPTY;
}

bool Dictionary_::Has(const String_& key) const
{
	return val_.count(String::Condensed(key)) > 0;
}

Handle_<Storable_> Dictionary::FindHandleBase
	(const std::map<String_, Handle_<Storable_>>& handles, 
	 const String_& key,
	 bool quiet)
{
	if (auto p = handles.find(String::Condensed(key)); p != handles.end())
        return p->second;
	REQUIRE(quiet, "No handle found at '" + key + "'");
	return Handle_<Storable_>();
}
