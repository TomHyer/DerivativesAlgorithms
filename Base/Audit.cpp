
#include "Platform.h"
#include "Audit.h"
#include <iterator>
#include "Strict.h"

#include "Strings.h"
#include "Storable.h"
#include "Bag.h"
#include "Functionals.h"

namespace
{
	struct ShowToAuditor_
	{
		String_ key_;
		const Handle_<Storable_>& value_;
		ShowToAuditor_(const String_& key, const Handle_<Storable_>& value) : key_(key), value_(value) {}
		void operator()(const Environment::Entry_& ee) const
		{
			if (const Auditor_* pa = dynamic_cast<const Auditor_*>(&ee))
				pa->Notice(key_, value_);
		}
	};
}	// leave local

void AuditorImp_::Notice
	(const String_& key,
	const Handle_<Storable_>& value)
const
{
	switch (mode_)
	{
	case READING_EXCLUSIVE:
		mine_->contents_.erase(key);  
		[[fallthrough]];
	case READING:
		mine_->contents_.insert(make_pair(key, value));
		break;
	}
}

Vector_<Handle_<Storable_> > AuditorImp_::Find
	(const String_& key)
const
{
	Vector_<Handle_<Storable_> > retval;
	if (mode_ == SHOWING)
	{
		auto range = mine_->contents_.equal_range(key);
		transform(range.first, range.second, std::back_inserter(retval), GetSecond_<String_, Handle_<Storable_>>());
	}
	return retval;
}

void Environment::AuditBase(_ENV,
	const String_& key, const Handle_<Storable_>& value)
{
	ShowToAuditor_ f(key, handle_cast<Storable_>(value));
	Environment::Iterate(_env, f);
}

