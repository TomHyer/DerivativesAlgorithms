#pragma once

#include <chrono>
#include "Bit.h"

typedef std::chrono::high_resolution_clock::time_point time_point;

time_point now()
{
	return std::chrono::high_resolution_clock::now();
}
uint32 millisecs(const time_point& t1, const time_point& t2)
{
	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
	return static_cast<uint32>(fabs(time_span.count()) * 1000.0);
}
