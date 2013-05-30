/*** tws_query.cpp -- structs for IB/API requests
 *
 * Copyright (C) 2010-2013 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 *
 * This file is part of twstools.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/

#include "tws_query.h"

#include <stdio.h>


const IB::Contract& ContractDetailsRequest::ibContract() const
{
	return _ibContract;
}

bool ContractDetailsRequest::initialize( const IB::Contract& c )
{
	_ibContract = c;
	return true;
}




HistRequest::HistRequest()
{
	useRTH = 0;
	formatDate = 0; // set invalid (IB allows 1 or 2)
}


bool HistRequest::initialize( const IB::Contract& c, const std::string &e,
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
		ibContract.expiry.c_str(),
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
