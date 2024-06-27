
// public interface for testing
	// very simple functions to test FFI
#include "__Platform.h"
#include <cmath>
#include "Numerics.h"
#include "LPSolve.h"

namespace
{
/*IF--------------------------------------------------------------------------
public Lorentz_Add
	Computes tanh(arctanh X + B * arctanh Y)
&inputs
X is number
	&fabs($) <= 1.0\$ must be in [-1, 1]
	as above
Y is number
	&fabs($) < 1.0\$ must be in (-1, 1)
	as above
&optional
B is number (1.0)
	&$ >= 0.0
	applies to Y only
&outputs
Z is number
	Will also be in (-1, 1) (or at the boundary, if X is)
-IF-------------------------------------------------------------------------*/

	void Lorentz_Add
		(double x,
		 double y,
		 double b,
		 double* z)
	{
		if (fabs(x) == 1.0)
			*z = x;
		else if (b == 1.0)
			*z = (x + y) / (1.0 + x * y);
		else
		{
			const double aZ = std::atanh(x) + b * std::atanh(y);
			*z = std::tanh(aZ);
		}
	}

/*IF--------------------------------------------------------------------------
public LP_Solve
	Solves LP problem
&inputs
A is number[][]
	constraint definitions
b is number[]
	&$.size() == A.Rows()
	constraints
c is number[]
	&$.size() == A.Cols()
	objective function
&outputs
best_x is number[]
	Solution
-IF-------------------------------------------------------------------------*/

	void LP_Solve
		(const Matrix_<>& A,
		 const Vector_<>& b,
		 const Vector_<>& c,
		 Vector_<>* x)
	{
		*x = LPSolve(A, b, c);
	}

/*IF--------------------------------------------------------------------------
public Play_The_Game
	computes returns to me
&inputs
in_play is number[]
	cards to fight over
my_hand is number[]
	&$.size() == in_play.size()
	I can read minds
his_hand is number[]
	&$.size() == in_play.size()
	He must randomize strategy
&outputs
value is number
	Value to me
-IF-------------------------------------------------------------------------*/

	Vector_<> HisStrategy(const Matrix_<>& payoff)
	{
		const int n = payoff.Rows();
		// set up LP problem to maximize a slack variable
		Matrix_<> A(n + 1, n + 1);
		double pMax = Max(*MaxElement(payoff.Row(0)), -*MinElement(payoff.Row(0)));
		for (int ii = 1; ii < n; ++ii)
			pMax = Max(pMax, Max(*MaxElement(payoff.Row(ii)), -*MinElement(payoff.Row(ii))));
		pMax += 1.0;
		auto toA = [=](double x) { return x - pMax; };
		for (int ii = 0; ii < n; ++ii)
		{
			for (int jj = 0; jj < n; ++jj)
				A(ii, jj) = toA(payoff(ii, jj));
			A(ii, n) = 1.0;	// slack variable cannot exceed expected payoff
			A(n, ii) = 1.0;	// sum-of-probabilities constraint
		}
		A(n, n) = 0.0;
		Vector_<> b(n + 1, pMax), c(n + 1, 0.0);
		b.back() = 1.0;	// unitarity
		c.back() = 1.0;	// maximize slack variable

		Vector_<> x = LPSolve(A, b, c);
		x.Resize(n);	// discard dual info
		if (Accumulate(x) < 0.99)
		{
			THROW("No probability");
		}
		return x;
	}

	double XPlay
		(int n,
		 Vector_<> in_play,
		 Vector_<> my_hand,
		 Vector_<> his_hand)
	{
		if (n == 1)
		{
			if (my_hand[0] == his_hand[0])
				return 0.0;
			return my_hand[0] > his_hand[0] ? in_play[0] : -in_play[0];
		}
		// make the payoff grid
		Matrix_<> payoff(n, n);
		for (int ii = 0; ii < n; ++ii)
		{
			std::swap(my_hand[ii], my_hand[n - 1]);
			double& iPlay = my_hand[n - 1];
			for (int jj = 0; jj < n; ++jj)
			{
				std::swap(his_hand[jj], his_hand[n - 1]);
				double& hePlays = his_hand[n - 1];
				double immediate = 0.0;
				if (iPlay != hePlays)
					immediate += (iPlay > hePlays ? 1 : -1)* in_play[n - 1];
				payoff(ii, jj) = immediate + XPlay(n - 1, in_play, my_hand, his_hand);
				std::swap(his_hand[jj], his_hand[n - 1]);
			}
			std::swap(my_hand[ii], my_hand[n - 1]);
		}
		// given this payoff matrix, he chooses a randomized strategy
		Vector_<> probs = HisStrategy(payoff);
		double best = -DA::INFINITY;
		for (int ii = 0; ii < n; ++ii)
		{
			double valToMe = InnerProduct(probs, payoff.Row(ii));
			best = Max(best, valToMe);
		}
		return best;
	}

	void Play_The_Game
		(Vector_<> in_play,
		 Vector_<> my_hand,
		 Vector_<> his_hand,
		 double* value)
	{
		*value = XPlay(in_play.size(), in_play, my_hand, his_hand);
	}

}

#include "MG_Lorentz_Add_public.inc"
#include "MG_Play_The_Game_public.inc"
#include "MG_LP_Solve_public.inc"

