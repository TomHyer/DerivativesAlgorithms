
#include "Platform.h"

#ifdef _DEBUG
#define DA_PYTHON_RESTORE_DEBUG
#undef _DEBUG
#endif
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <datetime.h>
#ifdef DA_PYTHON_RESTORE_DEBUG
#define _DEBUG
#endif

#if PY_MAJOR_VERSION < 3
#error The DA Python interface requires Python 3
#endif

#include "_Python.h"
#include "Strict.h"

#include "Exceptions.h"
#include "Numerics.h"
#include "Cell.h"
#include "Date.h"
#include "DateTime.h"
#include "Dictionary.h"
#include "Algorithms.h"
#include "_Repository.h"
#include <cstring>

namespace
{
	bool IsMissing(PyObject* src)
	{
		return !src || src == Py_None;
	}

	bool IsSeq(PyObject* src)
	{
		return src
			&& PySequence_Check(src)
			&& !PyUnicode_Check(src)
			&& !PyBytes_Check(src)
			&& !PyByteArray_Check(src);
	}

	void EnsureDateTime()
	{
		if (!PyDateTimeAPI)
			PyDateTime_IMPORT;
		REQUIRE(PyDateTimeAPI, "Failed to import Python datetime C API");
	}

	struct BorrowedItem_
	{
		PyObject* p_;
		PyObject* owned_;	// if we hold a new ref
		BorrowedItem_(PyObject* borrowed) : p_(borrowed), owned_(nullptr) {}
		BorrowedItem_(PyObject* newref, bool own) : p_(newref), owned_(own ? newref : nullptr) {}
		~BorrowedItem_() { Py_XDECREF(owned_); }
		BorrowedItem_(const BorrowedItem_&) = delete;
		PyObject* Get() const { return p_; }
	};

	PyObject* SeqGet(PyObject* seq, int i)	// new reference
	{
		PyObject* item = PySequence_GetItem(seq, i);
		if (!item)
		{
			PyErr_Clear();
			THROW("Failed to read sequence element");
		}
		return item;
	}

	int SeqSize(PyObject* seq)
	{
		const Py_ssize_t n = PySequence_Size(seq);
		if (n < 0)
		{
			PyErr_Clear();
			THROW("Failed to read sequence size");
		}
		REQUIRE(n <= 0x7fffffff, "Sequence is too large");
		return static_cast<int>(n);
	}

	String_ UnicodeToString(PyObject* src)
	{
		Py_ssize_t len = 0;
		const char* utf8 = PyUnicode_AsUTF8AndSize(src, &len);
		if (!utf8)
		{
			PyErr_Clear();
			THROW("Invalid input type -- string expected");
		}
		return String_(utf8, utf8 + len);
	}

	double XToNumber(PyObject* src)
	{
		if (PyBool_Check(src))
			THROW("Invalid input type -- number expected");
		if (PyFloat_Check(src))
			return PyFloat_AsDouble(src);
		if (PyLong_Check(src))
		{
			const double retval = PyLong_AsDouble(src);
			if (PyErr_Occurred())
			{
				PyErr_Clear();
				THROW("Integer input is too large for a number");
			}
			return retval;
		}
		THROW("Invalid input type -- number expected");
	}

	int XToInteger(PyObject* src)
	{
		if (PyBool_Check(src))
			THROW("Invalid input type -- integer expected");
		if (PyLong_Check(src))
		{
			const long retval = PyLong_AsLong(src);
			if (PyErr_Occurred())
			{
				PyErr_Clear();
				THROW("Integer input is out of range");
			}
			return static_cast<int>(retval);
		}
		if (PyFloat_Check(src))
		{
			const double d = PyFloat_AsDouble(src);
			const int retval = AsInt(d);
			REQUIRE(IsZero(d - retval), "Non-integer value where integer was expected");
			return retval;
		}
		THROW("Invalid input type -- integer expected");
	}

	bool XToBoolean(PyObject* src)
	{
		if (PyBool_Check(src))
			return src == Py_True;
		if (PyLong_Check(src))
		{
			const long v = PyLong_AsLong(src);
			REQUIRE(v == 0 || v == 1, "Can't convert a number (except 0 or 1) to boolean");
			return v != 0;
		}
		THROW("Invalid input type -- boolean expected");
	}

