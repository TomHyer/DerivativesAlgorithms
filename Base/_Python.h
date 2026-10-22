
#pragma once

// Framework for the Python 3 interface
// Parallel to _Excel.h: same translator names, so Machinist CTypeTranslator works unchanged.
// This header must not include Python.h -- PyObject is forward-declared.
// Requires Python 3 (module init is PyInit_*, strings are Unicode).

#include "Storable.h"
#include "Matrix.h"
#include "Optionals.h"
#include "Dictionary.h"
#include "Environment.h"
#include "_Reader.h"

struct Cell_;
class Date_;
class DateTime_;
class Dictionary_;

typedef struct _object PyObject;

namespace Python
{
	using api_func = PyObject* (*)(PyObject* self, PyObject* positional, PyObject* named);

	class BASE_EXPORT ArgReader_
	{
		PyObject* positional_;
		PyObject* named_;
		int np_;
		int ip_;
	public:
		ArgReader_(PyObject* positional, PyObject* named);
		PyObject* Next(const char* name);	// borrowed; nullptr if missing
		PyObject* Next(const String_& name) { return Next(name.c_str()); }
	};

	BASE_EXPORT double ToNumber(PyObject* src);
	BASE_EXPORT std::optional<double> ToOptionalNumber(PyObject* src);
	BASE_EXPORT int ToInteger(PyObject* src);
	BASE_EXPORT std::optional<int> ToOptionalInteger(PyObject* src);
	BASE_EXPORT bool ToBoolean(PyObject* src);
	BASE_EXPORT std::optional<bool> ToOptionalBoolean(PyObject* src);
	BASE_EXPORT String_ ToString(PyObject* src, bool optional = false);
	inline String_ ToOptionalString(PyObject* src) { return ToString(src, true); }
	BASE_EXPORT Date_ ToDate(PyObject* src, bool optional = false);
	BASE_EXPORT std::optional<Date_> ToOptionalDate(PyObject* src);
	BASE_EXPORT DateTime_ ToDateTime(PyObject* src, bool optional = false);
	BASE_EXPORT std::optional<DateTime_> ToOptionalDateTime(PyObject* src);
	BASE_EXPORT Cell_ ToCell(PyObject* src, bool optional = false);
	BASE_EXPORT Dictionary_ ToDictionary(PyObject* src, bool optional = false);
	BASE_EXPORT Handle_<Storable_> ToHandleBase(_ENV, PyObject* src, bool optional = false);
	inline Handle_<Storable_> ToOptionalHandleBase(_ENV, PyObject* src) { return ToHandleBase(_env, src, true); }
	BASE_EXPORT std::uint64_t ToLong(PyObject* src);
	BASE_EXPORT std::optional<std::uint64_t> ToOptionalLong(PyObject* src);

	BASE_EXPORT Vector_<> ToNumberVector(PyObject* src, bool optional = false);
	BASE_EXPORT Vector_<int> ToIntegerVector(PyObject* src, bool optional = false);
	BASE_EXPORT Vector_<bool> ToBoolVector(PyObject* src, bool optional = false);
	BASE_EXPORT Vector_<String_> ToStringVector(PyObject* src, bool optional = false);
	BASE_EXPORT Vector_<Date_> ToDateVector(PyObject* src, bool optional = false);
	BASE_EXPORT Vector_<Cell_> ToCellVector(PyObject* src, bool optional = false);
	BASE_EXPORT Vector_<Handle_<Storable_> > ToHandleVectorBase(_ENV, PyObject* src, bool optional = false);

	BASE_EXPORT Matrix_<double> ToNumberMatrix(PyObject* src, bool optional = false);
	BASE_EXPORT Matrix_<int> ToIntegerMatrix(PyObject* src, bool optional = false);
	BASE_EXPORT Matrix_<String_> ToStringMatrix(PyObject* src, bool optional = false);
	BASE_EXPORT Matrix_<Cell_> ToCellMatrix(PyObject* src, bool optional = false, bool greedy = false);

	template<class T_> T_ ToEnum(PyObject* src) { return T_(ToString(src)); }
	template<class T_> std::optional<T_> ToEnum(PyObject* src, bool optional)
	{
		std::optional<T_> retval;
		const String_ s = ToString(src, optional);
		if (!s.empty())
			retval = T_(s);
		return retval;
	}
	template<class T_> std::optional<T_> ToOptionalEnum(PyObject* src) { return ToEnum<T_>(src, true); }

