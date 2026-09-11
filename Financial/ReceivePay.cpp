
#include "Base/Platform.h"
#include "ReceivePay.h"
#include "Base/Strict.h"

#include "Base/Vectors.h"
#include "Base/Exceptions.h"
#include "MG_RecPay_enum.inc"

int RecPay_::RecSign() const
{
	return val_ == Value_::REC ? 1 : -1;
}

RecPay_ RecPay_::operator-() const
{
	return val_ == Value_::REC ? Value_::PAY : Value_::REC;
}

