#pragma once

#include "Chess.h"
#include "Array.h"
#include "BoardImpl.h"

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

