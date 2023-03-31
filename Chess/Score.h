#pragma once

#include "Chess.h"

typedef sint16 score_t;
constexpr score_t CP_EVAL = 4;	// numeric value of 1 centipawn, in eval phase
constexpr score_t CP_SEARCH = 4;	// numeric value of 1 centipawn, in search phase (# of value equivalence classes in 1-cp interval)

