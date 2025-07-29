
#define _Regex_traits _Regex_traits_Repository
#include "Platform.h"
#include "_Repository.h"
#include <map>
#include <mutex>
#include <regex> 
#include "Strict.h"

#include "Storable.h"
#include "Exceptions.h"
#include "Mutexes.h"
#include "Globals.h"
#include "Box.h"
#include "MatrixUtils.h"

#include "MG_RepositoryErase_enum.inc"

namespace
{
	typedef std::map<String_, Handle_<Storable_>> vals_t;

	MutexProtected_<vals_t>& TheObjects()
	{
		RETURN_STATIC(MutexProtected_<vals_t>);
	}

	template<class FUNC_, typename ... ARGS_>
	auto ApplyToObjects(FUNC_ func, ARGS_ ... args)
	{
		return TheObjects().Apply([&](vals_t objects) { return func(objects, args ...); });
	}


	String_ TypeTag(const Storable_& s)
	{
		return "~" + s.type_ + "~";
	}
	String_ BaseTag(const Storable_& s)
	{
		return TypeTag(s) + s.name_ + "~";
	}
	String_ Uniquifier(const Storable_& s)
	{
      return String::Uniquifier(&s);
	}


	void EraseByStart(vals_t& objects, const String_& start)
	{
		auto pLow = objects.lower_bound(start);
		auto pHigh = pLow;
		while (pHigh != objects.end() && pHigh->first.substr(0, start.size()) == start)
			++pHigh;
		objects.erase(pLow, pHigh);
	}

	String_ AddBaseImp
		(vals_t& objects, 
		 const Handle_<Storable_>& s,
		 const RepositoryErase_& erase)
	{
		const String_ untickered = BaseTag(*s);
		switch (erase.Switch())
		{
		case RepositoryErase_::Value_::NAME_NONEMPTY:
			if (s->name_.empty())
				break;
			[[fallthrough]];
		case RepositoryErase_::Value_::NAME:
			EraseByStart(objects, untickered);	// squash anything with this name
			break;
		case RepositoryErase_::Value_::TYPE:
			EraseByStart(objects, TypeTag(*s));	// squash anything with this type	
			break;
		}
		// call to LOCK_OBJECTS must be after EraseByStart, because that will also acquire the lock
		const String_ retval = untickered + Uniquifier(*s);
		objects.insert(make_pair(retval, s));
		return retval;
	}

	int ErasePattern(vals_t& objects, const String_& pattern)
	{
		std::regex match(pattern);
		auto start = objects.begin();
		// could do this with remove_if, but the lambda is ghastly
		int retval = 0;
		while (start != objects.end())
		{
			if (std::regex_search(start->first, match))
				++retval, objects.erase(start++);
			else
				++start;
		}
		return retval;
	}

	Vector_<Handle_<Storable_> > FindImp(vals_t& objects, const String_& pattern)
	{
		std::regex match(pattern);
		auto all = objects;
		Vector_<Handle_<Storable_>> retval;
		for (auto [tag, obj] : all)
		{
			if (std::regex_search(tag, match))
				retval.push_back(obj);
		}
		return retval;
	}

	Handle_<Storable_> FetchImp
		(vals_t& objects, const String_& tag, bool quiet)
	{
		NOTICE(tag);
		Handle_<Storable_> retval;
		if (auto po = objects.find(tag); po != objects.end())
		{
			assert(po->second);  // no null object in repository
			return po->second;
		}
		REQUIRE(quiet, "Repository object not found");
		return Handle_<Storable_>();
	}
}	// leave local

String_ ObjectAccess_::AddBase
	(const Handle_<Storable_>& s,
	 const RepositoryErase_& erase) 
const
{
	return ApplyToObjects(AddBaseImp, s, erase);
}

int ObjectAccess_::Erase(const String_& pattern) const
{
	return ApplyToObjects(ErasePattern, pattern);
}

bool ObjectAccess_::Erase(const Storable_& object) const
{	// standard implementation leads to C++ attempting to copy-construct a Storable_, for whatever reason
	auto erase = [&](vals_t& objects)
	{
		if (auto po = objects.find(BaseTag(object) + Uniquifier(object)); po != objects.end())
		{
			objects.erase(po);
			return true;
		}
		return false;
	};
	return TheObjects().Apply(erase);
}

int ObjectAccess_::Size() const
{
	return ApplyToObjects([&](vals_t& vals) { return static_cast<int>(vals.size()); });
}

Vector_<Handle_<Storable_> > ObjectAccess_::Find(const String_& pattern) const
{
	return ApplyToObjects(FindImp, pattern);
}

// enable a store for low-level global objects

namespace
{
	static const String_ GLOBAL_TAG("##GLOBAL##");

	Cell_ FindInBox(vals_t& objects, const String_& tag_start)
	{
		if (auto po = objects.lower_bound(tag_start);
			po != objects.end() && po->first.substr(0, tag_start.size()) == tag_start)
		{
			const Box_* box = dynamic_cast<const Box_*>(po->second.get());
			REQUIRE(box && !box->contents_.Empty(), "Internal error, or global tag meddling");
			return box->contents_(0, 0);
		}
		return Cell_();
	}

	struct RepoStore_ : Global::Store_
	{
		void Set(const String_& name, const Cell_& value) override
		{
			Handle_<Box_> storable(new Box_(GLOBAL_TAG + name, Matrix::M1x1(value)));
			(void) ObjectAccess_().Add(storable, RepositoryErase_());
		}
		Cell_ Get(const String_& name) override
		{
			static const Box_ dummy(GLOBAL_TAG + name, Matrix_<Cell_>());
			return ApplyToObjects(FindInBox, BaseTag(dummy));
		}
	};

	RUN_AT_LOAD(Global::SetTheStore(new RepoStore_))
}	// leave local

Handle_<Storable_> ObjectAccess_::Fetch
   (const String_& tag, bool quiet)
const
{
	return ApplyToObjects(FetchImp, tag, quiet);
}
