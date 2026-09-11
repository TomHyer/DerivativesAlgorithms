
#include "Base/Platform.h"
#include "Globals.h"
#include <mutex>
#include "Base/Strict.h"

#include "Base/Host.h"
#include "Base/Cell.h"

Global::Store_::~Store_()
{	}

namespace
{
	// this needs to be a Meyers singleton, not a file-scope static, because we will initialize it with a file-scope static
	std::unique_ptr<Global::Store_>& XTheStore()
	{
      RETURN_STATIC(std::unique_ptr<Global::Store_>);
	}

	static std::mutex TheStoreMutex;
#define LOCK_STORE std::lock_guard<std::mutex> l(TheStoreMutex)

	// utility functions to store a date by name
	Date_ GetGlobalDate(const String_& which)
	{
		LOCK_STORE;
		Cell_ stored = Global::TheStore().Get(which);
		if (Cell::IsEmpty(stored))
		{
			// no global date set; initialize to system date
			int yy, mm, dd;
			Host::LocalTime(&yy, &mm, &dd);
			stored = Date_(yy, mm, dd);
			Global::TheStore().Set(which, stored);
		}
		return Cell::ToDate(stored);
	}

	// names for the global dates (these will be visible in the repository, so make them comprehensible)
	static const String_ ACCOUNTING("AccountingDate");
}

Global::Store_& Global::TheStore()
{
	return *XTheStore();
}

void Global::SetTheStore(Global::Store_* orphan)
{
	XTheStore().reset(orphan);
}

Date_ Global::Dates_::AccountingDate() const
{
	return GetGlobalDate(ACCOUNTING);
}

void XGLOBAL::SetAccountingDate(const Date_& when)
{
	Global::TheStore().Set(ACCOUNTING, when);
}

XGLOBAL::ScopedOverride_<Date_> XGLOBAL::SetAccountingDateInScope(const Date_& dt)
{
	ScopedOverride_<Date_> retval(SetAccountingDate, GetGlobalDate(ACCOUNTING));
	SetAccountingDate(dt);
	return retval;
}
