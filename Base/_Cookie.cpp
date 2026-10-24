
#include "Platform.h"
#include "_Cookie.h"
#include <map>
#include <variant>
#include "Strict.h"

#include "Exceptions.h"
#include "Environment.h"
#include "_Repository.h"
#include "Date.h"
#include "DateTimeUtils.h"
#include "Numerics.h"
#include <algorithm>
#include <cstring>

#ifdef THREADED_FFI
#include <mutex>
static std::mutex TheObjectsMutex;
#define LOCK_OBJECTS std::lock_guard<std::mutex> l(TheObjectsMutex)
#else
#define LOCK_OBJECTS
#endif

void HandleDelete(void* ph)
{
	if (ph)
		delete reinterpret_cast<Handle_<Storable_>*>(ph);
}

void CookieFreeString(char* p)
{
	free(p);
}

namespace
{
	struct Stashable_
	{
		using variant_t = std::variant<
			Handle_<Storable_>,
			Handle_<Vector_<double>>,
			Handle_<Vector_<int>>,
			Handle_<Vector_<String_>>,
			Handle_<Vector_<Cell_>>,
			Handle_<Vector_<Handle_<Storable_>>>,
			Handle_<Matrix_<double>>,
			Handle_<Matrix_<int>>,
			Handle_<Matrix_<String_>>,
			Handle_<Matrix_<Cell_>>,
			Handle_<String_>,
			Handle_<Dictionary_>,
			double,
			int,
			bool>;
		std::shared_ptr<variant_t> val_;

		Stashable_() {}
		template<class T_> Stashable_(T_&& v) : val_(std::make_shared<variant_t>(std::forward<T_>(v))) {}
	};

	std::map<int, Stashable_>& TheStash()
	{
		RETURN_STATIC(std::map<int, Stashable_>);
	}

	int Unique()
	{
		static int RETVAL = 0;
		return ++RETVAL;
	}

	template<class T_> int ToCookie(T_&& stashable_value)
	{
		LOCK_OBJECTS;
		const int retval = Unique();
		TheStash()[retval] = Stashable_(std::forward<T_>(stashable_value));
		return retval;
	}

	template<class T_> int CopyToCookie(const T_& val)
	{
		return ToCookie(Handle_<T_>(new T_(val)));
	}

	const Stashable_& StashFind(int cookie)
	{
		static const Stashable_ EMPTY;
		LOCK_OBJECTS;
		if (auto pc = TheStash().find(cookie); pc != TheStash().end())
			return pc->second;
		return EMPTY;
	}
	void StashDrop(int cookie)
	{
		LOCK_OBJECTS;
		TheStash().erase(cookie);
	}

	struct Biscuit_
	{
		Vector_<int> cookies_;
		String_ error_;
	};
	std::map<double, Biscuit_>& TheBiscuits()
	{
		RETURN_STATIC(std::map<double, Biscuit_>);
	}

	template<class T_> const T_& Require(int cookie, bool optional)
	{
		static const T_ EMPTY{};
		if (cookie == 0)
		{
			REQUIRE(optional, "Missing input");
			return EMPTY;
		}
		const Stashable_& stored = StashFind(cookie);
		REQUIRE(stored.val_, "No such cookie");
		const T_* p = std::get_if<T_>(stored.val_.get());
		REQUIRE(p, "Input has the wrong cookie type");
		return *p;
	}

	Cell_ DecodeCell(int type, double num, const char* str)
	{
		switch (type)
		{
		case 1: return Cell_(num != 0.0);
		case 2: return Cell_(num);
		case 3: return Cell_(str ? String_(str) : String_());
		case 4: return Cell_(Date::FromExcel(AsInt(num)));
		case 5: return Cell_(DateTime::FromExcel(num));
		default: return Cell_();
		}
	}

	void EncodeCell(const Cell_& c, int* type, double* num, char** str)
	{
		*type = 0;
		*num = 0.0;
		*str = nullptr;
		c.Visit([&](auto const& v)
		{
			using V = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<V, bool>)
			{
				*type = 1;
				*num = v ? 1.0 : 0.0;
			}
			else if constexpr (std::is_same_v<V, double>)
			{
				*type = 2;
				*num = v;
			}
			else if constexpr (std::is_same_v<V, String_>)
			{
				*type = 3;
				*str = v.empty() ? nullptr : _strdup(v.c_str());
			}
			else if constexpr (std::is_same_v<V, Date_>)
			{
				*type = 4;
				*num = Date::ToExcel(v);
			}
			else if constexpr (std::is_same_v<V, DateTime_>)
			{
				*type = 5;
				*num = Date::ToExcel(v.Date()) + v.Frac();
			}
		});
	}
}	// leave local