	std::uint64_t XToLong(PyObject* src)
	{
		if (PyBool_Check(src))
			THROW("Invalid input type -- long expected");
		if (PyLong_Check(src))
		{
			const unsigned long long retval = PyLong_AsUnsignedLongLong(src);
			if (PyErr_Occurred())
			{
				PyErr_Clear();
				THROW("Long integer input is out of range");
			}
			return static_cast<std::uint64_t>(retval);
		}
		THROW("Invalid input type -- long expected");
	}

	Date_ XToDate(PyObject* src)
	{
		EnsureDateTime();
		if (PyDateTime_Check(src) || PyDate_Check(src))
		{
			return Date_(PyDateTime_GET_YEAR(src), PyDateTime_GET_MONTH(src), PyDateTime_GET_DAY(src));
		}
		if (PyLong_Check(src) || PyFloat_Check(src))
		{
			const int serial = XToInteger(src);
			REQUIRE(serial >= Date::ToExcel(Date::Minimum()) && serial <= Date::ToExcel(Date::Maximum()), "Integer date outside legal range");
			return Date::FromExcel(serial);
		}
		THROW("Invalid input type -- date expected");
	}

	DateTime_ XToDateTime(PyObject* src)
	{
		EnsureDateTime();
		if (PyDateTime_Check(src))
		{
			const Date_ d(PyDateTime_GET_YEAR(src), PyDateTime_GET_MONTH(src), PyDateTime_GET_DAY(src));
			return DateTime_(d, PyDateTime_DATE_GET_HOUR(src), PyDateTime_DATE_GET_MINUTE(src), PyDateTime_DATE_GET_SECOND(src));
		}
		if (PyDate_Check(src))
		{
			return DateTime_(XToDate(src), 0.0);
		}
		if (PyLong_Check(src) || PyFloat_Check(src))
		{
			const double d = XToNumber(src);
			const int id = AsInt(d);
			return DateTime_(Date::FromExcel(id), d - id);
		}
		THROW("Invalid input type -- timepoint expected");
	}

	String_ XToString(PyObject* src)
	{
		if (PyUnicode_Check(src))
			return UnicodeToString(src);
		if (PyBytes_Check(src))
			return String_(PyBytes_AsString(src));
		THROW("Invalid input type -- string expected");
	}

	Cell_ XToCell(PyObject* src)
	{
		if (IsMissing(src))
			return Cell_();
		if (PyBool_Check(src))
			return Cell_(src == Py_True);
		EnsureDateTime();
		if (PyDateTime_Check(src))
			return Cell_(XToDateTime(src));
		if (PyDate_Check(src))
			return Cell_(XToDate(src));
		if (PyUnicode_Check(src) || PyBytes_Check(src))
			return Cell_(XToString(src));
		if (PyLong_Check(src) || PyFloat_Check(src))
			return Cell_(XToNumber(src));
		THROW("Invalid input type -- can't form a cell");
	}

	template<class T_, class F_> Vector_<T_> ReadVector(PyObject* src, bool optional, F_ read_scalar)
	{
		if (IsMissing(src))
		{
			REQUIRE(optional, "Missing input vector");
			return Vector_<T_>();
		}
		if (!IsSeq(src))
			return Vector_<T_>(1, read_scalar(src));

		const int n = SeqSize(src);
		Vector_<T_> retval;
		retval.reserve(n);
		for (int ii = 0; ii < n; ++ii)
		{
			BorrowedItem_ item(SeqGet(src, ii), true);
			if (IsMissing(item.Get()))
				break;
			retval.push_back(read_scalar(item.Get()));
		}
		REQUIRE(!retval.empty() || optional, "Missing contents of input vector");
		return retval;
	}

