#pragma once

#include "Bit.h"
#include "Board.h"
#include "PawnEval.h"

struct GMaterial;
struct GEvalInfo
{
	std::array<uint64, 2> area, free;
	uint64 occ, rray, bray;
	const GPawnEntry* pawnEntry_;
	GPawnEntry pawnEval_;
	const GMaterial* material;
	packed_t score;
	std::array<uint32, 2> king_att;
	std::array<uint16, 2> king;
	uint16 mul;
};

// special calculators for specific material
// irritatingly, they need to be told what pop() to use
// setting any of these to call also triggers a call to eval_stalemate
// so they should never call eval_stalemate themselves
struct State_;
using eval_special_t = void(*)(GEvalInfo& EI, const State_& board, pop_func_t pop);
// here is a function that exists only to trigger eval_stalemate
inline void eval_null(GEvalInfo&, const State_&, pop_func_t) {}
inline void eval_unwinnable(GEvalInfo& EI, const State_&, pop_func_t) { EI.mul = 0; }

struct GMaterial
{
	sint16 score, closed;
	std::array<eval_special_t, 2> eval;
	std::array<uint8, 2> mul, kphase;
	uint8 phase;
};
