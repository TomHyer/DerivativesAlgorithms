
#include "Base/Platform.h"
#include "KillerSudoku.h"
#include <time.h>
#include <set>
#include <map>
#include "Base/Optionals.h"
#include "Base/Strings.h"
#include "Base/Numerics.h"
#include "Base/Strict.h"

Vector_<short> All9()
{
	return Vector_<short>({ 1,2,3,4,5,6,7,8,9 });
}
Vector_<short> Reverse9()
{
	return Vector_<short>({ 9,8,7,6,5,4,3,2,1 });
}

int RandLessThan(int size)
{
	return (size * rand()) / (1 + RAND_MAX);
}

short YankRandom(Vector_<short>* v)
{
	assert(!v->empty());
	const int i = RandLessThan(v->size());	// which one was yanked
	const short retval = (*v)[i];
	(*v)[i] = v->back();
	v->pop_back();
	return retval;
}

struct Loc_
{
	short row_, col_, region_;
	int ICell() const { return row_ * 9 + col_; }
	Loc_(int i_cell) : row_(short(i_cell / 9)), col_(short(i_cell % 9)) { region_ = 3 * (row_ / 3) + (col_ / 3); }
	Loc_(short r, short c) : row_(r), col_(c) { region_ = 3 * (row_ / 3) + (col_ / 3); }
};
bool operator&(const Loc_& lhs, const Loc_& rhs) { return lhs.row_ == rhs.row_ || lhs.col_ == rhs.col_ || lhs.region_ == rhs.region_; }
bool operator<(const Loc_& lhs, const Loc_& rhs) { return lhs.row_ < rhs.row_ || (lhs.row_ == rhs.row_ && lhs.col_ < rhs.col_); }

struct Grid_
{
	Matrix_<short> vals_;
	Grid_() : vals_(9, 9) {}
	short& operator[](const Loc_& loc) { return vals_(loc.row_, loc.col_); }
	const short& operator[](const Loc_& loc) const { return vals_(loc.row_, loc.col_); }
};
bool operator==(const Grid_& lhs, const Grid_& rhs)
{
	for (int ii = 0; ii < 9; ++ii)
		for (int jj = 0; jj < 9; ++jj)
			if (lhs.vals_(ii, jj) != rhs.vals_(ii, jj))
				return false;
	return true;
}

struct Square_
{
	Loc_ loc_;
	short val_;
	Square_(int loc, short v) : loc_(loc), val_(v) {}
};

inline bool Conflicts(const Vector_<Square_>& sofar, const Square_& test)
{
    return CheckAny(sofar, [=](const Square_& pvs) { return pvs.val_ == test.val_ && (pvs.loc_ & test.loc_); });
}

Grid_ RandomGrid()
{
	Vector_<Square_> squares;
	Vector_<Vector_<short> > available(81, All9());

	while (squares.size() < 81)
	{
		const int c = squares.size();
		if (!available[c].empty())
		{
			Square_ test(c, YankRandom(&available[c]));		// whether it conflicts or not at present, remove it from future possibilities
			if (!Conflicts(squares, test))
				squares.push_back(test);
		}
		else
		{
			available[c] = All9();	// forget anything about the current square
			squares.pop_back();	// erase this square, backing up to retry the previous
		}
	}

	// produce the output grid
	Grid_ retval;
	for (auto ps = squares.begin(); ps != squares.end(); ++ps)
		retval.vals_(ps->loc_.row_, ps->loc_.col_) = ps->val_;
	return retval;
}

//----------------------------------------------------------------------------

struct Group_
{
	Vector_<Loc_> contents_;
	Group_() : contents_() {}
	Group_(const Vector_<Loc_>& src) : contents_(src) {}
	Vector_<Loc_>::const_iterator begin() const { return contents_.begin(); }
	Vector_<Loc_>::const_iterator end() const { return contents_.end(); }
	void operator+=(const Group_& rhs) { contents_.Append(rhs.contents_); }
};

struct RangeConstraint_
{
	std::optional<int> sofar_;	// location of previous cell in group
	short lo_;
	short hi_;
};

