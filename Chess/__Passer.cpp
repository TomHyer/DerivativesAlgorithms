
#include "../Base/__Platform.h"
#include <array>
#include "../Base/Strict.h"

#include "../Base/Matrix.h"
#include "PasserEval.h"

namespace
{
    template<class T_> Matrix_<int> UnpackToMatrix(const T_& src)
    {
        Matrix_<int> retval(static_cast<int>(src.size()), 4);
        for (int iRow = 0; iRow < src.size(); ++iRow)
        {
            auto row = retval.Row(iRow);
            row[0] = Pick16<1>(src[iRow]);
            row[1] = Pick16<2>(src[iRow]);
            row[2] = Pick16<3>(src[iRow]);
            row[3] = Pick16<4>(src[iRow]);
        }
        return retval;
    }

/*IF--------------------------------------------------------------------------
public Passer_Values
    Compute passer values in Gull or Roc style
&inputs
params is integer[][]
    &$.Rows() * $.Cols() == 12
    Input parameters; 1x12 for Roc, 3x4 for Gull
&outputs
values is integer[][]
    Values by rank and phase
-IF-------------------------------------------------------------------------*/

    void Passer_Values
        (const Matrix_<int>& params,
         Matrix_<int>* values)
    {
        std::array<sint64, 8> packed;
        NOTICE(params.Cols());
        switch (params.Cols())
        {
        case 1:
            packed = PasserValues::InitRoc(params.Col(0));
            break;
        case 12:
            packed = PasserValues::InitRoc(params.Row(0));
            break;
        case 4:
            packed = PasserValues::InitGull(params);
            break;

        default:
            THROW("Unexpected column size");
        }
        // unpack
        *values = UnpackToMatrix(packed);
    }
}   // leave local

#include "MG_Passer_Values_public.inc"