	template<class E_, class F_> Matrix_<E_> ReadMatrix(PyObject* src, bool optional, F_ read_scalar)
	{
		if (IsMissing(src))
		{
			REQUIRE(optional, "Missing input matrix");
			return Matrix_<E_>();
		}
		if (!IsSeq(src))
		{
			Matrix_<E_> retval(1, 1);
			retval(0, 0) = read_scalar(src);
			return retval;
		}

		const int n0 = SeqSize(src);
		if (n0 == 0)
		{
			REQUIRE(optional, "Missing contents of matrix");
			return Matrix_<E_>();
		}

		BorrowedItem_ first(SeqGet(src, 0), true);
		if (!IsSeq(first.Get()))
		{
			Matrix_<E_> retval(n0, 1);
			retval(0, 0) = read_scalar(first.Get());
			for (int ii = 1; ii < n0; ++ii)
			{
				BorrowedItem_ item(SeqGet(src, ii), true);
				retval(ii, 0) = read_scalar(item.Get());
			}
			return retval;
		}

		const int n1 = SeqSize(first.Get());
		REQUIRE(n1 > 0 || optional, "Missing contents of matrix");
		Matrix_<E_> retval(n0, n1);
		for (int ii = 0; ii < n0; ++ii)
		{
			BorrowedItem_ row(SeqGet(src, ii), true);
			REQUIRE(IsSeq(row.Get()), "Matrix rows must themselves be sequences");
			REQUIRE(SeqSize(row.Get()) == n1, "Ragged matrix input");
			for (int jj = 0; jj < n1; ++jj)
			{
				BorrowedItem_ item(SeqGet(row.Get(), jj), true);
				retval(ii, jj) = read_scalar(item.Get());
			}
		}
		return retval;
	}

	PyObject* NewFloat(double d)
	{
		PyObject* p = PyFloat_FromDouble(d);
		REQUIRE(p, "Failed to allocate Python float");
		return p;
	}
	PyObject* NewLong(long v)
	{
		PyObject* p = PyLong_FromLong(v);
		REQUIRE(p, "Failed to allocate Python int");
		return p;
	}
	PyObject* NewBool(bool b)
	{
		PyObject* p = PyBool_FromLong(b ? 1 : 0);
		REQUIRE(p, "Failed to allocate Python bool");
		return p;
	}
	PyObject* NewString(const String_& s)
	{
		PyObject* p = PyUnicode_FromStringAndSize(s.c_str(), static_cast<Py_ssize_t>(s.size()));
		REQUIRE(p, "Failed to allocate Python string");
		return p;
	}
	PyObject* NewDate(const Date_& dt)
	{
		EnsureDateTime();
		REQUIRE(dt.IsValid(), "Can't return an invalid date");
		PyObject* p = PyDate_FromDate(Date::Year(dt), Date::Month(dt), Date::Day(dt));
		REQUIRE(p, "Failed to allocate Python date");
		return p;
	}
	PyObject* NewDateTime(const DateTime_& dt)
	{
		EnsureDateTime();
		const Date_ d = dt.Date();
		REQUIRE(d.IsValid(), "Can't return an invalid timepoint");
		const int secs = static_cast<int>(dt.Frac() * 86400.0 + 0.5);
		const int h = secs / 3600;
		const int m = (secs / 60) % 60;
		const int s = secs % 60;
		PyObject* p = PyDateTime_FromDateAndTime(Date::Year(d), Date::Month(d), Date::Day(d), h, m, s, 0);
		REQUIRE(p, "Failed to allocate Python datetime");
		return p;
	}

	struct CellToPy_
	{
		PyObject* operator()(std::monostate) const { Py_RETURN_NONE; }
		PyObject* operator()(bool b) const { return NewBool(b); }
		PyObject* operator()(double d) const { return NewFloat(d); }
		PyObject* operator()(const Date_& dt) const { return NewDate(dt); }
		PyObject* operator()(const DateTime_& dt) const { return NewDateTime(dt); }
		PyObject* operator()(const String_& s) const { return NewString(s); }
	};

	PyObject* NewCell(const Cell_& c)
	{
		return c.Visit(CellToPy_());
	}

	template<class E_, class F_> PyObject* NewList(const Vector_<E_>& v, F_ convert)
	{
		PyObject* list = PyList_New(v.size());
		REQUIRE(list, "Failed to allocate Python list");
		for (int ii = 0; ii < v.size(); ++ii)
		{
			PyObject* item = convert(v[ii]);
			if (!item)
			{
				Py_DECREF(list);
				THROW("Failed to convert list element");
			}
			PyList_SET_ITEM(list, ii, item);
		}
		return list;
	}

