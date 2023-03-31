#pragma once

#include "Chess.h"

constexpr uint32 TB_RESULT_FAILED = 0xFFFFFFFF;

unsigned tb_probe_wdl_fwd(
	uint64_t _white,
	uint64_t _black,
	uint64_t _kings,
	uint64_t _queens,
	uint64_t _rooks,
	uint64_t _bishops,
	uint64_t _knights,
	uint64_t _pawns,
	unsigned _rule50,
	unsigned _castling,
	unsigned _ep,
	bool     _turn);

unsigned tb_probe_root_fwd(
	uint64_t _white,
	uint64_t _black,
	uint64_t _kings,
	uint64_t _queens,
	uint64_t _rooks,
	uint64_t _bishops,
	uint64_t _knights,
	uint64_t _pawns,
	unsigned _rule50,
	unsigned _ep,
	bool     _turn);

static inline unsigned tb_probe_root_checked(
	uint64_t _white,
	uint64_t _black,
	uint64_t _kings,
	uint64_t _queens,
	uint64_t _rooks,
	uint64_t _bishops,
	uint64_t _knights,
	uint64_t _pawns,
	unsigned _rule50,
	unsigned _castling,
	unsigned _ep,
	bool     _turn)
{
	if (_castling != 0)
		return TB_RESULT_FAILED;
	return tb_probe_root_fwd(_white, _black, _kings, _queens, _rooks, _bishops, _knights, _pawns, _rule50, _ep, _turn);
}

int GetTBMove(unsigned res, int* best_score);
