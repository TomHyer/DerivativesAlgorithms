
#pragma once

// Framework for Excel interface

// From the Microsoft Excel Developer's Kit, Version 14
// Copyright (c) 1997 - 2010 Microsoft Corporation.

#define rwMaxO8			(65536)
#define colMaxO8		(256)
#define cchMaxStz		(255)
#define MAXSHORTINT		0x7fff

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

struct xloper12;
typedef xloper12 OPER_;
OPER_* TempStr(const String_& src);	// takes NON-BYTE-COUNTED input
#define XL_CALLBACK Excel12v

namespace Log
{
	inline void Write(const char* msg) {}	// logging is not supported
}

namespace Excel
{
	inline void InitializeSessionIfNeeded() {}	// no runtime initialization required
	BASE_EXPORT double ToDouble(const OPER_* src);
	BASE_EXPORT std::optional<double> ToDouble(const OPER_* src, bool);	// use overloading to know the return type
	BASE_EXPORT int ToInt(const OPER_* src);
	BASE_EXPORT std::optional<int> ToInt(const OPER_* src, bool);
	BASE_EXPORT bool ToBool(const OPER_* src);
	BASE_EXPORT std::optional<bool> ToBool(const OPER_* src, bool);
	BASE_EXPORT String_ ToString(const OPER_* src, bool optional = false);
	BASE_EXPORT Date_ ToDate(const OPER_* src, bool optional = false);
	BASE_EXPORT DateTime_ ToDateTime(const OPER_* src, bool optional = false);
	BASE_EXPORT Cell_ ToCell(const OPER_* src, bool optional = false);
	BASE_EXPORT Dictionary_ ToDictionary(const OPER_* src, bool optional = false);
	BASE_EXPORT Handle_<Storable_> ToHandleBase(_ENV, const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<> ToDoubleVector(const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<int> ToIntVector(const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<bool> ToBoolVector(const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<String_> ToStringVector(const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<Date_> ToDateVector(const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<Cell_> ToCellVector(const OPER_* src, bool optional = false);
	BASE_EXPORT Vector_<Handle_<Storable_> > ToHandleBaseVector(_ENV, const OPER_* src, bool optional = false);
	BASE_EXPORT Matrix_<double> ToDoubleMatrix(const OPER_* src, bool optional = false);
	BASE_EXPORT Matrix_<int> ToIntMatrix(const OPER_* src, bool optional = false);
	// Matrix of bool is presently not supported
	BASE_EXPORT Matrix_<String_> ToStringMatrix(const OPER_* src, bool optional = false);
	BASE_EXPORT Matrix_<Cell_> ToCellMatrix(const OPER_* src, bool optional = false, bool greedy = false);

   template<class T_> T_ ToEnum(const OPER_* src) { return T_(ToString(src)); }
   template<class T_> std::optional<T_> ToEnum(const OPER_* src, bool optional)
   {
      std::optional<T_> retval;
      const String_ s = ToString(src, optional);
      if (!s.empty())
         retval = T_(s);
      return retval;
   }

   template<class T_> Handle_<T_> ToHandle(_ENV, const OPER_* src, bool optional = false)
	{
		const Handle_<Storable_> base = ToHandleBase(_env, src, optional);
		if (base.Empty() && optional)
			return Handle_<T_>();

		const Handle_<T_> retval = handle_cast<T_>(base);
		REQUIRE(!retval.Empty(), "Input object has wrong type");
		return retval;
	}
	template<class T_> Vector_<Handle_<T_> > ToHandleVector(_ENV, const OPER_* src)
	{
		const Vector_<Handle_<Storable_> > base = ToHandleBaseVector(_env, src);
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
      const OPER_* src_;
   public:
      int iRow_;
      RowReader_(const OPER_* src) : src_(src), iRow_(0) {}
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
      (_ENV, const OPER_* src, bool optional = false)
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

	// matrix of handles is presently not supported

	template<class T_> T_ ToSettings(const OPER_* src, bool optional = false)
	{
		return T_(ToDictionary(src, optional));
	}

	struct BASE_EXPORT Retval_
	{
		Vector_<Matrix_<Cell_> > values_;
		Vector_<int> scalars_;	// locations of the scalars
		Vector_<int> vectors_;
		Vector_<int> matrices_;

		void Load(double s);
		void Load(const String_& s);
		void Load(const Cell_& s);
		void LoadBase(_ENV, const Handle_<Storable_>& s);
		template<class T_> void Load(_ENV, const Handle_<T_>& h) { LoadBase(_env, handle_cast<Storable_>(h)); }
		void Load(const Vector_<>& v);
		void Load(const Vector_<int>& v);
		void Load(const Vector_<String_>& v);
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
		OPER_* ToXloper() const;
		OPER_* ToXloper(const String_& format) const;
	};

	OPER_* Error
		(const String_& what,
		const char* arg_name);

	void Register
		(const String_& dll_name,
		const String_& c_name,
		const String_& xl_name,
		const String_& help,
		const String_& arg_types,
		const String_& arg_names,
		const Vector_<String_>& arg_help,
		bool volatile);
}

extern "C"
{
	BASE_EXPORT char* GetTempMemory(size_t cBytes);
	BASE_EXPORT void FreeAllTempMemory();
	BASE_EXPORT OPER_* TempNum(double d);
}
