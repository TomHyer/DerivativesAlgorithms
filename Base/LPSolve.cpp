#include "__Platform.h"
#include <cmath>
#include <limits>
#include <array>
#include "Vectors.h"
#include "Numerics.h"

/* based on Numerical Recipes Webnote */

#ifdef ZZZ

namespace
{
    struct DenseHandler_
    {
        Vector_<Vector_<>> cols_;

        void Set(const Matrix_<>& sa)
        {
            cols_.Resize(sa.Cols());
            for (int ic = 0; ic < sa.Cols(); ++ic)
                cols_[ic].Assign(sa.Col(ic).begin(), sa.Col(ic).end());
        }

    };

    struct NRSparseHandler_
    {
        void* Decompose()
        {
            auto retval = new LU_(m_, NumNonzero(sa_));

            for (int ii = 0; ii < m_; ++ii)
                retval->LoadCol(ii, array<int, 2>{0, ii}, value);
            retval->Factorize();
            return retval;
        }

    };
}


namespace
{
    const Vector_<>& Column(const Vector_<Vector_<>>& a, int k)
    {
        return a[k];
    }

    double ColumnDot(const Vector_<Vector_<>>& a, int k, const Vector_<>& x)
    {
        return InnerProduct(a[k], x);
    }
}

template<class COLS_> Vector_<> Scale(const COLS_& a)
{
    Vector_<> retval(n + m, 0.0);
    for (int k = 0; k < n; ++k)
    {
        auto x = Column(a, k);
        double h = Max(*MaxElement(x), -*MinElement(x));
        if (h == 0.0)
            retval[k] = 0.0;
        else
        {
            retval[k] = 1.0 / h;
            for (int ii = 0; ii < m; ++ii)
                retval[n + ii] = Max(retval[n + ii], abs(x[ii]) * retval[k]);
        }
    }
    for (int ii = 0; ii < m; ++ii)
        if (retval[ii + n] == 0.0)
            retval[ii + n] = 1.0;

    return retval;
}

