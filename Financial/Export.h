#pragma once

#ifdef IS_FINANCIAL
#define FINANCIAL_EXPORT __declspec(dllexport)
#else
#define FINANCIAL_EXPORT __declspec(dllimport)
#endif
