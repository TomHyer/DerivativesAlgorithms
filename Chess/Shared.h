#pragma once

#include "Magic.h"
#include "Eval.h"
#include "Material.h"

struct CommonData_
{
	Magic::Magic_ magic_;
	std::array<GMaterial, TotalMat> Material;
	std::array<std::array<std::array<uint64, 64>, 64>, 2> Kpk;
	std::array<std::array<uint64, 64>, 64> Between, FullLine;
	std::array<packed_t, 16 * 64> PstVals;
	std::array<std::array<uint64, 64>, 16> PieceKey;
	std::array<std::array<int, 16>, 16> MvvLva;  // [piece][capture]
	std::array<int, 256> SpanWidth, SpanGap;
	std::array<std::array<uint64, 64>, 2> BishopForward, PCone, PSupport;
	std::array<uint64, 64> LocusN, LocusB, LocusR, LocusQ, LocusK;
	std::array<std::array<packed_t, 28>, 2> MobQueen;
	std::array<uint8, 256> PieceFromChar;
	std::array<std::array<packed_t, 15>, 2> MobBishop, MobRook;
	std::array<std::array<packed_t, 9>, 2> MobKnight;
	std::array<uint64, 16> CastleKey;
	std::array<uint64, 8> EPKey;
	std::array<uint64, 8> PIsolated;
	std::array<packed_t, 8> PasserCandidate;
	//std::array<packed_t, 8> PasserGeneral, PasserBlocked, PasserFree, PasserSupported, PasserProtected, PasserConnected, PasserOutside, PasserClear;
	std::array<std::array<sint16, 8>, 3> Shelter;
	std::array<uint8, 16> LogDist;
	std::array<sint16, 8> PasserAtt, PasserDef, PasserAttLog, PasserDefLog;
	std::array<sint16, 4> StormBlocked, StormShelterAtt, StormConnected, StormOpen, StormFree;
	uint64 TurnKey;
};