	template<class E_, class F_> PyObject* NewMatrix(const Matrix_<E_>& m, F_ convert)
	{
		PyObject* rows = PyList_New(m.Rows());
		REQUIRE(rows, "Failed to allocate Python list");
		for (int ir = 0; ir < m.Rows(); ++ir)
		{
			PyObject* row = PyList_New(m.Cols());
			if (!row)
			{
				Py_DECREF(rows);
				THROW("Failed to allocate Python list");
			}
			for (int ic = 0; ic < m.Cols(); ++ic)
			{
				PyObject* item = convert(m(ir, ic));
				if (!item)
				{
					Py_DECREF(row);
					Py_DECREF(rows);
					THROW("Failed to convert matrix element");
				}
				PyList_SET_ITEM(row, ic, item);
			}
			PyList_SET_ITEM(rows, ir, row);
		}
		return rows;
	}

	PyObject* RowItem(PyObject* src, int i_row, int i_col, bool optional)
	{
		if (IsMissing(src) || !IsSeq(src))
		{
			REQUIRE(optional, "Can't read records from a scalar");
			return nullptr;
		}
		REQUIRE(i_row < SeqSize(src), "Record row out of range");
		BorrowedItem_ row(SeqGet(src, i_row), true);
		if (!IsSeq(row.Get()))
		{
			REQUIRE(i_col == 0, "Record index out of range");
			Py_INCREF(row.Get());
			return row.Get();
		}
		if (i_col >= SeqSize(row.Get()))
		{
			REQUIRE(optional, "Record index out of range");
			return nullptr;
		}
		return SeqGet(row.Get(), i_col);
	}

	struct PyFunc_
	{
		String_ dll_;
		Python::api_func func_;
		String_ pyName_;
		String_ help_;
		String_ argNames_;
		Vector_<String_> argHelp_;
	};

	Vector_<PyFunc_>& TheFunctions()
	{
		RETURN_STATIC(Vector_<PyFunc_>);
	}
}	// leave local


Python::ArgReader_::ArgReader_(PyObject* positional, PyObject* named)
	: positional_(positional), named_(named), np_(0), ip_(0)
{
	if (!IsMissing(positional_))
	{
		REQUIRE(PyTuple_Check(positional_), "Unexpected positional args type");
		np_ = static_cast<int>(PyTuple_GET_SIZE(positional_));
	}
	if (IsMissing(named_))
		named_ = nullptr;
	else
		REQUIRE(PyDict_Check(named_), "Unrecognized named args type");
}

PyObject* Python::ArgReader_::Next(const char* name)
{
	if (ip_ < np_)
	{
		if (named_ && PyDict_GetItemString(named_, name))
			THROW("Input is both positional and named");
		return PyTuple_GetItem(positional_, ip_++);
	}
	if (named_)
		return PyDict_GetItemString(named_, name);
	return nullptr;
}

double Python::ToNumber(PyObject* src)
{
	REQUIRE(!IsMissing(src), "Missing input number");
	return XToNumber(src);
}
std::optional<double> Python::ToOptionalNumber(PyObject* src)
{
	if (IsMissing(src))
		return std::optional<double>();
	return XToNumber(src);
}

int Python::ToInteger(PyObject* src)
{
	REQUIRE(!IsMissing(src), "Missing input integer");
	return XToInteger(src);
}
std::optional<int> Python::ToOptionalInteger(PyObject* src)
{
	if (IsMissing(src))
		return std::optional<int>();
	return XToInteger(src);
}

bool Python::ToBoolean(PyObject* src)
{
	REQUIRE(!IsMissing(src), "Missing input boolean");
	return XToBoolean(src);
}
std::optional<bool> Python::ToOptionalBoolean(PyObject* src)
{
	if (IsMissing(src))
		return std::optional<bool>();
	return XToBoolean(src);
}

