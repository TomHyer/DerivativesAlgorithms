#pragma once

#include <array>
#include "Bit.h"
#include "Pack.h"
#include "Eval.h"

namespace PasserValues
{
	// Gull style, through Roc 1.6; inputs are 4-element arrays for each of const, linear, quadratic
	template<class T_> inline std::array<sint64, 8> InitGull(const T_& weights)
	{
		auto loc = [&](int rank) { return rank * (rank - 1) / 15.0 - 1.0; };	// in [-1, 1]
		auto part = [&](int rank, int which)
		{
			double l = loc(rank);
			double r = weights[1][which] + abs(l) * ((l > 0 ? weights[2][which] : weights[0][which]) - weights[1][which]);
			return static_cast<sint16>(r);
		};
		auto ppart = [&](int rank, int which) { return std::max<sint16>(0, part(rank, which)); };

		std::array<sint64, 8> retval = { 0ll,0ll,0ll,0ll,0ll,0ll,0ll,0ll };
		for (int ii = 1; ii < 7; ++ii)
			retval[ii] = Pack(ppart(ii, 0), ppart(ii, 1), ppart(ii, 2), part(ii, 3));	// [3] can be negative
		return retval;
	}
	inline std::array<sint64, 8> Init(const std::array<std::array<int, 4>, 3>& weights)
	{
		return InitGull(weights);
	}

	// Roc 1.7+; inputs are normalized to score units and grouped by phase
	namespace Impl
	{
		double Basis(int excess)  // unnormalized
		{
			return sqrt(Square(excess) + 0.5) + excess - 0.5;
		}
		double Basis(int rank, int offset)
		{
			return Basis(rank - offset) / Basis(6 - offset);
		}

		constexpr std::array<int, 3> PasserOffset = { -2, 2, 4 };
		sint16 Val(const int* weight, int rank)
		{
			double retval = *weight * Basis(rank, PasserOffset[0])
				+ *(weight + 1) * Basis(rank, PasserOffset[1])
				+ *(weight + 2) * Basis(rank, PasserOffset[2]);
			return static_cast<sint16>(retval);
		}
	}
	template<class T_> inline std::array<sint64, 8> InitRoc(const T_& weights)
	{
		std::array<sint64, 8> retval = { 0ll,0ll,0ll,0ll,0ll,0ll,0ll,0ll };
		auto phased = [&](int which, int rank) { return Impl::Val(&weights[3 * which], rank); };
		for (int rank = 1; rank < 7; ++rank)
			retval[rank] = Pack(phased(0, rank), phased(1, rank), phased(2, rank), phased(3, rank));
		return retval;
	}
	inline std::array<sint64, 8> Init(const std::array<int, 12>& weights)
	{
		return InitRoc(weights);
	}
}
