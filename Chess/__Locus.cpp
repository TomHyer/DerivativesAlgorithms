
#include "../Base/__Platform.h"
#include <array>
#include "../Base/Strict.h"

#include "Locus.h"

namespace
{
    Matrix_<int> UnpackToMatrix(const uint64& src)
    {
        Matrix_<int> retval(8, 8);
        for (int iRow = 0; iRow < 8; ++iRow)
            for (int iCol = 0; iCol < 8; ++iCol)
                retval(iRow, iCol) = HasBit(src, 8 * iRow + iCol) ? 1 : 0;
        return retval;
    }

/*IF--------------------------------------------------------------------------
public Locus_Roc_2
    Compute mobility values using post-2022 Roc
&inputs
which is integer
    1 for knight, 2 for bishop, etc
params is number[]
    Input parameters
k_loc is integer
    &$ >= 0 && $ < 64
    Location to view
&optional
n_locus is integer (22)
    &$ > 0
    Size of the king locus
&outputs
locus is integer[][]
    Locus contents
-IF-------------------------------------------------------------------------*/

    void Locus_Roc_2
        (int which,
         const Vector_<>& weights,
         int k_loc,
         int n_locus,
         Matrix_<int>* locus)
    {
        uint64 temp;
        switch (which)
        {
        default:
        case 1:
            REQUIRE(weights.size() == 3, "Knight takes 3 weights");
            temp = MakeLocus([=](int j) { return Locus::MinorDist_(NAtt, weights[0], weights[1], weights[2])(k_loc, j); }, n_locus);
            break;

        case 2:
            REQUIRE(weights.size() == 3, "Bishop takes 3 weights");
            temp = MakeLocus([=](int j) { return Locus::MinorDist_(BMask, weights[0], weights[1], weights[2])(k_loc, j); }, n_locus);
            break;

        case 3:
            REQUIRE(weights.size() == 2, "Rook takes 2 weights");
            temp = MakeLocus([=](int j) { return Locus::RDist_(weights[0], weights[1])(k_loc, j); }, n_locus);
            break;

        case 4:
            REQUIRE(weights.size() == 3, "Queen takes 3 weights");
            temp = MakeLocus([=](int j) { return Locus::MinorDist_(QMask, weights[0], weights[1], weights[2])(k_loc, j); }, n_locus);
            break;

        case 5:
            // ignore weights
            temp = MakeLocus([=](int j) { return Locus::KDist(k_loc, j); }, n_locus);
            break;

        case 6:
            REQUIRE(weights.size() == 2, "Rook still takes 2 weights");
            temp = MakeLocus([=](int j) { return Locus::MinorDist_(RMask, weights[0], weights[1])(k_loc, j); }, n_locus);
            break;
        }
        *locus = UnpackToMatrix(temp);
        (*locus)(RankOf(k_loc), FileOf(k_loc)) = -1;
    }
}   // leave local

#include "MG_Locus_Roc_2_public.inc"
