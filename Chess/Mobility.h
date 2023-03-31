#pragma once

#include "Eval.h"

template<class T_> void InitMobility_Gull
	(const std::array<int, 12>& coeffs,
	 int n_locus,
	 T_* mob)
{
	// ordering of coeffs is (linear*4, log*4, locus*4)
	auto m1 = [&](int phase, int pop)->sint16
	{
		double val = pop * (coeffs[phase] - coeffs[phase + 8]) + log(1.0 + pop) * coeffs[phase + 4];
		return static_cast<sint16>(val / 64.0);
	};
	auto m2 = [&](int pop)->packed_t
	{
		return Pack(m1(0, pop), m1(1, pop), m1(2, pop), m1(3, pop));
	};
	auto l1 = [&](int phase, int pop)->sint16
	{
		return static_cast<sint16>(pop * coeffs[phase + 8] / double(n_locus));
	};
	auto l2 = [&](int pop)->packed_t
	{
		return Pack(l1(0, pop), l1(1, pop), l1(2, pop), l1(3, pop));
	};

	const auto p_max = (*mob)[0].size();
	for (int pop = 0; pop < p_max; ++pop)
	{
		(*mob)[0][pop] = m2(pop);
		(*mob)[1][pop] = l2(pop);
	}
}

template<class T_> void InitMobility_Roc_1_7
	(const std::array<int, 12>& coeffs,
	 int n_locus,
	 T_* mob)
{
	const size_t n = (*mob)[1].size() - 1;
	const double c1 = n * log(double(n)) - (n - 1) * log(double(n - 1));
	const double c2 = 1.0 - 1.0 / c1;
	// ordering of coeffs is (d(first)*4, d(last)*4, locus*4)
	auto m1 = [&](int phase, int pop)->sint16
	{
		double d1 = coeffs[phase], d2 = coeffs[phase + 4], dLocus = coeffs[phase + 8];
		double p = pow(1 + log(d2 / d1) / (c1 * c2), c2);	// reproduces the desired ratio
		double val = d1 * pow(pop, p) - n_locus * pop * dLocus / 64;
		return static_cast<sint16>(0.4 * val);	// coeffs are in millipawns
	};
	auto m2 = [&](int pop)->sint64
	{
		return Pack(m1(0, pop), m1(1, pop), m1(2, pop), m1(3, pop));
	};
	auto l1 = [&](int phase, int pop)->sint16
	{
		return static_cast<sint16>(0.4 * pop * coeffs[phase + 8]);	// coeffs are in millipawns
	};
	auto l2 = [&](int pop)->sint64
	{
		return Pack(l1(0, pop), l1(1, pop), l1(2, pop), l1(3, pop));
	};

	const auto p_max = (*mob)[0].size();
	for (int pop = 0; pop < p_max; ++pop)
	{
		(*mob)[0][pop] = m2(pop);
		(*mob)[1][pop] = l2(pop);
	}
}

