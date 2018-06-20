/*** tws_query.cpp -- structs for IB/API requests
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_query.h"

#include <twsapi/twsapi_config.h>

#include <stdio.h>

#if TWSAPI_IB_VERSION_NUMBER < 97200
# define lastTradeDateOrContractMonth expiry
#endif

const Contract& ContractDetailsRequest::ibContract() const
{
	return _ibContract;
}

bool ContractDetailsRequest::initialize( const Contract& c )
{
	_ibContract = c;
	return true;
}




HistRequest::HistRequest()
{
	useRTH = 0;
	formatDate = 0; // set invalid (IB allows 1 or 2)
}


bool HistRequest::initialize( const Contract& c, const std::string &e,
	const std::string &d, const std::string &b,
	const std::string &w, int u, int f )
{
	ibContract = c;
	endDateTime = e;
	durationStr = d;
	barSizeSetting = b;
	whatToShow = w;
	useRTH = u;
	formatDate = f;
	return true;
}


std::string HistRequest::toString() const
{
	char buf_c[512];
	char buf_a[1024];
	snprintf( buf_c, sizeof(buf_c), "%s\t%s\t%s\t%s\t%s\t%g\t%s",
		ibContract.symbol.c_str(),
		ibContract.secType.c_str(),
		ibContract.exchange.c_str(),
		ibContract.currency.c_str(),
		ibContract.lastTradeDateOrContractMonth.c_str(),
		ibContract.strike,
		ibContract.right.c_str() );

	snprintf( buf_a, sizeof(buf_a), "%s\t%s\t%s\t%s\t%d\t%d\t%s",
		endDateTime.c_str(),
		durationStr.c_str(),
		barSizeSetting.c_str(),
		whatToShow.c_str(),
		useRTH,
		formatDate,
		buf_c );

	return std::string(buf_a);
}




AccStatusRequest::AccStatusRequest() :
	subscribe(true)
{
}




PlaceOrder::PlaceOrder() :
	orderId(0),
	time_sent(0)
{
}




MktDataRequest::MktDataRequest() :
	snapshot(false)
{
}

OptParamsRequest::OptParamsRequest()
{
}