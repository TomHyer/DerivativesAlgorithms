
#include "../Base/__Platform.h"
#include <array>
#include "../Base/Strict.h"

#include "Material.h"
#include "Eval.h"
#include "Pack.h"

namespace
{
    Matrix_<int> UnpackToMatrix(const Vector_<packed_t>& src)
    {
        Matrix_<int> retval(src.size(), 4);
        for (int ii = 0; ii < src.size(); ++ii)
        {
            retval(ii, 0) = Pick16<1>(src[ii]);
            retval(ii, 1) = Pick16<2>(src[ii]);
            retval(ii, 2) = Pick16<3>(src[ii]);
            retval(ii, 3) = Pick16<4>(src[ii]);
        }
        return retval;
    }

/*IF--------------------------------------------------------------------------
public View_MvvLva
    Displays MvvLva table
&inputs
&optional
rshift is integer (26)
    Amount to shift right
&outputs
mvvlva is integer[][]
    MvvLva scores
-IF-------------------------------------------------------------------------*/

    void View_MvvLva
        (int rshift,
        Matrix_<int>* mvv_lva)
    {
        mvv_lva->Resize(8, 8);
        for (int ii = 0; ii < 8; ++ii)
            for (int jj = 0; jj < 8; ++jj)
                (*mvv_lva)(ii, jj) = MvvLva[ii][jj] >> rshift;
    }
}   // leave local

#include "MG_View_MvvLva_public.inc"
