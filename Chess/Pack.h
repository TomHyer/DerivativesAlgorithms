#pragma once

#include "Chess.h"

INLINE constexpr sint64 Pack(sint16 op, sint16 mid, sint16 eg, sint16 asym)
{
	return op + (static_cast<sint64>(mid) << 16) + (static_cast<sint64>(eg) << 32) + (static_cast<sint64>(asym) << 48);
}

template<int which> sint16 Pick16(sint64 x);	// instantiated only for 1..4
template<> INLINE sint16 Pick16<1>(sint64 x) { return static_cast<sint16>(x & 0xFFFF); }
template<> INLINE sint16 Pick16<2>(sint64 x) { return static_cast<sint16>((x >> 16) + ((x >> 15) & 1)); }
template<> INLINE sint16 Pick16<3>(sint64 x) { return static_cast<sint16>((x >> 32) + ((x >> 31) & 1)); }
template<> INLINE sint16 Pick16<4>(sint64 x) { return static_cast<sint16>((x >> 48) + ((x >> 47) & 1)); }

INLINE constexpr uint32 Pack(uint16 x, uint16 y)
{
	return x + (static_cast<uint32>(y) << 16);
}
template<int which> uint16 Pick16(uint32 x);	// instantiated only for 1..2
template<> INLINE uint16 Pick16<1>(uint32 x)
{
	return static_cast<uint16>(x & 0xFFFF);
}
template<> INLINE uint16 Pick16<2>(uint32 x)
{
	return static_cast<sint16>(x >> 16);
}
