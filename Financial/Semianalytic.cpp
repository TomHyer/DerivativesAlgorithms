
#include "Base/Platform.h"
#include "Semianalytic.h"
#include <map>
#include <functional>
#include "Base/Strict.h"

#include "Base/Exceptions.h"
#include "Base/Audit.h"
#include "Valuation.h"

namespace
{
	typedef std::multimap<int, Handle_<SemianalyticPricer_>, std::greater<int>> pricers_t;
	pricers_t& XThePricers()
	{
		RETURN_STATIC(pricers_t);
	}
}

void Semianalytic::Register(SemianalyticPricer_* orphan, int priority)
{
	Handle_<SemianalyticPricer_> h(orphan);
	XThePricers().insert(make_pair(priority, h));
}

Vector_<pair<String_, double> > Semianalytic::Value
	(_ENV, const Trade_& trade,
	 const Model_& model,
	 const ValuationParameters_* params)
{
	const auto& candidates = XThePricers();
	Vector_<pair<String_, double> > retval;
	for (auto pc : candidates)
	{
		if (pc.second->Attempt(_env, trade, model, params, &retval))
			return retval;
		retval.clear();
	}
	THROW("Can't find any pricer");
}

namespace
{
	class ReevaluateSemianalytic_ : public ReEvaluator_
	{
		AuditorImp_ auditor_;
		const Trade_& trade_;
		const ValuationParameters_* params_;

   public:
		ReevaluateSemianalytic_
			(_ENV, const Trade_& trade,
			 const Model_& model,
			 const ValuationParameters_* params)
		: 
		trade_(trade), 
		params_(params)
		{
         ENV_ADD(auditor_);
			baseVals_ = Semianalytic::Value(_env, trade, model, params);
			auditor_.mode_ = AuditorImp_::SHOWING;
		}

		Vector_<pair<String_, double> > Values
			(_ENV, const Model_* bumped_model = nullptr)
      const override
		{
         ENV_ADD(auditor_);
			return bumped_model
					? Semianalytic::Value(_env, trade_, *bumped_model, params_)
					: baseVals_;
		}
	};
}