Grid_ LastSolution
	(const Vector_<Vector_<int> >& exclusions,	// for each location [0, 81), the prior locations which cannot share its value (same row, col, region or group)
	 const Vector_<RangeConstraint_>& limits,	// for each location [0, 81), a range constraint, possibly referring to a prior sum
	 const Vector_<short>& all_9)	// in lexicographical order
{
	Vector_<Vector_<short> > test;
	Vector_<short> sums;
	test.push_back(all_9);
	for ( ; ; )
	{
		const int ii = test.size() - 1;
		assert(sums.size() == ii);
		const RangeConstraint_& limit = limits[ii];
		const short pvsSum = limit.sofar_.has_value() ? sums[limit.sofar_.value()] : 0;
		const Vector_<int>& exclude = exclusions[ii];
		while (!test.back().empty())
		{
			const short v = test.back().back();	// value being tested
			// check sum
			const short s = v + pvsSum;
			bool failed = s < limit.lo_ || s > limit.hi_;
			for (auto pe = exclude.begin(); !failed && pe != exclude.end(); ++pe)
				failed = v == test[*pe].back();	// fail if our test value equals a previous test value
			if (!failed)
				break;	// keep this test value
			test.back().pop_back();
		}

		if (test.back().empty())
		{
			// failure -- back up
			test.pop_back();
			assert(!test.empty());	// unsolvable
			test.back().pop_back();	// this guess failed
			sums.pop_back();
		}
		else
		{
			// success -- move forward
			sums.push_back(pvsSum + test.back().back());
			// are we done?
			if (sums.size() == 81)
			{
				// produce the output grid
				Grid_ retval;
				for (int jj = 0; jj < 81; ++jj)
					retval[Loc_(jj)] = test[jj].back();
				return retval;
			}
			// set up to test the next cell
			test.push_back(all_9);
		}
	}
}

void AddGroup
	(const Grid_& key,
	 const Group_& g1,
	 const Group_* g2,
	 Vector_<Vector_<int> >* exclusions,
	 Vector_<RangeConstraint_>* ranges)
{
	// make a copy, we need to sort it
	Group_ g(g1);
	if (g2)
		g += *g2;
	Sort(&g.contents_);
	short sum = 0;
	for (auto p = g.begin(); p != g.end(); ++p)
		sum += key[*p];
	// now walk backwards through the group
	for (int iLeft = 0; iLeft < g.contents_.size(); ++iLeft)
	{
		const int ie = g.contents_.size() - iLeft - 1;
		auto pe = g.begin() + ie;
		// add exclusions
		Vector_<int>& exclude = (*exclusions)[pe->ICell()];
		for (auto qe = g.begin(); qe != pe; ++qe)
			exclude.push_back(qe->ICell());
		// refer back to a prior partial sum
		RangeConstraint_& limit = (*ranges)[pe->ICell()];
		if (pe != g.begin())
			limit.sofar_ = Previous(pe)->ICell();
		limit.lo_ = short(sum - 9 * iLeft + (iLeft * (iLeft - 1)) / 2);
		limit.hi_ = short(sum - 1 * iLeft - (iLeft * (iLeft - 1)) / 2);
	}
}

Grid_ LastSolution
	(const Grid_& key,
	 const Vector_<Group_>& groups,
	 const Group_* g1,
	 const Group_* g2,	// two groups, within the input groups, to test the merge of
	 const Vector_<short>& all_9)	// defines the lexicographical order
{
	assert(!g1 == !g2);	// g1/g2 should both be supplied or neither
	// set up basic exclusions
	Vector_<Vector_<int> > exclusions(81);
	for (int ar = 0; ar < 3; ++ar)
	{
		for (int br = 0; br < 3; ++br)
		{
			const int ir = ar * 3 + br;
			for (int ac = 0; ac < 3; ++ac)
			{
				const int ie = ar * 3 + ac;
				for (int bc = 0; bc < 3; ++bc)
				{
					const int ic = ac * 3 + bc;
					Vector_<int>& exclude = exclusions[ir * 9 + ic];
					// now loop over prior locations
					for (int jr = 0; jr < ir; ++jr)
						exclude.push_back(jr * 9 + ic);
					for (int jc = 0; jc < ic; ++jc)
						exclude.push_back(ir * 9 + jc);
					// including region
					const int base = ar * 27 + ac * 3;
					for (int ja = 0; ja <= br; ++ja)
					{
						const int stop = (ja == br) ? bc : 3;
						for (int jb = 0; jb < stop; ++jb)
							exclude.push_back(base + 9 * ja + jb);
					}

					exclude = Unique(exclude);
				}
			}
		}
	}
	// go through groups, creating exclusions and sums
	Vector_<RangeConstraint_> ranges(81);
	for (auto pg = groups.begin(); pg != groups.end(); ++pg)
	{
		if (&*pg != g1 && &*pg != g2)
			AddGroup(key, *pg, 0, &exclusions, &ranges);
	}
	if (g1)
		AddGroup(key, *g1, g2, &exclusions, &ranges);

	return LastSolution(exclusions, ranges, all_9);
}

