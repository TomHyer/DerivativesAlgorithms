
#include "Platform.h"
#include "BuySell.h"
#include "Strict.h"

#include "Exceptions.h"

#include "MG_BuySell_enum.inc"

int BuySell_::Sign() const
{
	return val_ == Value_::BUY ? 1 : -1;
}

BuySell_ BuySell_::operator-() const
{
	return val_ == Value_::BUY ? Value_::SELL : Value_::BUY;
}