Cookie::Output_::Output_()
{
	static const double OFFSET = (1 << 17) * 1.0 * (1 << 17);
	id_ = OFFSET + Unique();
}

Cookie::Output_::~Output_()
{
	LOCK_OBJECTS;
	for (auto c : cookies_)
		TheStash().erase(c);
}

double Cookie::Output_::Finalize()
{
	LOCK_OBJECTS;
	TheBiscuits()[id_].cookies_.Swap(&cookies_);
	TheBiscuits()[id_].error_.clear();
	return id_;
}

double Cookie::Output_::Error(const char* what, const char* arg)
{
	String_ s(what);
	if (arg)
		s += " (at input '" + String_(arg) + "')";

	LOCK_OBJECTS;
	TheBiscuits()[id_].cookies_.clear();
	TheBiscuits()[id_].error_.Swap(&s);
	return id_;
}

void Cookie::Output_::AppendBase(const Handle_<Storable_>& h) { cookies_.push_back(ToCookie(h)); }
void Cookie::Output_::Append(double v) { cookies_.push_back(ToCookie(v)); }
void Cookie::Output_::Append(int v) { cookies_.push_back(ToCookie(v)); }
void Cookie::Output_::Append(bool v) { cookies_.push_back(ToCookie(v)); }
void Cookie::Output_::Append(const String_& s) { cookies_.push_back(CopyToCookie(s)); }
void Cookie::Output_::Append(const Date_& d) { cookies_.push_back(ToCookie(double(Date::ToExcel(d)))); }
void Cookie::Output_::Append(const DateTime_& d) { cookies_.push_back(ToCookie(Date::ToExcel(d.Date()) + d.Frac())); }
void Cookie::Output_::Append(const Cell_& c)
{
	Matrix_<Cell_> m(1, 1);
	m(0, 0) = c;
	Append(m);
}
void Cookie::Output_::Append(const Vector_<>& v) { cookies_.push_back(CopyToCookie(v)); }
void Cookie::Output_::Append(const Vector_<int>& v) { cookies_.push_back(CopyToCookie(v)); }
void Cookie::Output_::Append(const Vector_<String_>& v) { cookies_.push_back(CopyToCookie(v)); }
void Cookie::Output_::Append(const Vector_<Cell_>& v) { cookies_.push_back(CopyToCookie(v)); }
void Cookie::Output_::Append(const Vector_<Handle_<Storable_>>& v) { cookies_.push_back(CopyToCookie(v)); }
void Cookie::Output_::Append(const Matrix_<double>& m) { cookies_.push_back(CopyToCookie(m)); }
void Cookie::Output_::Append(const Matrix_<int>& m) { cookies_.push_back(CopyToCookie(m)); }
void Cookie::Output_::Append(const Matrix_<String_>& m) { cookies_.push_back(CopyToCookie(m)); }
void Cookie::Output_::Append(const Matrix_<Cell_>& m) { cookies_.push_back(CopyToCookie(m)); }

Cookie::Input_::~Input_()
{
	for (auto c : used_)
		StashDrop(c);
}

namespace
{
	void Use(Cookie::Input_* self, int cookie)
	{
		if (cookie)
			self->used_.push_back(cookie);
	}
}

const Handle_<Storable_>& Cookie::Input_::HandleBase(int cookie, bool optional)
{
	Use(this, cookie);
	static const Handle_<Storable_> EMPTY;
	if (cookie == 0)
	{
		REQUIRE(optional, "Missing input handle");
		return EMPTY;
	}
	return Require<Handle_<Storable_>>(cookie, optional);
}

