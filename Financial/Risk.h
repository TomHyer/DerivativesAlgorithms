
// risk tasks, creating output reports

#pragma once

#include "Base/Storable.h"
class Portfolio_;
class Model_;
class Report_;

class RiskRun_ : public Storable_
{
public:
	virtual Report_* Run
		(const Portfolio_& portfolio,
		 const Model_& model)
	const = 0;
};
