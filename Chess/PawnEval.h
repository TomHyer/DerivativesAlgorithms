#pragma once

#include "EvalImp.h"
struct GPawnEntry
{
	uint64 key;
	packed_t score;
	std::array<sint16, 2> shelter;
	std::array<uint8, 2> passer, draw;
};
constexpr GPawnEntry NullPawnEntry = { 0, 0,{ 0, 0 },{ 0, 0 },{ 0, 0 } };