//----------------------------------------------------------------------------

int Combinations(int n, int sum, int max = 9)
{
	if (n < 0 || n > max || sum < 0)
		return 0;
	if (2 * n > max)
		return Combinations(max - n, (max * (max + 1)) / 2 - sum);
	if (2 * sum > (max + 1) * n)
		return Combinations(n, (max + 1) * n - sum);
	if (n == 0)
		return sum == 0 ? 1 : 0;
	if (n == 1)
		return (sum > 0 && sum <= max) ? 1 : 0;
	return Combinations(n - 1, sum - n, max - 1) + Combinations(n, sum - n, max - 1);
}

double Entropy(int n, int sum)
{
	static const double LGE = 1.0 / log(2.0);
	int np = Combinations(n, sum);
	while (n > 0)
		np *= n--;
	return log(double(np)) * LGE;
}

int Sum(const Group_& group, const Grid_& grid)
{
	return accumulate(group.begin(), group.end(), 0, [&](int sofar, Loc_ loc) { return sofar + grid[loc]; });
}
double Entropy(const Group_& group, const Grid_& grid)
{
	return Entropy(group.contents_.size(), Sum(group, grid));
}

double Entropy(const Vector_<Group_>& groups, const Grid_& grid)
{
	return accumulate(
			groups.begin(), groups.end(), 0.0, [&](double sofar, const Group_& group) { return sofar + Entropy(group, grid); });
}

Vector_<Group_> UnitGroups()
{
	return Apply([](int ii) { return Group_({ Loc_(ii) }); }, Vector::UpTo(81));
}

bool Adjacent(const Loc_& l1, const Loc_& l2)
{
	return abs(l1.row_ - l2.row_) + abs(l1.col_ - l2.col_) == 1;
}

struct PairSet_
{
	std::map<Loc_, std::set<Loc_> > vals_;
	void Add(const Loc_& a, const Loc_& b) { vals_[a].insert(b); }
	bool operator()(const Loc_& a, const Loc_& b) const
	{
		return (vals_.count(a) && vals_.find(a)->second.count(b))
			|| (vals_.count(b) && vals_.find(b)->second.count(a));
	}
	void operator+=(const PairSet_& rhs)
	{
		for (auto pa = rhs.vals_.begin(); pa != rhs.vals_.end(); ++pa)
			for (auto pb = pa->second.begin(); pb != pa->second.end(); ++pb)
				Add(pa->first, *pb);
	}
};

bool Allowed(const Vector_<Group_>& groups, const PairSet_& forbidden)
{
	for (auto pg = groups.begin(); pg != groups.end(); ++pg)
		for (auto pe = pg->begin(); pe != pg->end(); ++pe)
			for (auto qe = Next(pe); qe != pg->end(); ++qe)
				if (forbidden(*pe, *qe) || forbidden(*qe, *pe))
					return false;
	return true;
}

bool CanMerge(const Vector_<Group_>& all_groups, const Group_& g1, const Group_& g2, const Grid_& grid, const PairSet_& forbidden)
{
	bool adjacent = false;
	for (auto p1 = g1.begin(); p1 != g1.end(); ++p1)
	{
		for (auto p2 = g2.begin(); p2 != g2.end(); ++p2)
		{
			if (grid[*p1] == grid[*p2])
				return false;
			if (forbidden(*p1, *p2) || forbidden(*p2, *p1))
				return false;
			adjacent = adjacent || Adjacent(*p1, *p2);
		}
	}
	// passed the fast tests -- now check solubility
	return adjacent
		&& LastSolution(grid, all_groups, &g1, &g2, All9()) == grid
		&& LastSolution(grid, all_groups, &g1, &g2, Reverse9()) == grid;
}