String_ Python::ToString(PyObject* src, bool optional)
{
	if (IsMissing(src))
	{
		REQUIRE(optional, "Missing input string");
		return String_();
	}
	return XToString(src);
}

Date_ Python::ToDate(PyObject* src, bool optional)
{
	if (IsMissing(src))
	{
		REQUIRE(optional, "Missing input date");
		return Date_();
	}
	return XToDate(src);
}
std::optional<Date_> Python::ToOptionalDate(PyObject* src)
{
	if (IsMissing(src))
		return std::optional<Date_>();
	return XToDate(src);
}

DateTime_ Python::ToDateTime(PyObject* src, bool optional)
{
	if (IsMissing(src))
	{
		REQUIRE(optional, "Missing input timepoint");
		return DateTime_();
	}
	return XToDateTime(src);
}
std::optional<DateTime_> Python::ToOptionalDateTime(PyObject* src)
{
	if (IsMissing(src))
		return std::optional<DateTime_>();
	return XToDateTime(src);
}

Cell_ Python::ToCell(PyObject* src, bool optional)
{
	if (IsMissing(src))
	{
		REQUIRE(optional, "Missing input cell");
		return Cell_();
	}
	return XToCell(src);
}

Dictionary_ Python::ToDictionary(PyObject* src, bool optional)
{
	Dictionary_ retval;
	if (IsMissing(src))
	{
		REQUIRE(optional, "Missing input dictionary");
		return retval;
	}
	if (PyDict_Check(src))
	{
		PyObject* key = nullptr;
		PyObject* val = nullptr;
		Py_ssize_t pos = 0;
		while (PyDict_Next(src, &pos, &key, &val))
		{
			REQUIRE(PyUnicode_Check(key), "Dictionary keys must be strings");
			if (!IsMissing(val))
				retval.Insert(UnicodeToString(key), XToCell(val));
		}
		return retval;
	}
	if (IsSeq(src))
	{
		const int n = SeqSize(src);
		for (int ii = 0; ii < n; ++ii)
		{
			BorrowedItem_ row(SeqGet(src, ii), true);
			REQUIRE(IsSeq(row.Get()) && SeqSize(row.Get()) >= 2, "Dictionary sequence entries must be (key, value) pairs");
			BorrowedItem_ key(SeqGet(row.Get(), 0), true);
			BorrowedItem_ val(SeqGet(row.Get(), 1), true);
			if (!IsMissing(val.Get()))
				retval.Insert(XToString(key.Get()), XToCell(val.Get()));
		}
		return retval;
	}
	THROW("Invalid input -- dictionary must be a dict or a sequence of pairs");
}

Handle_<Storable_> Python::ToHandleBase(_ENV, PyObject* src, bool optional)
{
	const String_ tag = ToString(src, optional);
	if (tag.empty())
	{
		REQUIRE(optional, "Missing input handle");
		return Handle_<Storable_>();
	}
	auto repo = Environment::Find<ObjectAccess_>(_env);
	assert(repo);
	return repo->Fetch(tag);
}

std::uint64_t Python::ToLong(PyObject* src)
{
	REQUIRE(!IsMissing(src), "Missing input long");
	return XToLong(src);
}
std::optional<std::uint64_t> Python::ToOptionalLong(PyObject* src)
{
	if (IsMissing(src))
		return std::optional<std::uint64_t>();
	return XToLong(src);
}

Vector_<> Python::ToNumberVector(PyObject* src, bool optional)
{
	return ReadVector<double>(src, optional, [](PyObject* p){ return XToNumber(p); });
}
Vector_<int> Python::ToIntegerVector(PyObject* src, bool optional)
{
	return ReadVector<int>(src, optional, [](PyObject* p){ return XToInteger(p); });
}
Vector_<bool> Python::ToBoolVector(PyObject* src, bool optional)
{
	return ReadVector<bool>(src, optional, [](PyObject* p){ return XToBoolean(p); });
}
Vector_<String_> Python::ToStringVector(PyObject* src, bool optional)
{
	return ReadVector<String_>(src, optional, [](PyObject* p){ return XToString(p); });
}
Vector_<Date_> Python::ToDateVector(PyObject* src, bool optional)
{
	return ReadVector<Date_>(src, optional, [](PyObject* p){ return XToDate(p); });
}
Vector_<Cell_> Python::ToCellVector(PyObject* src, bool optional)
{
	return ReadVector<Cell_>(src, optional, [](PyObject* p){ return XToCell(p); });
}
Vector_<Handle_<Storable_> > Python::ToHandleVectorBase(_ENV, PyObject* src, bool optional)
{
	return ReadVector<Handle_<Storable_> >(src, optional, [&](PyObject* p){ return ToHandleBase(_env, p, false); });
}

