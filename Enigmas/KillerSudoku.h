#pragma once

#include "Base/Matrix.h"
#include "Base/Cell.h"

Matrix_<Cell_> KS(double entropy, int seed, bool minimal);
