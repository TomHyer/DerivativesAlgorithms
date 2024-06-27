#pragma once

#include "Array.h"
#include "MaterialImp.h"

constexpr int MatCodeWQ = 1;
constexpr int MatCodeBQ = 3 * MatCodeWQ;
constexpr int MatCodeWR = 3 * MatCodeBQ;
constexpr int MatCodeBR = 3 * MatCodeWR;
constexpr int MatCodeWL = 3 * MatCodeBR;
constexpr int MatCodeBL = 2 * MatCodeWL;
constexpr int MatCodeWD = 2 * MatCodeBL;
constexpr int MatCodeBD = 2 * MatCodeWD;
constexpr int MatCodeWN = 2 * MatCodeBD;
constexpr int MatCodeBN = 3 * MatCodeWN;
constexpr int MatCodeWP = 3 * MatCodeBN;
constexpr int MatCodeBP = 9 * MatCodeWP;
constexpr int TotalMat = 9 * MatCodeBP;
constexpr int FlagUnusualMaterial = 1 << 30;

// indexed by piece type, see Piece.h
constexpr std::array<int, 16> MatCode = { 0, 0, MatCodeWP, MatCodeBP, MatCodeWN, MatCodeBN, MatCodeWL, MatCodeBL, MatCodeWD, MatCodeBD, MatCodeWR, MatCodeBR, MatCodeWQ, MatCodeBQ, 0, 0 };

constexpr std::array<std::array<int, 16>, 16> MvvLva = Calc::MakeMvvLva();


constexpr std::array<int, 16> MvvLvaPromotionCap = make_array<16>([=](int capture) { return MvvLva[(capture <= WhiteRook) ? WhiteRook : (capture >= WhiteQueen ? WhiteKing : WhiteKnight)][BlackQueen]; });
constexpr std::array<int, 16> MvvLvaPromotionKnightCap = MvvLva[WhiteKing];
