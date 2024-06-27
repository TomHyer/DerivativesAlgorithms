#pragma once

#include "../Base/Host.h"
#include "Piece.h"

namespace Calc
{
	constexpr std::array<int, 16> MvvLvaVictim = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3 };
	constexpr std::array<int, 16> MvvLvaAttacker = { 0, 0, 5, 5, 4, 4, 3, 3, 3, 3, 2, 2, 1, 1, 6, 6 };
	constexpr std::array<int, 16> MvvLvaAttackerKB = { 0, 0, 9, 9, 7, 7, 5, 5, 5, 5, 3, 3, 1, 1, 11, 11 };

	constexpr int PawnCaptureMvvLva(int attacker) { return MvvLvaAttacker[attacker]; }
	constexpr int MaxPawnCaptureMvvLva = MvvLvaAttacker[15];  // 6
	constexpr int KnightCaptureMvvLva(int attacker) { return MaxPawnCaptureMvvLva + MvvLvaAttackerKB[attacker]; }
	constexpr int MaxKnightCaptureMvvLva = MaxPawnCaptureMvvLva + MvvLvaAttackerKB[15];  // 17
	constexpr int BishopCaptureMvvLva(int attacker) { return MaxPawnCaptureMvvLva + MvvLvaAttackerKB[attacker] + 1; }
	constexpr int MaxBishopCaptureMvvLva = MaxPawnCaptureMvvLva + MvvLvaAttackerKB[15] + 1;  // usually 18
	constexpr int RookCaptureMvvLva(int attacker) { return MaxBishopCaptureMvvLva + MvvLvaAttacker[attacker]; }
	constexpr int MaxRookCaptureMvvLva = MaxBishopCaptureMvvLva + MvvLvaAttacker[15];  // usually 24
	constexpr int QueenCaptureMvvLva(int attacker) { return MaxRookCaptureMvvLva + MvvLvaAttacker[attacker]; }

	constexpr std::array<std::array<int, 16>, 16> MakeMvvLva()
	{
		std::array<std::array<int, 16>, 16> retval;
		for (int i = 0; i < 16; ++i)
			for (int j = 0; j < 16; ++j)
			{
				if (j < WhitePawn)
					retval[i][j] = 0;
				else if (j < WhiteKnight)
					retval[i][j] = PawnCaptureMvvLva(i) << 26;
				else if (j < WhiteLight)
					retval[i][j] = KnightCaptureMvvLva(i) << 26;
				else if (j < WhiteRook)
					retval[i][j] = BishopCaptureMvvLva(i) << 26;
				else if (j < WhiteQueen)
					retval[i][j] = RookCaptureMvvLva(i) << 26;
				else
					retval[i][j] = QueenCaptureMvvLva(i) << 26;
			}
		return retval;
	}
}