	template<class T_> Handle_<T_> ToHandle(_ENV, PyObject* src, bool optional = false)
	{
		const Handle_<Storable_> base = ToHandleBase(_env, src, optional);
		if (base.Empty() && optional)
			return Handle_<T_>();

		const Handle_<T_> retval = handle_cast<T_>(base);
		REQUIRE(!retval.Empty(), "Input object has wrong type");
		return retval;
	}
	template<class T_> Handle_<T_> ToOptionalHandle(_ENV, PyObject* src)
	{
		return ToHandle<T_>(_env, src, true);
	}
	template<class T_> Vector_<Handle_<T_> > ToHandleVector(_ENV, PyObject* src, bool optional = false)
	{
		const Vector_<Handle_<Storable_> > base = ToHandleVectorBase(_env, src, optional);
		Vector_<Handle_<T_> > retval;
		for (auto b : base)
		{
			retval.push_back(handle_cast<T_>(b));
			REQUIRE(!retval.back().Empty(), "Input object has wrong type");
		}
		return retval;
	}

	class BASE_EXPORT RowReader_ : public UIRow_
	{
		PyObject* src_;
	public:
		int iRow_;
		RowReader_(PyObject* src);
		int Rows() const;

		double ExtractDouble(int i_col) const override;
		double ExtractDouble(int i_col, double def_val) const override;
		int ExtractInt(int i_col) const override;
		int ExtractInt(int i_col, int def_val) const override;
		bool ExtractBool(int i_col) const override;
		String_ ExtractString(int i_col) const override;
		Date_ ExtractDate(int i_col) const override;
		Handle_<Storable_> ExtractHandleBase(_ENV, int i_col) const override;
	};

	template<class T_> Vector_<T_> ToRecordVector
		(_ENV, PyObject* src, bool optional = false)
	{
		Vector_<T_> retval;
		RowReader_ reader(src);
		int rows = reader.Rows();
		if (rows == 0)
		{
			REQUIRE(optional, "No records found");
			return retval;
		}
		for (int ir = 0; ir < rows; ++ir)
		{
			retval.push_back(T_(_env, reader));
			++reader.iRow_;
		}
		return retval;
	}

	template<class T_> T_ ToSettings(PyObject* src, bool optional = false)
	{
		return T_(ToDictionary(src, optional));
	}

	struct BASE_EXPORT Retval_
	{
		Vector_<PyObject*> values_;	// owned new-references

		Retval_();
		~Retval_();
		Retval_(const Retval_&) = delete;
		Retval_& operator=(const Retval_&) = delete;

		void Load(double s);
		void Load(int s);
		void Load(bool s);
		void Load(const String_& s);
		void Load(const Date_& s);
		void Load(const DateTime_& s);
		void Load(const Cell_& s);
		void LoadBase(_ENV, const Handle_<Storable_>& s);
		template<class T_> void Load(_ENV, const Handle_<T_>& h) { LoadBase(_env, handle_cast<Storable_>(h)); }
		void Load(const Vector_<>& v);
		void Load(const Vector_<int>& v);
		void Load(const Vector_<bool>& v);
		void Load(const Vector_<String_>& v);
		void Load(const Vector_<Date_>& v);
		void Load(const Vector_<Cell_>& v);
		void LoadBase(_ENV, const Vector_<Handle_<Storable_> >& v);
		template<class T_> void Load(_ENV, const Vector_<Handle_<T_> >& v)
		{
			LoadBase(_env, Apply([](const Handle_<T_>& h){return handle_cast<Storable_>(h); }, v));
		}
		void Load(const Matrix_<double>& m);
		void Load(const Matrix_<int>& m);
		void Load(const Matrix_<String_>& m);
		void Load(const Matrix_<Cell_>& m);
		PyObject* ToPython();	// steals the stored references
	};

	BASE_EXPORT PyObject* Error
		(const String_& what,
		 const char* arg_name);

	BASE_EXPORT void Register
		(const String_& dll_name,
		 api_func func,
		 const String_& py_name,
		 const String_& help,
		 const String_& arg_names,
		 const Vector_<String_>& arg_help);

	BASE_EXPORT PyObject* InitModule
		(const char* py_name,
		 const char* dll_short_name);
}