const Vector_<>& Cookie::Input_::VectorDouble(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Vector_<double>>>(cookie, optional);
	static const Vector_<> EMPTY;
	return h ? *h : EMPTY;
}
const Vector_<int>& Cookie::Input_::VectorInt(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Vector_<int>>>(cookie, optional);
	static const Vector_<int> EMPTY;
	return h ? *h : EMPTY;
}
const Vector_<String_>& Cookie::Input_::VectorString(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Vector_<String_>>>(cookie, optional);
	static const Vector_<String_> EMPTY;
	return h ? *h : EMPTY;
}
const Vector_<Cell_>& Cookie::Input_::VectorCell(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Vector_<Cell_>>>(cookie, optional);
	static const Vector_<Cell_> EMPTY;
	return h ? *h : EMPTY;
}
const Vector_<Handle_<Storable_>>& Cookie::Input_::VectorHandleBase(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Vector_<Handle_<Storable_>>>>(cookie, optional);
	static const Vector_<Handle_<Storable_>> EMPTY;
	return h ? *h : EMPTY;
}
const Matrix_<double>& Cookie::Input_::MatrixDouble(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Matrix_<double>>>(cookie, optional);
	static const Matrix_<double> EMPTY;
	return h ? *h : EMPTY;
}
const Matrix_<int>& Cookie::Input_::MatrixInt(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Matrix_<int>>>(cookie, optional);
	static const Matrix_<int> EMPTY;
	return h ? *h : EMPTY;
}
const Matrix_<String_>& Cookie::Input_::MatrixString(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Matrix_<String_>>>(cookie, optional);
	static const Matrix_<String_> EMPTY;
	return h ? *h : EMPTY;
}
const Matrix_<Cell_>& Cookie::Input_::MatrixCell(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Matrix_<Cell_>>>(cookie, optional);
	static const Matrix_<Cell_> EMPTY;
	return h ? *h : EMPTY;
}
const Dictionary_& Cookie::Input_::Dict(int cookie, bool optional)
{
	Use(this, cookie);
	const auto& h = Require<Handle_<Dictionary_>>(cookie, optional);
	static const Dictionary_ EMPTY;
	return h ? *h : EMPTY;
}

int HandleCookie(void* ph)
{
	if (!ph)
		return 0;
	return ToCookie(*reinterpret_cast<Handle_<Storable_>*>(ph));
}

int HandleCookieFromTag(const char* tag)
{
	if (!tag || !*tag)
		return 0;
	ENV_SEED_TYPE(ObjectAccess_);
	auto repo = Environment::Find<ObjectAccess_>(_env);
	if (!repo)
		return 0;
	auto h = repo->Fetch(tag, true);
	if (h.Empty())
		return 0;
	return ToCookie(h);
}

int StringCookie(const char* s)
{
	return CopyToCookie(s ? String_(s) : String_());
}

int VectorDoubleCookie(const double* vals, int size)
{
	return CopyToCookie(Vector_<>(vals, vals + size));
}
int VectorIntCookie(const int* vals, int size)
{
	return CopyToCookie(Vector_<int>(vals, vals + size));
}
int VectorStringCookie(const char* const* vals, int size)
{
	Vector_<String_> v(size);
	for (int i = 0; i < size; ++i)
		v[i] = vals[i] ? String_(vals[i]) : String_();
	return CopyToCookie(v);
}
int VectorCellCookie(int size, const int* types, const double* nums, const char* const* strs)
{
	Vector_<Cell_> v(size);
	for (int i = 0; i < size; ++i)
		v[i] = DecodeCell(types[i], nums[i], strs ? strs[i] : nullptr);
	return CopyToCookie(v);
}
int VectorHandleCookie(const int* handle_cookies, int size)
{
	Vector_<Handle_<Storable_>> v(size);
	for (int i = 0; i < size; ++i)
	{
		if (!handle_cookies[i])
			continue;
		v[i] = Require<Handle_<Storable_>>(handle_cookies[i], false);
		StashDrop(handle_cookies[i]);
	}
	return CopyToCookie(v);
}

namespace
{
	template<class E_> int MatrixFromFlat(const E_* vals, int rows, int cols)
	{
		Matrix_<E_> m(rows, cols);
		for (int r = 0; r < rows; ++r)
			for (int c = 0; c < cols; ++c)
				m(r, c) = vals[r * cols + c];
		return CopyToCookie(m);
	}
}

int MatrixDoubleCookie(const double* vals, int rows, int cols) { return MatrixFromFlat(vals, rows, cols); }
int MatrixIntCookie(const int* vals, int rows, int cols) { return MatrixFromFlat(vals, rows, cols); }
int MatrixStringCookie(const char* const* vals, int rows, int cols)
{
	Matrix_<String_> m(rows, cols);
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols; ++c)
		{
			auto s = vals[r * cols + c];
			m(r, c) = s ? String_(s) : String_();
		}
	return CopyToCookie(m);
}
int MatrixCellCookie(int rows, int cols, const int* types, const double* nums, const char* const* strs)
{
	Matrix_<Cell_> m(rows, cols);
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols; ++c)
		{
			const int i = r * cols + c;
			m(r, c) = DecodeCell(types[i], nums[i], strs ? strs[i] : nullptr);
		}
	return CopyToCookie(m);
}

