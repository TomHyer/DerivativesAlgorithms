#pragma once

#include "Array.h"

INLINE int constexpr FileOf(int loc)
{
	return loc & 7;
}
INLINE constexpr int RankOf(int loc)
{
	return loc >> 3;
}
INLINE int NDiagOf(int loc)
{
	return 7 - FileOf(loc) + RankOf(loc);
}
INLINE int SDiagOf(int loc)
{
	return FileOf(loc) + RankOf(loc);
}

INLINE constexpr int Dist(int x, int y)
{
	return Max(Abs(RankOf(x) - RankOf(y)), Abs(FileOf(x) - FileOf(y)));
}

namespace Calc
{
	constexpr uint64 FileA = 0x0101010101010101;
	constexpr uint64 Line0 = 0x00000000000000FF;
	constexpr uint64 NDiagMain = 0x8040201008040201;
	constexpr uint64 SDiagMain = 0x0102040810204080;

	constexpr std::array<uint64, 8> File = { Calc::FileA, Calc::FileA << 1, Calc::FileA << 2, Calc::FileA << 3, Calc::FileA << 4, Calc::FileA << 5, Calc::FileA << 6, Calc::FileA << 7 };
	constexpr std::array<uint64, 8> Line = { Calc::Line0, (Calc::Line0 << 8), (Calc::Line0 << 16), (Calc::Line0 << 24), (Calc::Line0 << 32), (Calc::Line0 << 40), (Calc::Line0 << 48), (Calc::Line0 << 56) };

	template<typename PRED_> constexpr uint64 BuildBB(PRED_ pred, int sq, int n = 64)
	{
		return n > 0 ? BuildBB<PRED_>(pred, sq, n - 1) | (pred(sq, n - 1) ? Bit(n - 1) : 0ull) : 0ull;
	}

	constexpr uint64 HLine(int sq)
	{
		return Line[RankOf(sq)] ^ Bit(sq);
	}
	constexpr uint64 VLine(int sq)
	{
		return File[FileOf(sq)] ^ Bit(sq);
	}
	constexpr uint64 Diag2(uint64 main, int which)
	{
		return which < 0 ? main >> (-8 * which) : main << (8 * which);
	}
	constexpr uint64 NDiag(int sq)
	{
		return Diag2(NDiagMain, RankOf(sq) - FileOf(sq)) ^ Bit(sq);
	}
	constexpr uint64 SDiag(int sq)
	{
		return Diag2(SDiagMain, RankOf(sq) + FileOf(sq) - 7) ^ Bit(sq);
	}

	constexpr uint64 PAttW(int sq)
	{
		return (FileOf(sq) > 0 ? Bit(sq) << 7 : 0ull) | (FileOf(sq) < 7 ? Bit(sq) << 9 : 0ull);
	}
	constexpr uint64 PAttB(int sq)
	{
		return (FileOf(sq) > 0 ? Bit(sq) >> 9 : 0ull) | (FileOf(sq) < 7 ? Bit(sq) >> 7 : 0ull);
	}
	constexpr bool IsNAtt(int from, int to)
	{
		return Square(RankOf(from) - RankOf(to)) + Square(FileOf(from) - FileOf(to)) == 5;
	}
	constexpr uint64 NAtt(int sq)
	{
		return BuildBB(IsNAtt, sq);
	}
	constexpr bool IsKAtt(int from, int to)
	{
		return Dist(from, to) == 1;
	}
	constexpr uint64 KAtt(int sq)
	{
		return BuildBB(IsKAtt, sq);
	}

	constexpr uint64 West(int file)
	{
		return file > 0 ? File[file - 1] | West(file - 1) : 0ull;
	}
	constexpr uint64 East(int file)
	{
		return file < 7 ? File[file + 1] | East(file + 1) : 0ull;
	}
	constexpr uint64 ForwardW(int rank)
	{
		return rank < 7 ? Line[rank + 1] | ForwardW(rank + 1) : 0ull;
	}
	constexpr uint64 ForwardB(int rank)
	{
		return rank > 0 ? Line[rank - 1] | ForwardB(rank - 1) : 0ull;
	}

	constexpr bool IsBetween(int from, int sq, int to)
	{
		return ((from > sq && sq > to) || (from < sq&& sq < to))
			&& ((RankOf(from) == RankOf(sq) && RankOf(sq) == RankOf(to))
				|| (FileOf(from) == FileOf(sq) && FileOf(sq) == FileOf(to))
				|| (NDiagOf(from) == NDiagOf(sq) && NDiagOf(sq) == NDiagOf(to))
				|| (SDiagOf(from) == SDiagOf(sq) && SDiagOf(sq) == SDiagOf(to)));
	}
}	// leave Calc
constexpr std::array<uint64, 64> HLine = make_array<64>(Calc::HLine);
constexpr std::array<uint64, 64> VLine = make_array<64>(Calc::VLine);
constexpr std::array<uint64, 64> NDiag = make_array<64>(Calc::NDiag);
constexpr std::array<uint64, 64> SDiag = make_array<64>(Calc::SDiag);
constexpr std::array<uint64, 64> NAtt = make_array<64>(Calc::NAtt);
constexpr std::array<uint64, 64> KAtt = make_array<64>(Calc::KAtt);
constexpr std::array<std::array<uint64, 8>, 2> Forward = { make_array<8>(Calc::ForwardW), make_array<8>(Calc::ForwardB) };

namespace Calc
{
	constexpr uint64 RMask(int sq)
	{
		return ::HLine[sq] | ::VLine[sq];
	}
	constexpr uint64 BMask(int sq)
	{
		return ::NDiag[sq] | ::SDiag[sq];
	}

	constexpr bool IsNAttAtt(int from, int to)
	{
		return !!(::NAtt[from] & ::NAtt[to]);
	}
	constexpr uint64 NAttAtt(int sq)
	{
		return BuildBB(IsNAttAtt, sq, 64);
	}

	constexpr bool IsKAttAtt(int from, int to)
	{
		return !!(::KAtt[from] & ::KAtt[to]);
	}
	constexpr uint64 KAttAtt(int sq)
	{
		return BuildBB(IsKAttAtt, sq, 64);
	}

	template<bool me> constexpr uint64 PWay(int sq)
	{
		return File[FileOf(sq)] & Forward[me][RankOf(sq)];
	}
}

constexpr std::array<uint64, 64> RMask = make_array<64>(Calc::RMask);
constexpr std::array<uint64, 64> BMask = make_array<64>(Calc::BMask);


namespace Calc
{
	constexpr uint64 QMask(int sq)
	{
		return ::RMask[sq] | ::BMask[sq];
	}


	INLINE uint64 ShiftNW(const uint64& target)
	{
		return (target & (~(File[0] | Line[7]))) << 7;
	}
	INLINE uint64 ShiftNE(const uint64& target)
	{
		return (target & (~(File[7] | Line[7]))) << 9;
	}
	INLINE uint64 ShiftSE(const uint64& target)
	{
		return (target & (~(File[7] | Line[0]))) >> 7;
	}
	INLINE uint64 ShiftSW(const uint64& target)
	{
		return (target & (~(File[0] | Line[0]))) >> 9;
	}
}
