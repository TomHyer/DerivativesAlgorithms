#pragma once

#include "Score.h"

struct PlyState_
{
	mutable std::array<uint64, 2> att, patt, dbl_att, xray;
	uint64 key, pawn_key, eval_key;
	mutable uint64 passer, threat, mask;
	packed_t pst;
	uint32 material;
	int* start, * current;
	mutable int best;
	mutable score_t score;
	std::array<uint16, N_KILLER + 1> killer;
	std::array<uint16, N_REF_TRY> ref;
	uint16 move;
	uint8 turn, castle_flags, ply, ep_square, capture, gen_flags, piece, stage, mul;
	sint32 moves[230];
};
