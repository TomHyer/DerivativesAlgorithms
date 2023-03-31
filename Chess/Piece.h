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