Matrix_<double> Python::ToNumberMatrix(PyObject* src, bool optional)
{
	return ReadMatrix<double>(src, optional, [](PyObject* p){ return XToNumber(p); });
}
Matrix_<int> Python::ToIntegerMatrix(PyObject* src, bool optional)
{
	return ReadMatrix<int>(src, optional, [](PyObject* p){ return XToInteger(p); });
}
Matrix_<String_> Python::ToStringMatrix(PyObject* src, bool optional)
{
	return ReadMatrix<String_>(src, optional, [](PyObject* p){ return XToString(p); });
}
Matrix_<Cell_> Python::ToCellMatrix(PyObject* src, bool optional, bool)
{
	return ReadMatrix<Cell_>(src, optional, [](PyObject* p){ return XToCell(p); });
}

Python::RowReader_::RowReader_(PyObject* src) : src_(src), iRow_(0) {}

int Python::RowReader_::Rows() const
{
	if (IsMissing(src_))
		return 0;
	REQUIRE(IsSeq(src_), "Can't read records from a scalar");
	return SeqSize(src_);
}

double Python::RowReader_::ExtractDouble(int i_col) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, false), true);
	return XToNumber(item.Get());
}
double Python::RowReader_::ExtractDouble(int i_col, double def_val) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, true), true);
	return IsMissing(item.Get()) ? def_val : XToNumber(item.Get());
}
int Python::RowReader_::ExtractInt(int i_col) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, false), true);
	return XToInteger(item.Get());
}
int Python::RowReader_::ExtractInt(int i_col, int def_val) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, true), true);
	return IsMissing(item.Get()) ? def_val : XToInteger(item.Get());
}
bool Python::RowReader_::ExtractBool(int i_col) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, false), true);
	return XToBoolean(item.Get());
}
String_ Python::RowReader_::ExtractString(int i_col) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, false), true);
	return XToString(item.Get());
}
Date_ Python::RowReader_::ExtractDate(int i_col) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, false), true);
	return XToDate(item.Get());
}
Handle_<Storable_> Python::RowReader_::ExtractHandleBase(_ENV, int i_col) const
{
	BorrowedItem_ item(RowItem(src_, iRow_, i_col, false), true);
	return ToHandleBase(_env, item.Get(), false);
}

Python::Retval_::Retval_() {}
Python::Retval_::~Retval_()
{
	for (auto p : values_)
		Py_XDECREF(p);
}

void Python::Retval_::Load(double s) { values_.push_back(NewFloat(s)); }
void Python::Retval_::Load(int s) { values_.push_back(NewLong(s)); }
void Python::Retval_::Load(bool s) { values_.push_back(NewBool(s)); }
void Python::Retval_::Load(const String_& s) { values_.push_back(NewString(s)); }
void Python::Retval_::Load(const Date_& s) { values_.push_back(NewDate(s)); }
void Python::Retval_::Load(const DateTime_& s) { values_.push_back(NewDateTime(s)); }
void Python::Retval_::Load(const Cell_& s) { values_.push_back(NewCell(s)); }

void Python::Retval_::LoadBase(_ENV, const Handle_<Storable_>& s)
{
	static const RepositoryErase_ ERASE;
	auto repo = Environment::Find<ObjectAccess_>(_env);
	assert(repo);
	Load(repo->Add(s, ERASE));
}

