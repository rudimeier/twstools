/*** tws_strat.cpp -- TWS strategy module
 *
 * Copyright (C) 2012 Ruediger Meier
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

#include "tws_strat.h"
#include "tws_query.h"
#include "tws_meta.h"
#include "twsdo.h"
#include "debug.h"
#include <assert.h>

Strat::Strat( TwsDL& _twsdo) :
	twsdo(_twsdo)
{
}

Strat::~Strat()
{
}

void Strat::adjustOrders()
{
	static int fuck = -1;
	fuck++;
	if( fuck <= 30 || twsdo.p_orders.size() > 0 ) {
		return;
	}

	DEBUG_PRINTF( "Adjust orders." );
	PlaceOrder pO;
	int i;

	const MktDataTodo &mtodo = twsdo.workTodo->getMktDataTodo();
	for( int i=0; i < mtodo.mktDataRequests.size(); i++ ) {
		pO.contract = mtodo.mktDataRequests[i].ibContract;
		pO.order.orderType = "LMT";
		pO.order.action = "BUY";
		pO.order.lmtPrice = twsdo.quotes->at(i).val[IB::BID] - 0.1;
		pO.order.totalQuantity = pO.contract.secType == "CASH" ? 25000 : 1;
		twsdo.workTodo->placeOrderTodo()->add(pO);
	}
	DEBUG_PRINTF( "Adjust orders. %d",
		twsdo.workTodo->placeOrderTodo()->countLeft());
}