
#pragma once
#include "Export.h"

namespace Matrix
{
	BASE_EXPORT Vector_<> Vols
		(const Matrix_<>& cov,
		 Matrix_<>* corr = nullptr);	// this routine also works in-place, when corr==&cov

	BASE_EXPORT void Multiply
		(const Matrix_<>& left,
		 const Matrix_<>& right,
		 Matrix_<>* result);

	BASE_EXPORT void Multiply
		(const Matrix_<>& left,
		 const Vector_<>& right,
		 Vector_<>* result);

	BASE_EXPORT void Multiply
		(const Vector_<>& left,
		 const Matrix_<>& right,
		 Vector_<>* result);

	BASE_EXPORT double WeightedInnerProduct
		(const Vector_<>& left,
		 const Matrix_<double>& w,
		 const Vector_<>& right);

	BASE_EXPORT void AddJSquaredToUpper
		(const Matrix_<>& j,
		 Matrix_<>* h);
}

