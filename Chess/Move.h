#pragma once

#include "Chess.h"
#include "Piece.h"

INLINE int From(uint16 move)
{
	return (move >> 6) & 0x3f;
}
INLINE int To(uint16 move)
{
	return move & 0x3f;
}
inline void SetScore(int* move, uint16 score)
{
	*move = (*move & 0xFFFF) | (score << 16);
}  // notice this operates on long-form (int) moves

constexpr uint8 CanCastle_OO = 1;
constexpr uint8 CanCastle_oo = 2;
constexpr uint8 CanCastle_OOO = 4;
constexpr uint8 CanCastle_ooo = 8;

constexpr uint16 FlagCastling = 0x1000;  // overloaded by Roc to flag "joins", desirable quiet moves
constexpr uint16 FlagEP = 0x2000;
constexpr uint16 FlagPKnight = 0x4000;
constexpr uint16 FlagPBishop = 0x6000;
constexpr uint16 FlagPRook = 0xA000;
constexpr uint16 FlagPQueen = 0xC000;

INLINE bool IsPromotion(uint16 move)
{
	return T(move & 0xC000);
}
INLINE bool IsCastling(int piece, uint16 move)
{
	return piece >= WhiteKing && abs(To(move) - From(move)) == 2;
}
INLINE bool IsEP(uint16 move)
{
	return (move & 0xF000) == FlagEP;
}
template<bool me> INLINE uint8 Promotion(uint16 move)
{
	uint8 retval = (move & 0xF000) >> 12;
	if (retval == WhiteLight && HasBit(DarkArea, To(move)))
		retval += 2;
	return retval + (me ? 1 : 0);
}

constexpr std::array<uint8, 64> UpdateCastling = { 0xFF ^ CanCastle_OOO, 0xFF, 0xFF, 0xFF,
	0xFF ^ (CanCastle_OO | CanCastle_OOO), 0xFF, 0xFF, 0xFF ^ CanCastle_OO,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF ^ CanCastle_ooo, 0xFF, 0xFF, 0xFF,
	0xFF ^ (CanCastle_oo | CanCastle_ooo), 0xFF, 0xFF, 0xFF ^ CanCastle_oo };
