#pragma once

#include <array>
#include <numeric>
#include "Bit.h"
#include "Board.h"

template<class T_> uint64 MakeLocus(const T_& merit, int n_locus)
{	// LOWEST merit is best
	if (n_locus <= 0)
		return 0ull;
	std::array<std::pair<double, int>, 64> temp;
	for (int ii = 0; ii < 64; ++ii)
		temp[ii] = { merit(ii), ii};
	sort(temp.begin(), temp.end());
	uint64 retval = 0ull;
	int ii = n_locus;
	// include elements tied with the cutoff
	while (ii < 64 && temp[ii].first == temp[n_locus - 1].first)
		++ii;
	while (ii)
		retval |= Bit(temp[--ii].second);
	return retval;
}

template<class T_> std::array<uint64, 64> MakeLoci(const T_& merit, int n_locus)
{
	std::array<uint64, 64> retval;
	for (int ii = 0; ii < 64; ++ii)
	{
		auto meritJ = [=](int j) { return merit(ii, j); };
		retval[ii] = MakeLocus(meritJ, n_locus);
	}
	return retval;
}

//----------------------------------------------------------------------------
// helper functions for various Loci

namespace Locus
{
	inline double KDist(int x, int y)
	{
		return sqrt(1.0 * Square(RankOf(x) - RankOf(y)) + Square(FileOf(x) - FileOf(y)));
	}

	inline double CenterDist(int y)
	{
		return sqrt(1.0 * Square(3.5 - RankOf(y)) + Square(3.5 - FileOf(y)));
	}

	struct RDist_
	{
		double rankWt_, fileWt_;
		RDist_(double rank_wt, double file_wt) : rankWt_(rank_wt), fileWt_(file_wt) {}
		inline double operator()(int x, int y) const
		{
			double retval = KDist(x, y);
			if (fabs(RankOf(x) - 3.5) > 2 && abs(RankOf(x) ^ RankOf(y)) < 2)
				retval -= rankWt_;
			if (fabs(FileOf(x) - 3.5) > 2 && abs(FileOf(x) ^ FileOf(y)) < 2)
				retval -= fileWt_;
			return retval;
		}
	};

	struct MinorDist_
	{
		const std::array<uint64, 64> att_;
		double attWt_, attAttWt_, centerWt_;
		MinorDist_(const std::array<uint64, 64>& att, double att_wt, double att_att_wt, double center_wt = 0.0) : att_(att), attWt_(att_wt), attAttWt_(att_att_wt), centerWt_(center_wt) {}
		inline double operator()(int x, int y) const
		{
			double retval = KDist(x, y) + centerWt_ * CenterDist(y);
			if (att_[y] & Bit(x))
				retval -= attWt_;
			else if (att_[y] & KAtt[x])
				retval -= attAttWt_;
			return retval;
		}
	};

	template<class D_, class W_> struct SummedDist_
	{
		D_ parts_;
		W_ weights_;
		SummedDist_(const D_& parts, const W_& weights) : parts_(parts), weights_(weights) {}

		inline double operator()(int x, int y) const
		{
			double retval = 0.0;
			for (int ii = 0; ii < parts_.size(); ++ii)
				retval += weights_[ii] * parts_[ii](x, y);
			return retval;
		}
	};
}
