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

/**
 * Return min tick price for a given contract.
 */
double Strat::min_tick(const IB::Contract& c)
{
	assert( c.conId != 0 );
	assert( twsdo.con_details.find( c.conId ) != twsdo.con_details.end() );
	double min_tick = twsdo.con_details[c.conId]->minTick ;
	assert(min_tick > 0.0);
	long priceMagnifier = twsdo.con_details[c.conId]->priceMagnifier;
	assert(priceMagnifier > 0);
	return min_tick * priceMagnifier;
}

/**
 * Place or modify buy and sell orders for a single contract. Quote should
 * valid bid and ask.
 */
void Strat::adjust_order( const IB::Contract& c, const Quote& quote,
	buy_sell_oid& oids )
{
	PlaceOrder pO;
	pO.contract = c;
	pO.order.orderType = "LMT";
	pO.order.totalQuantity = pO.contract.secType == "CASH" ? 25000 : 1;
	const char *symbol = pO.contract.symbol.c_str();

	double quote_dist = 1 * min_tick(c);

	double lmt_buy = quote.val[IB::BID] - quote_dist;
	double lmt_sell = quote.val[IB::ASK] + quote_dist;

	if( twsdo.p_orders.find(oids.buy_oid) == twsdo.p_orders.end() ) {
		/* new buy order */
		DEBUG_PRINTF( "strat, new buy order %s", symbol );
		oids.buy_oid = twsdo.fetch_inc_order_id();
		pO.orderId = oids.buy_oid;
		pO.order.action = "BUY";
		pO.order.lmtPrice = lmt_buy;
		twsdo.workTodo->placeOrderTodo()->add(pO);
	} else {
		/* modify buy order */
		PacketPlaceOrder *ppo = twsdo.p_orders[oids.buy_oid];
		const PlaceOrder &po = ppo->getRequest();
		if( po.order.lmtPrice != lmt_buy ) {
			DEBUG_PRINTF( "strat, modify buy order %s", symbol );
			pO.orderId = oids.buy_oid;
			pO.order.action = "BUY";
			pO.order.lmtPrice = lmt_buy;
			twsdo.workTodo->placeOrderTodo()->add(pO);
		}
	}
	if( twsdo.p_orders.find(oids.sell_oid) == twsdo.p_orders.end() ) {
		/* new sell order */
		DEBUG_PRINTF( "strat, new sell order %s", symbol );
		oids.sell_oid = twsdo.fetch_inc_order_id();
		pO.orderId = oids.sell_oid;
		pO.order.action = "SELL";
		pO.order.lmtPrice = lmt_sell;
		twsdo.workTodo->placeOrderTodo()->add(pO);
	} else {
		/* modify sell order */
		PacketPlaceOrder *ppo = twsdo.p_orders[oids.sell_oid];
		const PlaceOrder &po = ppo->getRequest();
		if( po.order.lmtPrice != lmt_sell ) {
			DEBUG_PRINTF( "strat, modify sell order %s", symbol );
			pO.orderId = oids.sell_oid;
			pO.order.action = "SELL";
			pO.order.lmtPrice = lmt_sell;
			twsdo.workTodo->placeOrderTodo()->add(pO);
		}
	}
}

void Strat::adjustOrders()
{
	DEBUG_PRINTF( "strat, adjust orders" );

	const MktDataTodo &mtodo = twsdo.workTodo->getMktDataTodo();
	for( int i=0; i < mtodo.mktDataRequests.size(); i++ ) {
		const IB::Contract &contract = mtodo.mktDataRequests[i].ibContract;
		const Quote &quote = twsdo.quotes->at(i);

		/* here we also initialize zero order ids */
		buy_sell_oid &oids = map_data_order[i];

		if( quote.val[IB::BID] > 0.0 && quote.val[IB::ASK] > 0.0 ) {
			adjust_order( contract, quote, oids );
		} else {
			/* invalid quotes, TODO cleanup, cancel, whatever */
		}
	}
	DEBUG_PRINTF( "strat, place/modify %d orders",
		twsdo.workTodo->placeOrderTodo()->countLeft());
	assert( mtodo.mktDataRequests.size() == map_data_order.size() );
}