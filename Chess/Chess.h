#pragma once

// Platform for all chess development
#include "../Base/Platform.h"
#define INLINE __forceinline

typedef unsigned char uint8;
typedef char sint8;
typedef unsigned short uint16;
typedef short sint16;
typedef unsigned int uint32;
typedef int sint32;
typedef unsigned long long uint64;
typedef long long sint64;

template<class T> INLINE constexpr int Sgn(const T& x)
{
	return x == 0 ? 0 : (x > 0 ? 1 : -1);
}
template<class C> INLINE constexpr bool T(const C& x)
{
	return x != 0;
}
template<class T> INLINE constexpr bool F(const T& x)
{
	return x == 0;
}
template<class T> INLINE constexpr bool Even(const T& x)
{
	return F(x & 1);
}
template<class C> INLINE constexpr bool Odd(const C& x)
{
	return T(x & 1);
}
template<class T> INLINE constexpr T Abs(const T& x)
{
	return x > 0 ? x : -x;
}

constexpr uint8 White = 0;
constexpr uint8 Black = 1;

// by convention, mover/evaluator is 'me'
#define opp (1 ^ (me))
#define MY_TURN(state) (!(state).current_->turn == opp)
#define TEMPLATE_ME(f) (me ? f<1> : f<0>)
