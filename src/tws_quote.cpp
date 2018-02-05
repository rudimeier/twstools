/*** tws_quote.cpp -- TWS real time quotes
 *
 * Copyright (C) 2012-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_quote.h"
#include <twsapi/EWrapper.h>
#include <string.h>


Quote::Quote() :
	val(new double[IB::NOT_SET]),
	stamp(new int64_t[IB::NOT_SET])
{
	memset(val, 0, sizeof(val) * IB::NOT_SET);
	memset(stamp, 0, sizeof(stamp) * IB::NOT_SET);
}

Quote::Quote( const Quote& other ) :
	val(new double[IB::NOT_SET]),
	stamp(new int64_t[IB::NOT_SET])
{
	memcpy(val, other.val, sizeof(val) * IB::NOT_SET);
	memcpy(stamp, other.stamp, sizeof(stamp) * IB::NOT_SET);
}

Quote& Quote::operator=( const Quote& other )
{
	memcpy(val, other.val, sizeof(val) * IB::NOT_SET);
	memcpy(stamp, other.stamp, sizeof(stamp) * IB::NOT_SET);
	return *this;
}

Quote::~Quote()
{
	delete[] val;
	delete[] stamp;
}
