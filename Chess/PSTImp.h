#pragma once

#include <array>
#include "Bit.h"
#include "Eval.h"
#include "Roc.h"

namespace Calc
{
	// helper to divide intermediate quantities to form scores
	// note that straight integer division (a la Gull) creates an attractor at 0
	// we support this, especially for weights inherited from Gull which have not been tuned for Roc
	template<int DEN, int SINK = DEN> struct Div_
	{
		constexpr int operator()(int x) const
		{
			constexpr int shift = std::numeric_limits<int>::max() / (2 * DEN);
			constexpr int shrink = (SINK - DEN) / 2;
			const int y = x > 0 ? Max(0, x - shrink) : Min(0, x + shrink);
			return (y + DEN * shift) / DEN - shift;
		}
	};

	constexpr std::array<std::array<packed_t, 64>, 16> MakePst(std::array<const PstW::Weights_*, 6> weights)
	{
		constexpr std::array<sint8, 8> DistC = { 3, 2, 1, 0, 0, 1, 2, 3 };
		constexpr std::array<sint8, 8> RankR = { -3, -2, -1, 0, 1, 2, 3, 4 };

		std::array<std::array<packed_t, 64>, 16> retval = {};
		for (int i = 0; i < 64; ++i)
		{
			int r = RankOf(i);
			int f = FileOf(i);
			int d = f > r ? f - r : r - f;
			int e = f + r > 7 ? f + r - 7 : 7 - f - r;
			std::array<int, 4> distL = { DistC[f], DistC[r],  RankR[d] + RankR[e], RankR[r] };
			std::array<int, 4> distQ = { DistC[f] * DistC[f], DistC[r] * DistC[r], RankR[d] * RankR[d] + RankR[e] * RankR[e], RankR[r] * RankR[r] };
			std::array<int, 2> distM = { DistC[f] * DistC[r], DistC[f] * RankR[r] };
			
			for (int j = 2; j < 16; j += 2)
			{
				int index = PieceType[j];
				const PstW::Weights_& src = *weights[index];
				int op = 0, md = 0, eg = 0, cl = 0;
				for (int k = 0; k < 2; ++k)
				{
					op += src.op_.quadMixed_[k] * distM[k];
					md += src.md_.quadMixed_[k] * distM[k];
					eg += src.eg_.quadMixed_[k] * distM[k];
					cl += src.cl_.quadMixed_[k] * distM[k];
				}
				for (int k = 0; k < 4; ++k)
				{
					op += src.op_.quad_[k] * distQ[k] + src.op_.linear_[k] * distL[k];
					md += src.md_.quad_[k] * distQ[k] + src.md_.linear_[k] * distL[k];
					eg += src.eg_.quad_[k] * distQ[k] + src.eg_.linear_[k] * distL[k];
					cl += src.cl_.quad_[k] * distQ[k] + src.cl_.linear_[k] * distL[k];
				}
				// Regularize(&op, &md, &eg);
				Div_<64> d64;
				retval[j][i] = Pack(d64(op), d64(md), d64(eg), d64(cl));
			}
		}

		retval[WhiteKnight][56] -= Pack(100 * CP_EVAL, 0);
		retval[WhiteKnight][63] -= Pack(100 * CP_EVAL, 0);
		// now for black
		for (int i = 0; i < 64; ++i)
			for (int j = 3; j < 16; j += 2)
			{
				auto src = retval[j - 1][63 - i];
				retval[j][i] = Pack(-Opening(src), -Middle(src), -Endgame(src), -Closed(src));
			}

		return retval;
	}
}