void Python::Retval_::Load(const Vector_<>& v)
{
	values_.push_back(NewList(v, [](double x){ return NewFloat(x); }));
}
void Python::Retval_::Load(const Vector_<int>& v)
{
	values_.push_back(NewList(v, [](int x){ return NewLong(x); }));
}
void Python::Retval_::Load(const Vector_<bool>& v)
{
	values_.push_back(NewList(v, [](bool x){ return NewBool(x); }));
}
void Python::Retval_::Load(const Vector_<String_>& v)
{
	values_.push_back(NewList(v, [](const String_& x){ return NewString(x); }));
}
void Python::Retval_::Load(const Vector_<Date_>& v)
{
	values_.push_back(NewList(v, [](const Date_& x){ return NewDate(x); }));
}
void Python::Retval_::Load(const Vector_<Cell_>& v)
{
	values_.push_back(NewList(v, [](const Cell_& x){ return NewCell(x); }));
}
void Python::Retval_::LoadBase(_ENV, const Vector_<Handle_<Storable_> >& v)
{
	static const RepositoryErase_ ERASE;
	auto repo = Environment::Find<ObjectAccess_>(_env);
	assert(repo);
	Vector_<String_> tags;
	for (auto h : v)
		tags.push_back(repo->Add(h, ERASE));
	Load(tags);
}
void Python::Retval_::Load(const Matrix_<double>& m)
{
	values_.push_back(NewMatrix(m, [](double x){ return NewFloat(x); }));
}
void Python::Retval_::Load(const Matrix_<int>& m)
{
	values_.push_back(NewMatrix(m, [](int x){ return NewLong(x); }));
}
void Python::Retval_::Load(const Matrix_<String_>& m)
{
	values_.push_back(NewMatrix(m, [](const String_& x){ return NewString(x); }));
}
void Python::Retval_::Load(const Matrix_<Cell_>& m)
{
	values_.push_back(NewMatrix(m, [](const Cell_& x){ return NewCell(x); }));
}

PyObject* Python::Retval_::ToPython()
{
	if (values_.empty())
		Py_RETURN_NONE;
	if (values_.size() == 1)
	{
		PyObject* p = values_[0];
		values_.clear();
		return p;
	}
	PyObject* tup = PyTuple_New(values_.size());
	REQUIRE(tup, "Failed to allocate Python tuple");
	for (int ii = 0; ii < values_.size(); ++ii)
		PyTuple_SET_ITEM(tup, ii, values_[ii]);
	values_.clear();
	return tup;
}

PyObject* Python::Error(const String_& what, const char* arg_name)
{
	String_ display = what;
	if (arg_name)
		display = String_(arg_name) + ": " + what;
	PyErr_SetString(PyExc_RuntimeError, display.c_str());
	return nullptr;
}

void Python::Register
	(const String_& dll_name,
	 api_func func,
	 const String_& py_name,
	 const String_& help,
	 const String_& arg_names,
	 const Vector_<String_>& arg_help)
{
	TheFunctions().push_back(PyFunc_{ dll_name, func, py_name, help, arg_names, arg_help });
}

PyObject* Python::InitModule(const char* py_name, const char* dll_short_name)
{
	EnsureDateTime();

	Vector_<PyMethodDef> methods;
	for (const auto& f : TheFunctions())
	{
		if (f.dll_ != dll_short_name)
			continue;
		PyMethodDef def{};
		def.ml_name = _strdup(f.pyName_.c_str());
		def.ml_meth = reinterpret_cast<PyCFunction>(f.func_);
		def.ml_flags = METH_VARARGS | METH_KEYWORDS;
		def.ml_doc = _strdup(f.help_.c_str());
		methods.push_back(def);
	}
	PyMethodDef sentinel{};
	methods.push_back(sentinel);

	// PyModuleDef and method table must outlive the module
	auto* keptMethods = new Vector_<PyMethodDef>(std::move(methods));
	auto* def = new PyModuleDef{};
	memset(def, 0, sizeof(*def));
	def->m_base = PyModuleDef_HEAD_INIT;
	def->m_name = _strdup(py_name);
	def->m_size = -1;
	def->m_methods = keptMethods->empty() ? nullptr : &(*keptMethods)[0];
	return PyModule_Create(def);
}

extern "C" __declspec(dllexport) PyObject* PyInit__da_base()
{
	return Python::InitModule("_da_base", "Base");
}
