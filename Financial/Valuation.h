
// capture and repeat valuations
	// use this for all hedge computation

#pragma once

#include "../Base/Vectors.h"
#include "../Base/Strings.h"
#include "../Base/Environment.h"

class Model_;

class ReEvaluator_ : noncopyable
{
protected:
	Vector_<pair<String_, double> > baseVals_;
public:
	virtual ~ReEvaluator_();
	virtual Vector_<pair<String_, double> > Values
		(_ENV, const Model_* bumped_model = nullptr)
	const = 0;
};
