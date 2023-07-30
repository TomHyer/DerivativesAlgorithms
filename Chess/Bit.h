#pragma once

#include <intrin.h>
// Bit utilities
#include "Chess.h"

#ifndef HNI
INLINE void Cut(uint64& x)
{
	x &= x - 1;
}
#else
INLINE void Cut(uint64& x)
{
	x = _blsr_u64(x);
}
#endif
constexpr void Cut_c(uint64& x)
{
	x &= x - 1;
}

INLINE int lsb(uint64 x)
{
	unsigned long y;
	_BitScanForward64(&y, x);
	return y;
}

INLINE int msb(uint64 x)
{
	unsigned long y;
	_BitScanReverse64(&y, x);
	return y;
}

constexpr int lsb_c(uint64 x)
{
	int retval = 0;
	while (x && !(x & 1))
		++retval, x >>= 1;
	return retval;
}

constexpr int msb_c(uint64 x)
{
	int retval = 0;
	while (x > 1)
		++retval, x >>= 1;
	return retval;
}

INLINE int popcnt(uint64 x)
{
	x = x - ((x >> 1) & 0x5555555555555555);
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (x * 0x0101010101010101) >> 56;
}

INLINE constexpr uint64 Bit(int loc)
{
	return 1ull << loc;
}

INLINE bool Multiple(const uint64& b)
{
	return T(b & (b - 1));
}
INLINE bool Single(const uint64& b)
{
	return F(b & (b - 1));
}
INLINE bool HasBit(const uint64& bb, int loc)
{
	return T(bb & Bit(loc));
}

INLINE uint32 High32(const uint64& x)
{
	return (uint32)(x >> 32);
}
INLINE uint32 Low32(const uint64& x)
{
	return (uint32)(x);
}

typedef int(*pop_func_t)(const uint64&);
inline int pop0(const uint64& b) { return popcnt(b); }
struct pop0_
{
	INLINE int operator()(const uint64& b) const { return pop0(b); }
	INLINE pop_func_t Imp() const { return pop0; }
};
inline int pop1(const uint64& b) { return static_cast<int>(_mm_popcnt_u64(b)); }
struct pop1_
{
	INLINE int operator()(const uint64& b) const { return pop1(b); }
	INLINE pop_func_t Imp() const { return pop1; }
};