int BiscuitCount(double id)
{
	LOCK_OBJECTS;
	if (auto pb = TheBiscuits().find(id); pb != TheBiscuits().end())
		return pb->second.error_.empty() ? pb->second.cookies_.size() : -2;
	return -1;
}

int BiscuitCookie(double id, int which)
{
	if (which < 0)
		return which;
	LOCK_OBJECTS;
	if (auto pb = TheBiscuits().find(id); pb != TheBiscuits().end())
	{
		if (!pb->second.error_.empty())
			return -2;
		if (which >= pb->second.cookies_.size())
			return -3;
		int retval = pb->second.cookies_[which];
		if (which + 1 == pb->second.cookies_.size())
			TheBiscuits().erase(pb);
		return retval;
	}
	return -1;
}

char* BiscuitError(double id)
{
	LOCK_OBJECTS;
	if (auto pb = TheBiscuits().find(id); pb != TheBiscuits().end() && !pb->second.error_.empty())
	{
		char* retval = _strdup(pb->second.error_.c_str());
		TheBiscuits().erase(pb);
		return retval;
	}
	return nullptr;
}

void CookieRelease(int cookie)
{
	StashDrop(cookie);
}

void* CookieHandle(int cookie)
{
	Handle_<Storable_> handle = Require<Handle_<Storable_>>(cookie, true);
	StashDrop(cookie);
	return handle.Empty() ? nullptr : new Handle_<Storable_>(handle);
}

const char* CookieHandleTag(int cookie)
{
	const Handle_<Storable_>& handle = Require<Handle_<Storable_>>(cookie, true);
	if (handle.Empty())
		return "";
	return handle->name_.c_str();
}

int CookieInt(int cookie)
{
	int v = Require<int>(cookie, false);
	StashDrop(cookie);
	return v;
}
void DoRequire(bool cond, const char* msg)
{
	REQUIRE(cond, msg);
}
double CookieDouble(int cookie)
{
	const Stashable_& stored = StashFind(cookie);
	DoRequire(!!stored.val_, "No such cookie");
	double v = 0.0;
	if (auto p = std::get_if<double>(stored.val_.get()))
		v = *p;
	else if (auto pi = std::get_if<int>(stored.val_.get()))
		v = *pi;
	else
		DoRequire(false, "Input is not numeric");
	StashDrop(cookie);
	return v;
}
int CookieBool(int cookie)
{
	int v = Require<bool>(cookie, false) ? 1 : 0;
	StashDrop(cookie);
	return v;
}
const char* CookieStringBase(int cookie)
{
	const auto& h = Require<Handle_<String_>>(cookie, true);
	return h ? h->c_str() : "";
}
const char* CookieStringCopy(int cookie)
{
	const auto& h = Require<Handle_<String_>>(cookie, true);
	char* p = _strdup(h ? h->c_str() : "");
	StashDrop(cookie);
	return p;
}

namespace
{
	int HandleSize(const Stashable_& stored)
	{
		if (!stored.val_)
			return 0;
		return std::visit([](const auto& v) -> int
		{
			using V = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<V, Handle_<Vector_<double>>> ||
				std::is_same_v<V, Handle_<Vector_<int>>> ||
				std::is_same_v<V, Handle_<Vector_<String_>>> ||
				std::is_same_v<V, Handle_<Vector_<Cell_>>> ||
				std::is_same_v<V, Handle_<Vector_<Handle_<Storable_>>>>)
				return v ? v->size() : 0;
			return -1;
		}, *stored.val_);
	}
}

int CookieVectorSize(int cookie)
{
	return HandleSize(StashFind(cookie));
}