PairSet_ ForbidMore(const Group_& g1, const Group_& g2, const Grid_& grid)
{
	PairSet_ retval;
	// prevent one failure mode we know about
	for (auto p1 = g1.begin(); p1 != g1.end(); ++p1)
	{
		for (auto p2 = g2.begin(); p2 != g2.end(); ++p2)
		{
			if (p1->row_ == p2->row_)
			{
				const short c1 = p1->col_, c2 = p2->col_;
				for (short ir = 0; ir < 9; ++ir)
				{
					if (grid.vals_(ir, c1) == grid[*p2] && grid.vals_(ir, c2) == grid[*p1])
						retval.Add(Loc_(ir, c1), Loc_(ir, c2));
					const short jr = 3 * (ir / 3) + 1;
					if (jr != ir)
					{
						if (grid.vals_(ir, c1) == grid[*p2] && grid.vals_(jr, c2) == grid[*p1])
							retval.Add(Loc_(ir, c1), Loc_(jr, c2));
						if (grid.vals_(jr, c1) == grid[*p2] && grid.vals_(ir, c2) == grid[*p1])
							retval.Add(Loc_(jr, c1), Loc_(ir, c2));
					}
				}
			}
			else if (p1->col_ == p2->col_)
			{
				const short r1 = p1->row_, r2 = p2->row_;
				for (short ic = 0; ic < 9; ++ic)
				{
					if (grid.vals_(r1, ic) == grid[*p2] && grid.vals_(r2, ic) == grid[*p1])
						retval.Add(Loc_(r1, ic), Loc_(r2, ic));
					const short jc = 3 * (ic / 3) + 1;
					if (jc != ic)
					{
						if (grid.vals_(r1, jc) == grid[*p2] && grid.vals_(r2, ic) == grid[*p1])
							retval.Add(Loc_(r1, jc), Loc_(r2, ic));
						if (grid.vals_(r1, ic) == grid[*p2] && grid.vals_(r2, jc) == grid[*p1])
							retval.Add(Loc_(r1, ic), Loc_(r2, jc));
					}
				}
			}
		}
	}
	return retval;
}

int Edginess(const Group_& group)	// a measure of the difficulty of finding adjacent groups
{
	bool t = false, b = false, r = false, l = false;
	for (auto pe = group.begin(); pe != group.end(); ++pe)
	{
		l = l || pe->row_ == 0;
		r = r || pe->row_ == 8;
		t = t || pe->col_ == 0;
		b = b || pe->col_ == 8;
	}
	return (t ? 1 : 0) + (b ? 1 : 0) + (r ? 1 : 0) + (l ? 1 : 0);
}

bool HasTwoSingletons(const Vector_<Group_>& groups)
{
	int ns = 0;
	for (auto pg = groups.begin(); pg != groups.end(); ++pg)
		if (pg->contents_.size() == 1)
			if (++ns > 1)
				return true;
	return false;
}

Group_& GetRandom(Vector_<Group_>& groups)
{
	const bool singleton = HasTwoSingletons(groups);
	for ( ; ; )
	{
		if (Group_& retval = groups[RandLessThan(groups.size())]; !singleton || retval.contents_.size() == 1)
			return retval;
	}
}

void MergeOne(Vector_<Group_>* groups, const Grid_& grid, PairSet_* forbidden, int& max_tries)
{
	Group_& g1 = GetRandom(*groups);
	const int tries = 10 * (1 + Edginess(g1)) / Square(g1.contents_.size()); // how earnestly to try to merge this group
	for (int ii = 0; ii < tries; ++ii)
	{
		Group_& g2 = (*groups)[RandLessThan(groups->size())];
		if (&g1 != &g2 && CanMerge(*groups, g1, g2, grid, *forbidden))
		{
			// execute the merge
			if (PairSet_ more = ForbidMore(g1, g2, grid); Allowed(*groups, more))	// doesn't conflict with anything we have already done
			{
				g1 += g2;
				std::swap(g2, (*groups).back());
				groups->pop_back();
				*forbidden += more;
				return;
			}
		}
		if (--max_tries == 0)
			return;
	}
	// try again
	MergeOne(groups, grid, forbidden, max_tries);
}

Vector_<Group_> MakeGroups(const Grid_& grid, double entropy, int max_tries = 10000)
{
	Vector_<Group_> retval = UnitGroups();
	PairSet_ forbidden;
	for ( ; ; )
	{
		MergeOne(&retval, grid, &forbidden, max_tries);
		if (max_tries <= 0)
			return retval;	// fail, out of iterations
		if (Entropy(retval, grid) >= entropy)
			return retval;	// succeed
	}
}

// sort groups by comparing "first" cell -- this is not very meaningful but goodenuf
void Sort(Vector_<Group_>* groups)
{
	for (auto pg = groups->begin(); pg != groups->end(); ++pg)
		Sort(&pg->contents_);
	sort(groups->begin(), groups->end(), [](const Group_& lhs, const Group_& rhs) { return *lhs.begin() < *rhs.begin(); });
}

