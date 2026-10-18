
// Enhanced Platform for interface files
	// include all necessary FFI frameworks here (per build configuration)

#pragma once

#include "Platform.h"
#include "Exceptions.h"
#include "Environment.h"
#include "Cell.h"
#include "_Excel.h"
#include "_Repository.h"
#include "_Reader.h"
#include "_Cookie.h"
#include "DateTimeUtils.h"
#ifndef DA_EXCEL_ONLY
#include "_Python.h"
#endif
//#include "_JNI.h"
