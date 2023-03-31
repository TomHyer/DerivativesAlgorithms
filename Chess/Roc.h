#pragma once

INLINE sint16 Opening(packed_t x) { return Pick16<1>(x); }
INLINE sint16 Middle(packed_t x) { return Pick16<2>(x); }
INLINE sint16 Endgame(packed_t x) { return Pick16<3>(x); }
INLINE sint16 Closed(packed_t x) { return Pick16<4>(x); }

#define TEMPLATE_ME(f) (me ? f<1> : f<0>)

constexpr int MAX_PHASE = 128;
constexpr int MIDDLE_PHASE = 64;
constexpr int PHASE_M2M = MAX_PHASE - MIDDLE_PHASE;

constexpr int MAX_HEIGHT = 128;
