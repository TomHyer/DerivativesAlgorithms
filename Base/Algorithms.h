
// container-level algorithms, mostly based on STL iterator algorithms

#pragma once

#include <algorithm>
#include <numeric>
#include <type_traits>

template<class T_> T_ Next(const T_& src) { T_ retval(src);  return ++retval; }
template<class T_> T_ Previous(const T_& src) { T_ retval(src);  return --retval; }
#define ASSIGN(p, v) if (!p); else *p = v
#define DEREFERENCE(p, v) (p ? *p : v)

template<class CS_, class CD_> void Copy(const CS_& src, CD_* dst)
{
	assert(dst && src.size() == dst->size());
	std::copy(src.begin(), src.end(), dst->begin());
}

template<template<class> class V_ = Vector_, class C_> auto Copy(const C_& src)
{
	typedef V_<VALUE_TYPE_OF(*src.begin())> vector_t;
	vector_t retval(src.size());
	std::copy(src.begin(), src.end(), retval.begin());
	return retval;
}

template<class CS_, class OP_, class CD_> void Transform(const CS_& src, OP_ op, CD_* dst)
{
	assert(dst && src.size() == dst->size());
	std::transform(src.begin(), src.end(), dst->begin(), op);
}

template<class CS1_, class CS2_, class OP_, class CD_> void Transform(const CS1_& src1, const CS2_& src2, OP_ op, CD_* dst)
{
	assert(dst && src1.size() == dst->size() && src2.size() == dst->size());
	std::transform(src1.begin(), src1.end(), src2.begin(), dst->begin(), op);
}

template<class C_, class OP_> void Transform(C_* to_change, OP_ op)
{
	assert(to_change != nullptr);
	std::transform(to_change->begin(), to_change->end(), to_change->begin(), op);
}

template<class C_, class CI_, class OP_> void Transform(C_* to_change, const CI_& other, OP_ op)
{
	assert(to_change != nullptr);
	std::transform(to_change->begin(), to_change->end(), other.begin(), to_change->begin(), op);
}

template<class C_, class E_> void Fill(C_* range, const E_& val)
{
	std::fill(range->begin(), range->end(), val);
}

template<template<class> class V_ = Vector_, typename C_, typename OP_> auto Apply(OP_ op, const C_& src)
{
	V_<decltype(op(*src.begin()))> retval(src.size());
	Transform(src, op, &retval);
	return retval;
}

template<template<class> class V_ = Vector_, class C1_, class C2_, class OP_> auto Apply(OP_ op, const C1_& src1, const C2_& src2)
{
	V_<decltype(op(*src1.begin(), *src2.begin()))> retval(src1.size());
	Transform(src1, src2, op, &retval);
	return retval;
}

template<class C1_, class C2_> void Append(C1_* c1, const C2_& c2)
{
	c1->insert(c1->end(), c2.begin(), c2.end());
}
// specialization for Vector_ which does not expose insert
template<class E1_, class C2_> void Append(Vector_<E1_>* c1, const C2_& c2)
{
	c1->Append(c2);
}

template<class C1_, class C2_> C1_ Join(const C1_& c1, const C2_& c2)
{
	C1_ retval = c1;
	Append(&retval, c2);
	return retval;
}

template<class C_, class P_> void Filter(C_* c, const P_& pred)
{
	typedef decltype(*c->begin()) element_t;
	auto stop = std::remove_if(c->begin(), c->end(), [&](const element_t& e) {return !pred(e); });
	c->erase(stop, c->end());
}
template<class C_, class P_> C_ Filter(const C_& src, const P_& pred)
{
	C_ retval(src);
	Filter(&retval);
	return retval;
}

template<class C_, class LT_> C_ Unique(const C_& src, const LT_& less)
{
   C_ retval(src);
   std::sort(retval.begin(), retval.end(), less);
   retval.erase(unique(retval.begin(), retval.end()), retval.end());
   return retval;
}
template<class C_> C_ Unique(const C_& src)
{
   return Unique(src, std::less<decltype(*src.begin())>());
}

template<class C_, class E_> typename C_::const_iterator LowerBound(const C_& c, const E_& x)
{
   return std::lower_bound(c.begin(), c.end(), x);
}
template<class C_, class E_> typename C_::const_iterator UpperBound(const C_& c, const E_& x)
{
   return std::upper_bound(c.begin(), c.end(), x);
}

template<class C_, class E_> bool BinarySearch(const C_& c, const E_& val)
{
	return std::binary_search(c.begin(), c.end(), val);
}
template<class C_, class E_> bool LinearSearch(const C_& c, const E_& val)
{
	return std::find(c.begin(), c.end(), val) != c.end();
}

template<class C_, class L_> bool IsMonotonic(const C_& c, const L_& pred)
{
	if (c.size() < 2)
		return true;
	auto pvs = c.begin();
	for (auto nxt = Next(pvs); nxt != c.end(); pvs = nxt++)
	{
		if (!pred(*pvs, *nxt))
			return false;
	}
	return true;
}
template<class C_> bool IsMonotonic(const C_& c)
{
	return IsMonotonic(c, std::less<decltype(*c.begin())>());
}

// zipping
template<class C1_, class C2_> auto Zip(const C1_& v1, const C2_& v2)
{
   auto join = [](const auto& e1, const auto& e2) {return make_pair(e1, e2); };
   return Apply(join, v1, v2);
}

// unzipping
template<class P_> typename P_::first_type GetFirst(const P_& pair) { return pair.first; }
template<class P_> typename P_::second_type GetSecond(const P_& pair) { return pair.second; }

template<template<class> class V_ = Vector_, class C_> auto Keys(const C_& src)
{
	auto op = [](const auto& kv) { return kv.first; };
	return Apply<V_>(op, src);
}

template<template<class> class V_ = Vector_, class C_> auto MapValues(const C_& src)
{
	auto op = [](auto kv) { return kv.second; };
	V_<typename C_::mapped_type> retval(src.size());
	Transform(src, op, &retval);
	return retval;
}

template<class C_> auto MaxElement(const C_& src)
{
	return std::max_element(src.begin(), src.end());
}
template<class C_> auto MinElement(const C_& src)
{
	return std::min_element(src.begin(), src.end());
}

template<class C_> void Sort(C_* vals)
{
	std::sort(vals->begin(), vals->end());
}
template<class C_, class P_> void Sort(C_* vals, const P_& pred)
{
	std::sort(vals->begin(), vals->end(), pred);
}

template<class C_, class P_> bool CheckAll(const C_& vals, const P_& pred)
{
   for (auto& e : vals)
      if (!pred(e))
         return false;
   return true;
}

template <class C_, class P_> bool CheckAny(const C_& vals, const P_& pred)
{
    for (auto& e : vals)
        if (pred(e))
            return true;
    return false;
}
