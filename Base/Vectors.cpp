
#include "Platform.h"
#include "Vectors.h"
#include "Strict.h"

Vector_<int> Vector::UpTo(int n)
{
	auto iota = std::views::iota(0, n);
	return Vector_<int>(iota.begin(), iota.end());
}
