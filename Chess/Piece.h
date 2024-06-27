#pragma once

#include <array>
#include "Chess.h"

constexpr uint8 WhitePawn = 2;
constexpr uint8 BlackPawn = 3;
constexpr std::array<uint8, 2> IPawn = { WhitePawn, BlackPawn };
constexpr uint8 WhiteKnight = 4;
constexpr uint8 BlackKnight = 5;
constexpr std::array<uint8, 2> IKnight = { WhiteKnight, BlackKnight };
constexpr uint8 WhiteLight = 6;
constexpr uint8 BlackLight = 7;
constexpr std::array<uint8, 2> ILight = { WhiteLight, BlackLight };
constexpr uint8 WhiteDark = 8;
constexpr uint8 BlackDark = 9;
constexpr std::array<uint8, 2> IDark = { WhiteDark, BlackDark };
constexpr uint8 WhiteRook = 10;
constexpr uint8 BlackRook = 11;
constexpr std::array<uint8, 2> IRook = { WhiteRook, BlackRook };
constexpr uint8 WhiteQueen = 12;
constexpr uint8 BlackQueen = 13;
constexpr std::array<uint8, 2> IQueen = { WhiteQueen, BlackQueen };
constexpr uint8 WhiteKing = 14;
constexpr uint8 BlackKing = 15;
constexpr std::array<uint8, 2> IKing = { WhiteKing, BlackKing };
INLINE bool IsBishop(uint8 piece)
{
	return piece >= WhiteLight && piece < WhiteRook;
}

constexpr std::array<uint8, 256> MakePieceFromChar()
{
	std::array<uint8, 256> retval = {};
	for (int i = 0; i < 256; ++i)
		retval[i] = 0;
	retval['B'] = WhiteLight;
	retval['K'] = WhiteKing;
	retval['N'] = WhiteKnight;
	retval['P'] = WhitePawn;
	retval['Q'] = WhiteQueen;
	retval['R'] = WhiteRook;
	retval['b'] = BlackLight;
	retval['k'] = BlackKing;
	retval['n'] = BlackKnight;
	retval['p'] = BlackPawn;
	retval['q'] = BlackQueen;
	retval['r'] = BlackRook;
	return retval;
}
constexpr std::array<uint8, 256> PieceFromChar = MakePieceFromChar();

constexpr int PieceType[16] = { 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5 };