template<class MATRIX_, class COLS_, class LU_> struct Simplex_
{
    static constexpr int MAX_FACTORIZATIONS = 10000, MAX_UPDATES = 50;
    static constexpr double EPS = std::numeric_limits<double>::epsilon();
    static constexpr double EPS_SMALL = 1.0e5 * EPS, EPS_ARTIF = 1.0e5 * EPS;
    static constexpr double EPS_FEASIBLE = 1.0e8 * EPS, EPS_OPT = 1.0e8 * EPS;
    static constexpr double EPS_INFEASIBLE = 1.0e4 * EPS, EPS_ROW1 = 0.00001, EPS_ROW2 = EPS, EPS_ROW3 = EPS;
    int m_, n_, ierr_;
    Vector_<int> initvars_, ord_, ad_;
    // only columns with ad>0 can enter the basis; only those with ord>0 can leave
    Vector_<> b_, obj_, u_, x_, xb_, v_, w_, scale_;
    MATRIX_ sa_;

    int nm1_, nMax_, nSteps_;
    COLS_ a_;
    std::unique_ptr<LU_> lu_;

    void initialize()
    {
        constexpr array<double, 2> value = { 0.0, 1.0 };
        nSteps_ = 0;
        nm1_ = n_ + m_ + 1;
        nMax_ = MAX_FACTORIZATIONS * Max(m_, n_);
        ad_ = Apply([](int x) { return x == 0 ? 0 : 1; }, initvars_);
        ad_.Append(Vector_<int>(m_, 1));
        ord_ = Apply([&](int ii) { return initvars_[ii + n_] >= 0 ? ii + n : ii - m_ - 1; }, std::views::iota(0, m_));

        a_.Set(sa_);
        lu_ = a_.Decompose();
        

        v_.Resize(m_);
    }

    void Phase0()
    {
        // ends when all free variables are in the basis, and all zero variables are out
        for (int kp = 0; kp < n_; ++kp) // loop over columns
        {
            if (initvars_[kp] < 0)
            {   // free variable
                x = lx(kp);    // L x = a_k
                w = lu_->uinv(x);   // A_B w = a_k
                int ip = rowpiv(w, 0, kp, 0.0); // will leave
                int ind = ord_[ip];
                this->transform(x, ip, kp);   // interchange columns
                ord_[ip] -= nm1_;
                if (initvars_[ind] == 0)
                    ad_[ind] = 0;   // if a zero variable left, keep it out
            }
        }
        for (int ip = 0; ip < m_; ++ip)  // loop over basis
        {
            if (int ind = ord_[ip]; ind < 0 || initvars_[ind] != 0)
                continue;
            v_.Fill(0.0);
            v_[ip] = 1.0;
            int kp = colpiv(v, 0, piv);
            if (fabs(piv) < EPS_SMALL)
            {
                xb = lu_->solve(b); // A_B x_B = b
                if (abs(xb[ip]) > EPS_ARTIF * maxnorm(xb))
                {
                    THROW("Zero artificial variable left in basis with nonzero rhs");
                }
                Say("Artificial variable remains: ip = " + ToString(ip));
                continue;
            }
            x = lx(kp);
            this->transform(x, ip, kp);
            if (ad_[ind] == 1)
                ad_[ind] = 0;   // zero variable removed and no longer eligible

        }
    }

    void Phase1()
    {   // find a feasible basis
        for ( ; ; )
        {
            xb = lu_->solve(b);
            double xbmax = maxnorm(xb);
            bool done = true;
            v_.Fill(0.0);
            for (int ii = 0; ii < m_; ++ii)
            {
                if (ord_[ii] > 0 && xb[ii] < -EPS_FEASIBLE * xbmax)
                {
                    v_[ii] = 1.0 / scale_[ord_[ii]];
                    done = false;
                }
            }
            if (done)
                break;  // and go to phase 2

            int kp = colpiv(v_, 1, piv);    // entering column
            int ip;
            bool first = true;
            for ( ; ; )
            {
                x = lx(kp);     // L x = a_k
                w = lu_->uinv(x);      // A_B w = a_k
                try
                {
                    ip = rowpiv(w, 1, kp, xbmax);
                }
                catch (Exception_&)
                {
                    if (!first)
                        throw;
                    first = false;
                    // attempt to recover
                    refactorize();
                }
            }
            this->transform(x, ip, kp);
            REQUIRE(nSteps_ < nMax_, "Exceeded n_steps limit");
        }
    }

    void Phase2()
    {   // find optimal feasible basis
        for ( ; ; )
        {
            auto objValue = [&](int io) { return -obj_[io > 0 ? io : io + nm1_]; };
            Transform(ord_, objValue, &v_);
            int kp = colpiv(v_, 2, piv);    // entering column
            if (piv > -EPS_OPT)
                break;  // success

            int ip;
            bool first = true;
            for ( ; ; )
            {
                x = lx(kp); // L x = a_k
                w = lu_->uinv(x);   // A_B w = a_k
                xb = lu_->solve(b_);    // A_b x_b = b
                double xbmax = maxnorm(xb);
                try
                {
                    ip = rowpiv(w, 2, kp, xbmax);
                }
                catch (Exception_&)
                {
                    if (!first)
                        throw;
                    first = false;
                    // attempt to recover
                    refactorize();
                }
            }
            this->transform(x, ip, kp); // interchange columns
            REQUIRE(nSteps_ < nMax_, "Exceeded max steps in optimization");
        }
    }

    template<int PHASE> pair<int, double> colpiv(const Vector_<>& v)
    {   // input v must contain negatives of objective function in the current basis
        x = lu_->solvet(v);  //A_B^T x = -c_B
        int kp;
        for (int k = 0; k < n_ + m_; ++k)
        {
            if (ad_[k] > 0)
            {   // candidate to enter
                double h1 = (k > n ? x[k - n] : xdotcol(x, k);
                if (PHASE == 2)
                    h1 += obj_[k];
                h1 *= scale[k];
                if (PHASE == 0 ? fabs(h1) > fabs(piv) : h1 < piv)
                {
                    piv = h1;
                    kp = k;
                }
            }
        }
        if (PHASE == 1)
        {
            double hs = 0.0;
            for (int k = 0; k < m_; ++k)
                hs += abs(x[k]) * scale[n + k];
            if (piv > -EPS_INFEASIBLE * hs)
                THROW("Infeasible (no pivot)");
        }
        return kp;
    }

    // find index ov variable to leave basis; input w = A_B^{-1} a_k where a_k is the incoming column
    template<int PHASE> int rowpiv(const Vector_<>& w, int kp, double xbmax)
    {
        int j = -1, ip = -1;
        double min = 0.0, piv = 0.0;
        for (int ii = 0; ii < m; ++ii)
        {
            int ind = ord_[ii];
            if (ind >= 0)   // free variables don't leave
            {
                if (fabs(w[ii]) * scale_[kp] <= EPS_ROW1 * scale_[ind])
                    continue;   // skip zero pivots
                if (PHASE == 0)
                    h2 = h1 = fabs(w[ii]);
                else
                {
                    double hMin = EPS_ROW2 * xbmax * scale_[ind] * m_;
                    h2 = abs(xb[ii]) < hMin ? hMin : xb[ii];    // handle degenerate case xb[ii] ~ 0
                    h1 = w[ii] / h2;
                }
                if (h1 > 0.0)
                {
                    if (h2 > 0.0 && h1 > piv)
                    {
                        piv = h1;
                        ip = ii;
                    }
                    else if (h2 * scale_[kp] < -EPS_ROW3 * scale_[ind] && (j < 0 || h1 < min))
                    {
                        REQUIRE(PHASE == 1, "Only applies in phase one");
                        min = h1;
                        j = ii;
                    }
                }
            }
        }
        if (min > piv)
        {
            piv = min;
            ip = j;
        }
        REQUIRE(ip >= 0, "Unbounded objective function (no pivot)");
        return ip;
    }

    void transform(const Vector_<>& x, int ip, int kp)
    {
        // move ip out of the basis, kp in.  x = lx(kp) on input
        ad_[ord_[ip]] = 1;  // eligible to reenter
        ad_[kp] = -1;       // in basis
        int oldOrd = ord_[ip];  // allow possible undo
        ord_[ip] = kp;
        ++nSteps_;
        if (nSteps_ % MAX_UPDATES)      // if MAX_UPDATES is reached, don't even try to not refactorize
        {
            if (int ok = lu_->update(x, ip); ok)    // Bartels-Golub update
                return;
            // try to recover by refactorizing, below
            ad_[oldOrd] = -1;
            ad_[kp] = 1;
            ord_[ip] = oldOrd;
        }
        refactorize();
    }

    double maxnorm(const Vector_<>& xb)
    {
        double retval = 0.0;
        for (int ii = 0; ii < m; ++ii)
        {
            int indv = ord_[ii] >= 0 ? ord_[ii] : ord_[ii] + nm1_;
            retval = Max(retval, fabs(xb[ii]) / scale_[indv]);
        }
        return retval == 0.0 ? 1.0 : retval;
    }

    void Solve(bool do_scale)
    {
        initialize();   // surely should be called from constructor?
        scale_ = do_scale ? Scale(a_) : Vector_<>(n_ + m_, 1.0);
        phase0();
        phase1();
        phase2();
        prepare_output();
    }

};



/* based on port of Tulip.jl */

namespace
{
	struct Point_
	{
		int m_;		// number of constraints
		int n_;		// number of variables
		int p_;		// total lower+upper bounds
		bool homogenousEmbedding_;

		// primal variables:
		Vector_<> x_, xl_, xu_;

		// dual variables:
		Vector_<> y_;
		Vector_<> zl_, zu_;

		// HSD variables for homogenous form:
		double tau_, kappa_;	// otherwise use (1, 0)

		// Centrality parameter:
		double mu_;

		void UpdateMy()
		{
			mu_ = (InnerProduct(xl_, zl_) + InnerProduct(xu_, zu_) + tau_ * kappa_) 
					/ (p_ + (homogenousEmbedding_ ? 1.0 : 0.0));
		}

		Point_(int m, int n, int p, bool homogenous)
			:
		m_(m),
		n_(n),
		p_(p),
		homogenousEmbedding_(homogenous),
		x_(n, 0.0),
		xl_(n, 0.0),
		xu_(n, 0.0),
		y_(m, 0.0),
		zl_(n, 0.0),
		zu_(n, 0.0),
		tau_(1.0),
		kappa_(homogenousEmbedding_ ? 1.0 : 0.0),
		mu_(1.0)
		{	}
	};

	struct Residuals_
	{
		// primal residuals:
		Vector_<> rp_;	// = tau b - A x
		Vector_<> rl_;	// = tau l - (x - x_l)
		Vector_<> ru_;	// = tau u - (x + x_u)

		// dual residuals:
		Vector_<> rd_;	// = tau c - (A'y + z_l - z_u)
		double rg_;		// = c'x - (b'y + l'z_l - u'z_u) + kappa

		// norms of the above
		double rpNorm_, rlNorm_, ruNorm_, rdNorm_, rgNorm_;
	};

	struct Options_
	{
		int outputLevel_ = 0;
		
		// user limits
		int iterationsLimit_ = 100;
		double timeLimit_ = DA::INFINITY;

		// numerical tolerances
		double pFeasibleTol_ = sqrt(DA::EPSILON);
		double dFeasibleTol_ = sqrt(DA::EPSILON);
		double rGapTol_ = sqrt(DA::EPSILON);	// optimality
		double infeasibleTol_ = sqrt(DA::EPSILON);

		// algorithmic parameters
		int correctionLimit_ = 3;
		double stepDampFactor_ = 0.9995;
		double gammaMin_ = 0.1;
		double centralityOutlierThreshold_ = 0.1;

		double pRegMin_ = sqrt(DA::EPSILON);
		double dRegMin_ = sqrt(DA::EPSILON);

	};
}

#endif

// Simplex solver based on Petar Velickovic

namespace
{
    struct PetarVSimplex_
    {
        int n_, m_;
        Matrix_<> A_;
        Vector_<> b_, c_;
        double v_;
        Vector_<int> N_, B_;

        // PULP form:  Mx <= q therefore Mx - q <= 0 
        // slack form: Ax + b >= 0

        PetarVSimplex_(const Matrix_<>& A, const Vector_<>& b, const Vector_<>& c)
            :
        n_(A.Cols()),
        m_(A.Rows()),
        A_(A.Rows(), A.Cols() + 1), // need an extra column during initialization
        b_(b),
        c_(c),
        v_(0.0),
        N_(n_),
        B_(m_)
        {
            for (int ir = 0; ir < A.Rows(); ++ir)
                std::copy(A.Row(ir).begin(), A.Row(ir).end(), A_.Row(ir).begin());
            A_ *= -1.0;
        }

        /*
         Petar 'PetarV' Velickovic
         Algorithm: Simplex Algorithm
        */

        /*
         The Simplex algorithm aims to solve a linear program - optimising a linear function subject
         to linear constraints. As such it is useful for a very wide range of applications.

         N.B. The linear program has to be given in *slack form*, which is as follows:
         maximise
             c_1 * x_1 + c_2 * x_2 + ... + c_n * x_n + v
         subj. to
             a_11 * x_1 + a_12 * x_2 + ... + a_1n * x_n + b_1 = s_1
             a_21 * x_1 + a_22 * x_2 + ... + a_2n * x_n + b_2 = s_2
             ...
             a_m1 * x_1 + a_m2 * x_2 + ... + a_mn * x_n + b_m = s_m
         and
             x_1, x_2, ..., x_n, s_1, s_2, ..., s_m >= 0

         Every linear program can be translated into slack form; the parameters to specify are:
             - the number of variables, n, and the number of constraints, m;
             - the matrix A = [[A_11, A_12, ..., A_1n], ..., [A_m1, A_m2, ..., A_mn]];
             - the vector b = [b_1, b_2, ..., b_m];
             - the vector c = [c_1, c_2, ..., c_n] and the constant v.

         Complexity:    O(m^(n/2)) worst case
                        O(n + m) average case (common)
        */

        // pivot yth variable around xth constraint
        void pivot(int x, int y)
        {
            //printf("Pivoting variable %d around constraint %d.\n", y, x);

            // first rearrange the x-th row
            for (int j = 0; j < n_; j++)
            {
                if (j != y)
                {
                    A_(x, j) /= -A_(x, y);
                }
            }
            b_[x] /= -A_(x, y);
            A_(x, y) = 1.0 / A_(x, y);

            // now rearrange the other rows
            for (int i = 0; i < m_; i++)
            {
                if (i != x)
                {
                    for (int j = 0; j < n_; j++)
                    {
                        if (j != y)
                            A_(i, j) += A_(i, y) * A_(x, j);
                    }
                    b_[i] += A_(i, y) * b_[x];
                    A_(i, y) *= A_(x, y);
                }
            }

            // now rearrange the objective function
            for (int j = 0; j < n_; j++)
            {
                if (j != y)
                {
                    c_[j] += c_[y] * A_(x, j);
                }
            }
            v_ += c_[y] * b_[x];
            c_[y] *= A_(x, y);

            // finally, swap the basic & nonbasic variable
            std::swap(B_[x], N_[y]);
        }

        enum {OK, STOP, UNBOUNDED};

        // Run a single iteration of the simplex algorithm.
        // Returns: 0 if OK, 1 if STOP, -1 if UNBOUNDED
        int iterate_simplex()
        {
            if (false)
            {
                printf("--------------------\n");
                printf("State:\n");
                printf("Maximise: ");
                for (int j = 0; j < n_; j++) printf("%lfx_%d + ", c_[j], N_[j]);
                printf("%lf\n", v_);
                printf("Subject to:\n");
                for (int i = 0; i < m_; i++)
                {
                    for (int j = 0; j < n_; j++) printf("%lfx_%d + ", A_(i, j), N_[j]);
                    printf("%lf = x_%d\n", b_[i], B_[i]);
                }

                // getchar(); // uncomment this for debugging purposes!
            }

            int ind = -1, best_var = -1;
            for (int j = 0; j < n_; j++)
            {
                if (c_[j] > 0)
                {
                    if (best_var == -1 || N_[j] < ind)
                    {
                        ind = N_[j];
                        best_var = j;
                    }
                }
            }
            if (ind == -1)
                return STOP;

            double max_constr = DA::INFINITY;
            int best_constr = -1;
            for (int i = 0; i < m_; i++)
            {
                if (A_(i, best_var) < 0.0)
                {
                    double curr_constr = -b_[i] / A_(i, best_var);
                    if (curr_constr < max_constr)
                    {
                        max_constr = curr_constr;
                        best_constr = i;
                    }
                }
            }
            REQUIRE(max_constr != DA::INFINITY, "Unbounded LP problem");
            pivot(best_constr, best_var);

            return OK;
        }

        // (Possibly) converts the LP into a slack form with a feasible basic solution.
        // Returns 0 if OK, -1 if INFEASIBLE
        int initialise_simplex()
        {
            auto minB = MinElement(b_);

            if (*minB >= 0) // basic solution feasible!
            {
                for (int j = 0; j < n_; j++) 
                    N_[j] = j;
                for (int i = 0; i < m_; i++) 
                    B_[i] = n_ + i;
                return OK;
            }

            // generate auxiliary LP
            ++n_;
            for (int j = 0; j < n_; j++) 
                N_[j] = j;
            for (int i = 0; i < m_; i++) 
                B_[i] = n_ + i;

            // store the objective function
            Vector_<> c_old(c_);
            double v_old = v_;

            // aux. objective function
            c_.Fill(0.0);
            c_.back() = -1.0;
            v_ = 0;
            // aux. coefficients
            for (int i = 0; i < m_; i++) 
                A_(i, n_ - 1) = 1;

            // perform initial pivot
            int k = static_cast<int>(minB - b_.begin());
            pivot(k, n_ - 1);

            // now solve aux. LP
            for ( ; ; )
            {
                if (iterate_simplex() != OK)
                    break;
            }

            REQUIRE(IsZero(v_), "Infeasible LP problem");

            if (int z_basic = static_cast<int>(std::find(B_.begin(), B_.begin() + m_, n_ - 1) - B_.begin()); z_basic < m_) 
                pivot(z_basic, n_ - 1);

            auto z_nonbasic = static_cast<int>(std::find(N_.begin(), N_.begin() + n_, n_ - 1) - N_.begin());
            REQUIRE(z_nonbasic < n_, "No starting pivot in LP");

            for (int i = 0; i < m_; i++)
            {
                A_(i, z_nonbasic) = A_(i, n_ - 1);
            }
            std::swap(N_[z_nonbasic], N_[n_ - 1]);
            --n_;

            for (int j = 0; j < n_; j++) 
                if (N_[j] > n_) 
                    --N_[j];
            for (int i = 0; i < m_; i++) 
                if (B_[i] > n_) 
                    --B_[i];

            c_.Fill(0.0);
            v_ = v_old;

            for (int j = 0; j < n_; j++)
            {
                auto jj = std::find(N_.begin(), N_.begin() + n_, j) - N_.begin();
                if (jj < n_)
                {
                    c_[jj] += c_old[j];
                }
                else
                { 
                    int i = static_cast<int>(std::find(B_.begin(), B_.begin() + m_, j) - B_.begin());
                    for (int jj = 0; jj < n_; jj++)
                    {
                        c_[jj] += c_old[j] * A_(i, jj);
                    }
                    v_ += c_old[j] * b_[i];
                }
            }

            return OK;
        }

        // Runs the simplex algorithm to optimise the LP.
        // Returns a vector of -1s if unbounded, -2s if infeasible.
        pair<Vector_<>, double> simplex()
        {
            initialise_simplex();
            for ( ; iterate_simplex() != STOP; );

            Vector_<> retval(n_ + m_);
            for (int j = 0; j < n_; j++)
            {
                retval[N_[j]] = 0;
            }
            for (int i = 0; i < m_; i++)
            {
                retval[B_[i]] = b_[i];
            }

            return make_pair(retval, v_);
        }
    };
}

Vector_<> LPSolve
    (const Matrix_<>& A,
     const Vector_<>& b,
     const Vector_<>& c)
{
    PetarVSimplex_ solver(A, b, c);
    auto [retval, v] = solver.simplex();
    return retval;
}

