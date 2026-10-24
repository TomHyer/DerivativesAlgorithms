
// Cookie/biscuit FFI: one C ABI for every host language.
// Hosts stash composite values, call ffi_* with cookies (and C scalars),
// and unpack a biscuit (output-cookie list, or error).

#pragma once

#include "Storable.h"
#include "Vectors.h"
#include "Matrix.h"
#include "Cell.h"
#include "Dictionary.h"
#include "Exceptions.h"

extern "C"
{
	BASE_EXPORT void HandleDelete(void* ph);

	BASE_EXPORT int HandleCookie(void* ph);
	BASE_EXPORT int HandleCookieFromTag(const char* tag);	// repository lookup
	BASE_EXPORT int StringCookie(const char* s);
	BASE_EXPORT int VectorDoubleCookie(const double* vals, int size);
	BASE_EXPORT int VectorIntCookie(const int* vals, int size);
	BASE_EXPORT int VectorStringCookie(const char* const* vals, int size);
	BASE_EXPORT int VectorCellCookie(int size, const int* types, const double* nums, const char* const* strs);
	BASE_EXPORT int VectorHandleCookie(const int* handle_cookies, int size);
	BASE_EXPORT int MatrixDoubleCookie(const double* vals, int rows, int cols);
	BASE_EXPORT int MatrixIntCookie(const int* vals, int rows, int cols);
	BASE_EXPORT int MatrixStringCookie(const char* const* vals, int rows, int cols);
	BASE_EXPORT int MatrixCellCookie
		(int rows, int cols, const int* types, const double* nums, const char* const* strs);

	BASE_EXPORT int BiscuitCount(double biscuit);
	BASE_EXPORT int BiscuitCookie(double biscuit, int which);
	BASE_EXPORT char* BiscuitError(double biscuit);
	BASE_EXPORT void CookieFreeString(char* p);

	BASE_EXPORT void CookieRelease(int cookie);

	BASE_EXPORT void* CookieHandle(int cookie);
	BASE_EXPORT const char* CookieHandleTag(int cookie);	// Storable_::name_; does not destroy the cookie

	BASE_EXPORT int CookieInt(int cookie);
	BASE_EXPORT double CookieDouble(int cookie);
	BASE_EXPORT int CookieBool(int cookie);
	BASE_EXPORT const char* CookieStringBase(int cookie);	// does not destroy
	BASE_EXPORT const char* CookieStringCopy(int cookie);	// strdup; destroy cookie; free with CookieFreeString

	BASE_EXPORT int CookieVectorSize(int cookie);
	BASE_EXPORT int CookieMatrixRows(int cookie);
	BASE_EXPORT int CookieMatrixCols(int cookie);

	BASE_EXPORT const double* CookieVectorDoubleBase(int cookie);
	BASE_EXPORT void CookieVectorDoubleCopyVals(int cookie, double* dst);
	BASE_EXPORT const int* CookieVectorIntBase(int cookie);
	BASE_EXPORT void CookieVectorIntCopyVals(int cookie, int* dst);
	BASE_EXPORT void CookieVectorStringCopyVals(int cookie, char** dst);	// strdup each; free with CookieFreeString
	BASE_EXPORT void CookieVectorCellCopy(int cookie, int* types, double* nums, char** strs);
	BASE_EXPORT void CookieVectorHandleCopyVals(int cookie, void** dst);	// new Handle_*; owner is caller

	BASE_EXPORT void CookieMatrixDoubleCopyVals(int cookie, double* dst);
	BASE_EXPORT void CookieMatrixIntCopyVals(int cookie, int* dst);
	BASE_EXPORT void CookieMatrixStringCopyVals(int cookie, char** dst);
	BASE_EXPORT void CookieMatrixCellCopy
		(int cookie, int* types, double* nums, char** strs);
}

namespace Cookie
{
	struct BASE_EXPORT Input_
	{
		Vector_<int> used_;
		~Input_();

		const Handle_<Storable_>& HandleBase(int cookie, bool optional = false);
		template<class T_> Handle_<T_> Handle(int cookie, bool optional = false)
		{
			Handle_<Storable_> temp = HandleBase(cookie, optional);
			Handle_<T_> retval = handle_cast<T_>(temp);
			REQUIRE(retval || !temp, "Input handle has wrong type");
			return retval;
		}

		const Vector_<>& VectorDouble(int cookie, bool optional = false);
		const Vector_<int>& VectorInt(int cookie, bool optional = false);
		const Vector_<String_>& VectorString(int cookie, bool optional = false);
		const Vector_<Handle_<Storable_>>& VectorHandleBase(int cookie, bool optional = false);
		template<class T_> Vector_<Handle_<T_>> VectorHandle(int cookie, bool optional = false)
		{
			const auto& base = VectorHandleBase(cookie, optional);
			Vector_<Handle_<T_>> retval;
			for (auto b : base)
			{
				retval.push_back(handle_cast<T_>(b));
				REQUIRE(retval.back() || !b, "Input handle has wrong type");
			}
			return retval;
		}

		const Matrix_<double>& MatrixDouble(int cookie, bool optional = false);
		const Matrix_<int>& MatrixInt(int cookie, bool optional = false);
		const Matrix_<String_>& MatrixString(int cookie, bool optional = false);
		const Matrix_<Cell_>& MatrixCell(int cookie, bool optional = false);
		const Vector_<Cell_>& VectorCell(int cookie, bool optional = false);
		const Dictionary_& Dict(int cookie, bool optional = false);

		// Enum inputs arrive as a string cookie.  Machinist emits get.String<Enum_>(cookie).
		template<class T_> T_ String(int cookie, bool optional = false)
		{
			if (cookie == 0)
			{
				REQUIRE(optional, "Missing input");
				return T_();
			}
			used_.push_back(cookie);	// destructor drops it if the copy throws
			const char* owned = CookieStringCopy(cookie);	// drops the cookie
			String_ s(owned ? owned : "");
			CookieFreeString(const_cast<char*>(owned));
			if (optional && s.empty())
				return T_();
			return T_(s);
		}
	};

	class BASE_EXPORT Output_
	{
		double id_;
		Vector_<int> cookies_;
	public:
		Output_();
		~Output_();
		double Finalize();
		double Error(const char* what, const char* arg);

		void AppendBase(const Handle_<Storable_>& h);
		template<class T_> void Append(const Handle_<T_>& h) { AppendBase(handle_cast<Storable_>(h)); }
		void Append(double v);
		void Append(int v);
		void Append(bool v);
		void Append(const String_& s);
		void Append(const Date_& d);
		void Append(const DateTime_& d);
		void Append(const Cell_& c);
		void Append(const Vector_<>& v);
		void Append(const Vector_<int>& v);
		void Append(const Vector_<String_>& v);
		void Append(const Vector_<Cell_>& v);
		void Append(const Vector_<Handle_<Storable_>>& v);
		template<class T_> void Append(const Vector_<Handle_<T_>>& v)
		{
			Vector_<Handle_<Storable_>> base;
			for (auto h : v)
				base.push_back(handle_cast<Storable_>(h));
			Append(base);
		}
		void Append(const Matrix_<double>& m);
		void Append(const Matrix_<int>& m);
		void Append(const Matrix_<String_>& m);
		void Append(const Matrix_<Cell_>& m);
	};
}
