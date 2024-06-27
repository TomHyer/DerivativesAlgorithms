#pragma once

#include "Magic.h"
#include "Eval.h"
#include "Material.h"
#include "SMP.h"
#include "Board.h"
#include "State.h"
#include "Time.h"

struct CommonData_
{
	Magic::Magic_<B_MAGIC_SIZE> BMagic_;
	Magic::Magic_<R_MAGIC_SIZE> RMagic_;
	std::array<GMaterial, TotalMat> Material;
	std::array<std::array<std::array<uint64, 64>, 64>, 2> Kpk;
	std::array<std::array<uint64, 64>, 16> PieceKey;
	std::array<std::array<uint64, 64>, 2> LocusN, LocusB, LocusR, LocusQ, LocusK;
	std::array<std::array<packed_t, 28>, 2> MobQueen;
	std::array<std::array<packed_t, 15>, 2> MobBishop, MobRook;
	std::array<std::array<packed_t, 9>, 2> MobKnight;
	std::array<uint64, 16> CastleKey;
	std::array<uint64, 8> EPKey;
	std::array<std::array<sint16, 8>, 3> Shelter;
	std::array<uint8, 16> LogDist;
	std::array<sint16, 8> PasserAtt, PasserDef, PasserAttLog, PasserDefLog;
	uint64 TurnKey;
};

#ifdef SINGLE_THREAD
struct WorkerList_ 
{
	void Init() {}
	void Insert(int) {}
	void Remove(int) {}
	bool Empty() { return true; }
};
#else
struct WorkerList_
{
	struct Record_
	{
		int pvs_, next_;
	};
	std::array<Record_, maxNumThreads + 1> vals_;
	void Init()
	{
		for (int jj = 0; jj <= maxNumThreads; ++jj)
			vals_[jj] = { jj, jj };
	}
	void Insert(int iw);
	void Remove(int iw);
	bool Empty() const
	{
		return vals_[maxNumThreads].pvs_ == maxNumThreads;
	}
};

inline void WorkerList_::Insert(int iw)
{
	assert(iw >= 0 && iw < maxNumThreads);
	assert(vals_[iw].pvs_ == iw && vals_[iw].next_ == iw);	// double insertion
	VSAY("Adding worker " + Str(iw) + "\n");
	int last = vals_[maxNumThreads].pvs_;
	vals_[iw].pvs_ = last;
	vals_[iw].next_ = maxNumThreads;
	vals_[last].next_ = iw;
	vals_[maxNumThreads].pvs_ = iw;
}
inline void WorkerList_::Remove(int iw)
{
	assert(iw >= 0 && iw < maxNumThreads);
	assert(vals_[iw].pvs_ != iw && vals_[iw].next_ != iw);	// double deletion
	VSAY("Removing worker " + Str(iw) + "\n");
	vals_[vals_[iw].pvs_].next_ = vals_[iw].next_;
	vals_[vals_[iw].next_].pvs_ = vals_[iw].pvs_;
	vals_[iw].pvs_ = vals_[iw].next_ = iw;
	//if (Empty()) VSAY("No more workers\n");
}
#endif

struct SharedInfo_;
struct Progress_
{
	uint8 count_;	// packed W,B
	constexpr Progress_() : count_(0) {}
	constexpr Progress_(uint8 nw, uint8 nb) : count_((nw - 1) << 4 | (nb - 1)) {}
	bool Reachable(SharedInfo_* shared) const;	// implementation after SharedInfo_ is declared
};

struct SharedSearchState_
{
	bool stopAll_;
	uint16_t date_;
	bool ponder_;
	int depth_;
	bool firstMove_;
	score_t pvsScore_;
};
SharedSearchState_ TheShare;

struct SharedInfo_
{
	SharedSearchState_ search_;

	WorkerList_ working;
	Progress_ rootProgress;
	mutex_t mutex = NULL;
	mutex_t outMutex = NULL;
	event_t goEvent = NULL;
	int rootDepth;
	int depthLimit;
	time_point startTime;
	uint64_t softTimeLimit;
	uint64_t hardTimeLimit;
	Gull::Board_ rootBoard;
	PlyState_ rootData;
	struct {
		int depth, value, move, worker;
		bool failLow;
	} best;
	uint64_t rootStack[1024];
	unsigned rootSp;
};

inline bool Progress_::Reachable(SharedInfo_* shared) const
{
	assert(shared);
	if ((count_ >> 4) > (shared->rootProgress.count_ >> 4))
		return false;
	if ((count_ & 0xF) > (shared->rootProgress.count_ & 0xF))
		return false;
	return true;
}

