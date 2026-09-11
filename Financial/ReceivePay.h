
// receive/pay flag for FI and FX trades

#pragma once

#include "Base/Strings.h"
#include "Export.h"

/*IF--------------------------------------------------------------------------
enumeration RecPay
	Identifies receive/pay legs
switchable
alternative REC RECEIVE
alternative PAY
method int RecSign() const;
method RecPay_ operator-() const;
-IF-------------------------------------------------------------------------*/
#include "MG_RecPay_enum.h"
