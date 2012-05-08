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


buy_sell_oid::buy_sell_oid() :
	sell_oid(0),
	buy_oid(0)
{
}


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
	if( fuck <= 30 ) {
		return;
	}

	DEBUG_PRINTF( "strat, adjust orders" );

	const MktDataTodo &mtodo = twsdo.workTodo->getMktDataTodo();
	for( int i=0; i < mtodo.mktDataRequests.size(); i++ ) {
		PlaceOrder pO;
		pO.contract = mtodo.mktDataRequests[i].ibContract;
		pO.order.orderType = "LMT";
		pO.order.totalQuantity = pO.contract.secType == "CASH" ? 25000 : 1;
		const char *symbol = pO.contract.symbol.c_str();

		/* here we also initialize zero order ids */
		buy_sell_oid &oids = map_data_order[i];
		if( twsdo.p_orders.find(oids.buy_oid) == twsdo.p_orders.end() ) {
			/* new buy order */
			DEBUG_PRINTF( "strat, new buy order %s", symbol );
			oids.buy_oid = twsdo.fetch_inc_order_id();
			pO.orderId = oids.buy_oid;
			pO.order.action = "BUY";
			pO.order.lmtPrice = twsdo.quotes->at(i).val[IB::BID] - 0.1;
			twsdo.workTodo->placeOrderTodo()->add(pO);
		} else {
			/* modify buy order */
			DEBUG_PRINTF( "strat, modify buy order %s", symbol );
		}
		if( twsdo.p_orders.find(oids.sell_oid) == twsdo.p_orders.end() ) {
			/* new sell order */
			DEBUG_PRINTF( "strat, new sell order %s", symbol );
			oids.sell_oid = twsdo.fetch_inc_order_id();
			pO.orderId = oids.sell_oid;
			pO.order.action = "SELL";
			pO.order.lmtPrice = twsdo.quotes->at(i).val[IB::ASK] + 0.1;
			twsdo.workTodo->placeOrderTodo()->add(pO);
		} else {
			/* modify sell order */
			DEBUG_PRINTF( "strat, modify sell order %s", symbol );
		}
	}
	DEBUG_PRINTF( "strat, place/modify %d orders",
		twsdo.workTodo->placeOrderTodo()->countLeft());
	assert( mtodo.mktDataRequests.size() == map_data_order.size() );
}