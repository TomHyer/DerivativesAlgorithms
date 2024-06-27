
#include "Base/__Platform.h"
#include "KillerSudoku.h"
#include "Base/Strict.h"


extern "C" __declspec(dllexport) int xlAutoOpen(void)
{
    return Excel::AutoOpen("Sudoku");
}

namespace
{
/*IF--------------------------------------------------------------------------
public Killer_Sudoku
    Create a matrix describing a puzzle
&inputs
entropy is number
    A measure of difficulty; 150 is high
seed is integer
    &$ > 0
    Seed for random generation
minimal is boolean
    Show fewer outputs
&outputs
puzzle is cell[][]
    The puzzle
-IF-------------------------------------------------------------------------*/

    void Killer_Sudoku
        (double entropy,
         int seed,
         bool minimal,
         Matrix_<Cell_>* puzzle)
    {
        *puzzle = KS(entropy, seed, minimal);
    }
}  // leave local

#include "MG_Killer_Sudoku_public.inc"
