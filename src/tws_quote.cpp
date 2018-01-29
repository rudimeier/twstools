/*** tws_quote.cpp -- TWS real time quotes
 *
 * Copyright (C) 2012-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_quote.h"
#include <twsapi/twsapi_config.h>
#include <twsapi/EWrapper.h>
#include <string.h>

#ifndef TWSAPI_NO_NAMESPACE
namespace IB {
}
using namespace IB;
#endif

Quote::Quote() :
	val(new double[NOT_SET]),
	stamp(new int64_t[NOT_SET])
{
	memset(val, 0, sizeof(val) * NOT_SET);
	memset(stamp, 0, sizeof(stamp) * NOT_SET);
}

Quote::Quote( const Quote& other ) :
	val(new double[NOT_SET]),
	stamp(new int64_t[NOT_SET])
{
	memcpy(val, other.val, sizeof(val) * NOT_SET);
	memcpy(stamp, other.stamp, sizeof(stamp) * NOT_SET);
}

Quote& Quote::operator=( const Quote& other )
{
	memcpy(val, other.val, sizeof(val) * NOT_SET);
	memcpy(stamp, other.stamp, sizeof(stamp) * NOT_SET);
	return *this;
}

Quote::~Quote()
{
	delete[] val;
	delete[] stamp;
}
