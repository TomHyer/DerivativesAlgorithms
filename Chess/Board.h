#pragma once

#include "Chess.h"
#include "Array.h"
#include "BoardImpl.h"
#include "Piece.h"

using Loc_ = uint8;

constexpr uint64 Filled = 0xFFFFFFFFFFFFFFFF;
constexpr uint64 Interior = 0x007E7E7E7E7E7E00;
constexpr uint64 Boundary = ~Interior;
constexpr uint64 LightArea = 0x55AA55AA55AA55AA;
constexpr uint64 DarkArea = ~LightArea;

template<bool me> INLINE int OwnRank(int loc)
{
	return me ? (7 - RankOf(loc)) : RankOf(loc);
}
INLINE int OwnRank(bool me, int loc)
{
	return me ? (7 - RankOf(loc)) : RankOf(loc);
}
INLINE constexpr int LInf(int x, int y)
{
	return Max(Abs(RankOf(x) - RankOf(y)), Abs(FileOf(x) - FileOf(y)));
}

using Calc::File;
using Calc::Line;

template<bool me> INLINE const uint64& OwnLine(int n)
{
	return Line[me ? 7 - n : n];
}


constexpr std::array<std::array<uint64, 64>, 2> PAtt = { make_array<64>(Calc::PAttW) , make_array<64>(Calc::PAttB) };
constexpr std::array<uint64, 8> West = make_array<8>(Calc::West);
constexpr std::array<uint64, 8> East = make_array<8>(Calc::East);
constexpr std::array<uint64, 64> NAttAtt = make_array<64>(Calc::NAttAtt);
constexpr std::array<uint64, 64> KAttAtt = make_array<64>(Calc::KAttAtt);
constexpr std::array<std::array<uint64, 64>, 2> PWay = { make_array<64>(Calc::PWay<White>), make_array<64>(Calc::PWay<Black>) };
constexpr std::array<uint64, 64> QMask = make_array<64>(Calc::QMask);

template<bool me> INLINE uint64 ShiftW(const uint64& target)
{
	return me ? Calc::ShiftSW(target) : Calc::ShiftNW(target);
}
template<bool me> INLINE uint64 ShiftE(const uint64& target)
{
	return me ? Calc::ShiftSE(target) : Calc::ShiftNE(target);
}
template<bool me> INLINE uint64 Shift(const uint64& target)
{
	return me ? target >> 8 : target << 8;
}

struct Board_
{
	std::array<uint8, 64> square;
	std::array<uint64, 16> bb;

	// lots of query functions
	INLINE uint8& PieceAt(int sq) { return square[sq]; }
	INLINE const uint8& PieceAt(int sq) const { return square[sq]; }
	INLINE uint64& Piece(int i) { return bb[i]; }
	INLINE const uint64& Piece(int i) const { return bb[i]; }

	INLINE uint64& Pawn(int me) { return Piece(WhitePawn | me); }
	INLINE const uint64& Pawn(int me) const { return Piece(WhitePawn | me); }
	INLINE uint64& Knight(int me) { return Piece(WhiteKnight | me); }
	INLINE const uint64& Knight(int me) const { return Piece(WhiteKnight | me); }
	INLINE uint64 Bishop(int me) const { return Piece(WhiteLight | me) | Piece(WhiteDark | me); }
	INLINE uint64& Rook(int me) { return Piece(WhiteRook | me); }
	INLINE const uint64& Rook(int me) const { return Piece(WhiteRook | me); }
	INLINE uint64& Queen(int me) { return Piece(WhiteQueen | me); }
	INLINE const uint64& Queen(int me) const { return Piece(WhiteQueen | me); }
	INLINE uint64& King(int me) { return Piece(WhiteKing | me); }
	INLINE const uint64& King(int me) const { return Piece(WhiteKing | me); }
	INLINE uint64 NonPawn(int me) const { return Piece(me) ^ Pawn(me); }
	INLINE uint64 NonPawnKing(int me) const { return NonPawn(me) ^ King(me); }
	INLINE uint64 Major(int me) const { return Rook(me) | Queen(me); }
	INLINE uint64 Minor(int me) const { return Knight(me) | Bishop(me); }
	INLINE uint64 PieceAll() const { return Piece(White) | Piece(Black); }
	INLINE uint64 PawnAll() const { return Pawn(White) | Pawn(Black); }
	INLINE uint64 NonPawnKingAll() const { return NonPawnKing(White) | NonPawnKing(Black); }
};