int CookieMatrixRows(int cookie)
{
	const Stashable_& stored = StashFind(cookie);
	if (!stored.val_)
		return 0;
	return std::visit([](const auto& v) -> int
	{
		using V = std::decay_t<decltype(v)>;
		if constexpr (std::is_same_v<V, Handle_<Matrix_<double>>> ||
			std::is_same_v<V, Handle_<Matrix_<int>>> ||
			std::is_same_v<V, Handle_<Matrix_<String_>>> ||
			std::is_same_v<V, Handle_<Matrix_<Cell_>>>)
			return v ? v->Rows() : 0;
		return 0;
	}, *stored.val_);
}
int CookieMatrixCols(int cookie)
{
	const Stashable_& stored = StashFind(cookie);
	if (!stored.val_)
		return 0;
	return std::visit([](const auto& v) -> int
	{
		using V = std::decay_t<decltype(v)>;
		if constexpr (std::is_same_v<V, Handle_<Matrix_<double>>> ||
			std::is_same_v<V, Handle_<Matrix_<int>>> ||
			std::is_same_v<V, Handle_<Matrix_<String_>>> ||
			std::is_same_v<V, Handle_<Matrix_<Cell_>>>)
			return v ? v->Cols() : 0;
		return 0;
	}, *stored.val_);
}

const double* CookieVectorDoubleBase(int cookie)
{
	const auto& h = Require<Handle_<Vector_<double>>>(cookie, true);
	return (h && !h->empty()) ? &(*h)[0] : nullptr;
}
void CookieVectorDoubleCopyVals(int cookie, double* dst)
{
	const auto& h = Require<Handle_<Vector_<double>>>(cookie, true);
	if (h)
		std::copy(h->begin(), h->end(), dst);
	StashDrop(cookie);
}
const int* CookieVectorIntBase(int cookie)
{
	const auto& h = Require<Handle_<Vector_<int>>>(cookie, true);
	return (h && !h->empty()) ? &(*h)[0] : nullptr;
}
void CookieVectorIntCopyVals(int cookie, int* dst)
{
	const auto& h = Require<Handle_<Vector_<int>>>(cookie, true);
	if (h)
		std::copy(h->begin(), h->end(), dst);
	StashDrop(cookie);
}
void CookieVectorStringCopyVals(int cookie, char** dst)
{
	const auto& h = Require<Handle_<Vector_<String_>>>(cookie, true);
	if (h)
		for (int i = 0; i < h->size(); ++i)
			dst[i] = _strdup((*h)[i].c_str());
	StashDrop(cookie);
}
void CookieVectorCellCopy(int cookie, int* types, double* nums, char** strs)
{
	const auto& h = Require<Handle_<Vector_<Cell_>>>(cookie, true);
	if (h)
		for (int i = 0; i < h->size(); ++i)
			EncodeCell((*h)[i], types + i, nums + i, strs + i);
	StashDrop(cookie);
}
void CookieVectorHandleCopyVals(int cookie, void** dst)
{
	const auto& h = Require<Handle_<Vector_<Handle_<Storable_>>>>(cookie, true);
	if (h)
		for (int i = 0; i < h->size(); ++i)
			dst[i] = (*h)[i] ? new Handle_<Storable_>((*h)[i]) : nullptr;
	StashDrop(cookie);
}

void CookieMatrixDoubleCopyVals(int cookie, double* dst)
{
	const auto& h = Require<Handle_<Matrix_<double>>>(cookie, true);
	if (h)
		for (int r = 0; r < h->Rows(); ++r)
			for (int c = 0; c < h->Cols(); ++c)
				dst[r * h->Cols() + c] = (*h)(r, c);
	StashDrop(cookie);
}
void CookieMatrixIntCopyVals(int cookie, int* dst)
{
	const auto& h = Require<Handle_<Matrix_<int>>>(cookie, true);
	if (h)
		for (int r = 0; r < h->Rows(); ++r)
			for (int c = 0; c < h->Cols(); ++c)
				dst[r * h->Cols() + c] = (*h)(r, c);
	StashDrop(cookie);
}
void CookieMatrixStringCopyVals(int cookie, char** dst)
{
	const auto& h = Require<Handle_<Matrix_<String_>>>(cookie, true);
	if (h)
		for (int r = 0; r < h->Rows(); ++r)
			for (int c = 0; c < h->Cols(); ++c)
				dst[r * h->Cols() + c] = _strdup((*h)(r, c).c_str());
	StashDrop(cookie);
}
void CookieMatrixCellCopy(int cookie, int* types, double* nums, char** strs)
{
	const auto& h = Require<Handle_<Matrix_<Cell_>>>(cookie, true);
	if (h)
		for (int r = 0; r < h->Rows(); ++r)
			for (int c = 0; c < h->Cols(); ++c)
			{
				const int i = r * h->Cols() + c;
				EncodeCell((*h)(r, c), types + i, nums + i, strs + i);
			}
	StashDrop(cookie);
}
