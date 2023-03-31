
#include "../Base/__Platform.h"
#include <array>
#include "../Base/Strict.h"

#include "Pack.h"
#include "Mobility.h"

extern "C" __declspec(dllexport) int xlAutoOpen(void)
{
    return Excel::AutoOpen();
}


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
public Mobility_Roc
    Compute mobility values using Gull recipe
&inputs
gull_like is boolean
    Whether to use Gull-derived formulas (Roc 1.6 or earlier)
params is integer[]
    &$.size() == 12
    Input parameters (Gull formulas but 4 phases)
n_max is integer
    &$ > 0
    Maximum number of moves for the piece
&optional
n_locus is integer (22)
    &$ > 0
    Size of the king locus
&outputs
mob_0 is integer[][]
    Non-locus mobility values, unpacked
mob_1 is integer[][]
    Locus mobility values, unpacked
-IF-------------------------------------------------------------------------*/

    void Mobility_Roc
        (bool gull_like,
         const Vector_<int>& weights,
         int n_max,
         int n_locus,
         Matrix_<int>* mob_0,
         Matrix_<int>* mob_1)
    {
        std::array<int, 12> w;
        Copy(weights, &w);

        Vector_<Vector_<packed_t>> mob(2);
        mob[0].resize(n_max + 1);
        mob[1].resize(n_max + 1);
        if (gull_like)
            InitMobility_Gull(w, n_locus, &mob);
        else
            InitMobility_Roc_1_7(w, n_locus, &mob);

        *mob_0 = UnpackToMatrix(mob[0]);
        *mob_1 = UnpackToMatrix(mob[1]);
    }
}   // leave local

#include "MG_Mobility_Roc_public.inc"