String_ ObscureSolution(const Grid_& grid)
{
	String_ retval;
	char offset = static_cast<char>('A' + RandLessThan(16));
	for (int ir = 0; ir < 9; ++ir)
	{
		if (ir > 0)
			retval.push_back('|');
		for (int ic = 0; ic < 9; ++ic)
			retval.push_back(char(offset + grid.vals_(ir, ic)));
	}
	return retval;
}

const Cell_& operator&(const Cell_& lhs, const Cell_& rhs)
{
	return lhs.type_ == Cell_::Type_::EMPTY ? lhs : rhs;
}

Matrix_<Cell_> KS(double entropy, int seed, bool minimal)
{
	srand(seed);
	Grid_ grid = RandomGrid();
	auto groups = MakeGroups(grid, entropy);
	Sort(&groups);
	Matrix_<String_> text(9, 9);
	Matrix_<char> group(9, 9);
	String_ gid("A");
	for (auto pg = groups.begin(); pg != groups.end(); ++pg, ++gid[0])
	{
		for (auto pe = pg->begin(); pe != pg->end(); ++pe)
		{
			group(pe->row_, pe->col_) = gid[0];
			if (pe == pg->begin())	// only write the sum once
			{
				String_ s = String::FromInt(Sum(*pg, grid));
				text(pe->row_, pe->col_) = minimal ? s : gid + ':' + s + '/' + String::FromInt(pg->contents_.size());
			}
			else if (!minimal)
				text(pe->row_, pe->col_) = gid;
		}
		if (gid[0] == 'Z')
			gid[0] -= 26;
	}
	// insert spacers
	static const double HORIZONTAL = 1000.0;
	static const double VERTICAL = 1000.0;
	Matrix_<Cell_> retval(28, 27);
	for (int ii = 0; ii < 9; ++ii)
	{
		for (int jj = 0; jj < 9; ++jj)
		{
			retval(3 * ii + 1, 3 * jj + 1) = text(ii, jj);
			const char g = group(ii, jj);
			if (jj == 0 || group(ii, jj - 1) != g)
				retval(3 * ii + 1, 3 * jj) = VERTICAL;
			if (jj == 8 || group(ii, jj + 1) != g)
				retval(3 * ii + 1, 3 * jj + 2) = VERTICAL;
			if (ii == 0 || group(ii - 1, jj) != g)
				retval(3 * ii, 3 * jj + 1) = HORIZONTAL;
			if (ii == 8 || group(ii + 1, jj) != g)
				retval(3 * ii + 2, 3 * jj + 1) = HORIZONTAL;
		}
	}
	// second pass to get the corners
	for (int ii = 0; ii < 9; ++ii)
	{
		for (int jj = 0; jj < 9; ++jj)
		{
			const char g = group(ii, jj);
			if (jj > 0 && group(ii, jj - 1) == g)
			{
				retval(3 * ii, 3 * jj) = retval(3 * ii, 3 * jj + 1) & retval(3 * ii, 3 * jj - 2);
				retval(3 * ii + 2, 3 * jj) = retval(3 * ii + 2, 3 * jj + 1) & retval(3 * ii + 2, 3 * jj - 2);
			}
			if (jj < 8 && group(ii, jj + 1) == g)
			{
				retval(3 * ii, 3 * jj + 2) = retval(3 * ii, 3 * jj + 1) & retval(3 * ii, 3 * jj + 4);
				retval(3 * ii + 2, 3 * jj + 2) = retval(3 * ii + 2, 3 * jj + 1) & retval(3 * ii + 2, 3 * jj + 4);
			}
			if (ii > 0 && group(ii - 1, jj) == g)
			{
				retval(3 * ii, 3 * jj) = retval(3 * ii + 1, 3 * jj) & retval(3 * ii - 2, 3 * jj);
				retval(3 * ii, 3 * jj + 2) = retval(3 * ii + 1, 3 * jj + 2) & retval(3 * ii - 2, 3 * jj + 2);
			}
			if (ii < 8 && group(ii + 1, jj) == g)
			{
				retval(3 * ii + 2, 3 * jj) = retval(3 * ii + 1, 3 * jj) & retval(3 * ii + 4, 3 * jj);
				retval(3 * ii + 2, 3 * jj + 2) = retval(3 * ii + 1, 3 * jj + 2) & retval(3 * ii + 4, 3 * jj + 2);
			}
		}
	}


	// append solution
	retval(27, 0) = ObscureSolution(grid);
	retval(27, 1) = seed;
	retval(27, 4) = entropy;
	retval(27, 7) = Entropy(groups, grid);
	return retval;
}

