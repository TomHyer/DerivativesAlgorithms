#pragma once

#include <array>

constexpr int MatWQ = 1;
constexpr int MatBQ = 3 * MatWQ;
constexpr int MatWR = 3 * MatBQ;
constexpr int MatBR = 3 * MatWR;
constexpr int MatWL = 3 * MatBR;
constexpr int MatBL = 2 * MatWL;
constexpr int MatWD = 2 * MatBL;
constexpr int MatBD = 2 * MatWD;
constexpr int MatWN = 2 * MatBD;
constexpr int MatBN = 3 * MatWN;
constexpr int MatWP = 3 * MatBN;
constexpr int MatBP = 9 * MatWP;
constexpr int TotalMat = 2 * (MatWQ + MatBQ) + MatWL + MatBL + MatWD + MatBD + 2 * (MatWR + MatBR + MatWN + MatBN) + 8 * (MatWP + MatBP) + 1;
constexpr int FlagUnusualMaterial = 1 << 30;

// indexed by piece type, see Piece.h
constexpr std::array<int, 16> MatCode = { 0, 0, MatWP, MatBP, MatWN, MatBN, MatWL, MatBL, MatWD, MatBD, MatWR, MatBR, MatWQ, MatBQ, 0, 0 };

