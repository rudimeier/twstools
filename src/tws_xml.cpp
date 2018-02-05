/*** tws_xml.cpp -- conversion between IB/API structs and xml
 *
 * Copyright (C) 2011-2013 Ruediger Meier
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

#include "tws_xml.h"

#include "debug.h"
#include "tws_meta.h"
#include "tws_query.h"
#include "tws_util.h"
#include "config.h"

#include <twsapi/twsapi_config.h>
#include <twsapi/Contract.h>
#include <twsapi/Execution.h>
#include <twsapi/Order.h>
#include <twsapi/OrderState.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#if defined HAVE_MALLOC_TRIM
# include <malloc.h>
#endif


#define ADD_CHILD_TAGVALUELIST( _struct_, _attr_ ) \
	if( _struct_._attr_.get() != NULL ) { \
		xmlNodePtr np = xmlNewChild(ne, NULL, (const xmlChar*) #_attr_, NULL); \
		for( IB::TagValueList::const_iterator it = _struct_._attr_->begin(); \
			    it != _struct_._attr_->end(); ++it) { \
			conv_ib2xml( np, "tagValue", **it ); \
		} \
	}

#define GET_CHILD_TAGVALUELIST( _tvl_ ) \
	_tvl_ = IB::TagValueListSPtr( new IB::TagValueList); \
	for( xmlNodePtr q = p->children; q!= NULL; q=q->next) { \
		IB::TagValueSPtr tV( new IB::TagValue()); \
		if( q->type != XML_ELEMENT_NODE \
			|| (strcmp((char*) q->name, "tagValue") != 0)) { \
			continue; \
		} \
		conv_xml2ib( tV.get(), q ); \
		_tvl_->push_back(tV); \
	}


void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::ComboLeg& cl )
{
	char tmp[128];
	static const IB::ComboLeg dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (const xmlChar*)name, NULL);

	ADD_ATTR_LONG( cl, conId );
	ADD_ATTR_LONG( cl, ratio );
	ADD_ATTR_STRING( cl, action );
	ADD_ATTR_STRING( cl, exchange );
	ADD_ATTR_LONG( cl, openClose );
	ADD_ATTR_LONG( cl, shortSaleSlot );
	ADD_ATTR_STRING( cl, designatedLocation );
	ADD_ATTR_INT( cl, exemptCode );
}


void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::UnderComp& uc )
{
	char tmp[128];
	static const IB::UnderComp dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (const xmlChar*)name, NULL);

	ADD_ATTR_LONG( uc, conId );
	ADD_ATTR_DOUBLE( uc, delta );
	ADD_ATTR_DOUBLE( uc, price );
}


void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Contract& c )
{
	char tmp[128];
	static const IB::Contract dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (const xmlChar*)name, NULL);

	ADD_ATTR_LONG( c, conId );
	ADD_ATTR_STRING( c, symbol );
	ADD_ATTR_STRING( c, secType );
	ADD_ATTR_STRING( c, expiry );
	ADD_ATTR_DOUBLE( c, strike );
	ADD_ATTR_STRING( c, right );
	ADD_ATTR_STRING( c, multiplier );
	ADD_ATTR_STRING( c, exchange );
	ADD_ATTR_STRING( c, primaryExchange );
	ADD_ATTR_STRING( c, currency );
	ADD_ATTR_STRING( c, localSymbol );
	ADD_ATTR_STRING( c, tradingClass );
	ADD_ATTR_BOOL( c, includeExpired );
	ADD_ATTR_STRING( c, secIdType );
	ADD_ATTR_STRING( c, secId );
	ADD_ATTR_STRING( c, comboLegsDescrip );

	if( c.comboLegs.get() != NULL ) {
		xmlNodePtr ncl = xmlNewChild( ne, NULL, (xmlChar*)"comboLegs", NULL);
		for( IB::Contract::ComboLegList::const_iterator it
			    = c.comboLegs->begin(); it != c.comboLegs->end(); ++it) {
			conv_ib2xml( ncl, "comboLeg", **it );
		}
	}
	if( c.underComp != NULL ) {
		conv_ib2xml( ne, "underComp", *c.underComp );
	}
}


void conv_ib2xml( xmlNodePtr parent, const char* name,
	const IB::ContractDetails& cd )
{
	char tmp[128];
	static const IB::ContractDetails dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	conv_ib2xml( ne, "summary", cd.summary );

	ADD_ATTR_STRING( cd, marketName );
	ADD_ATTR_DOUBLE( cd, minTick );
	ADD_ATTR_STRING( cd, orderTypes );
	ADD_ATTR_STRING( cd, validExchanges );
	ADD_ATTR_LONG( cd, priceMagnifier );
	ADD_ATTR_INT( cd, underConId );
	ADD_ATTR_STRING( cd, longName );
	ADD_ATTR_STRING( cd, contractMonth );
	ADD_ATTR_STRING( cd, industry );
	ADD_ATTR_STRING( cd, category );
	ADD_ATTR_STRING( cd, subcategory );
	ADD_ATTR_STRING( cd, timeZoneId );
	ADD_ATTR_STRING( cd, tradingHours );
	ADD_ATTR_STRING( cd, liquidHours );
	ADD_ATTR_STRING( cd, evRule );
	ADD_ATTR_DOUBLE( cd, evMultiplier );

	// BOND values
	ADD_ATTR_STRING( cd, cusip );
	ADD_ATTR_STRING( cd, ratings );
	ADD_ATTR_STRING( cd, descAppend );
	ADD_ATTR_STRING( cd, bondType );
	ADD_ATTR_STRING( cd, couponType );
	ADD_ATTR_BOOL( cd, callable );
	ADD_ATTR_BOOL( cd, putable );
	ADD_ATTR_DOUBLE( cd, coupon );
	ADD_ATTR_BOOL( cd, convertible );
	ADD_ATTR_STRING( cd, maturity );
	ADD_ATTR_STRING( cd, issueDate );
	ADD_ATTR_STRING( cd, nextOptionDate );
	ADD_ATTR_STRING( cd, nextOptionType );
	ADD_ATTR_BOOL( cd, nextOptionPartial );
	ADD_ATTR_STRING( cd, notes );

	ADD_CHILD_TAGVALUELIST( cd, secIdList );
}


void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Execution& e )
{
	char tmp[128];
	static const IB::Execution dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	ADD_ATTR_STRING( e, execId );
	ADD_ATTR_STRING( e, time );
	ADD_ATTR_STRING( e, acctNumber );
	ADD_ATTR_STRING( e, exchange );
	ADD_ATTR_STRING( e, side );
	ADD_ATTR_INT( e, shares );
	ADD_ATTR_DOUBLE( e, price );
	ADD_ATTR_INT( e, permId );
	ADD_ATTR_LONG( e, clientId );
	ADD_ATTR_LONG( e, orderId );
	ADD_ATTR_INT( e, liquidation );
	ADD_ATTR_INT( e, cumQty );
	ADD_ATTR_DOUBLE( e, avgPrice );
	ADD_ATTR_STRING( e, orderRef );
	ADD_ATTR_STRING( e, evRule );
	ADD_ATTR_DOUBLE( e, evMultiplier );
}

void conv_ib2xml( xmlNodePtr parent, const char* name,
	const IB::ExecutionFilter& eF )
{
	char tmp[128];
	static const IB::ExecutionFilter dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	ADD_ATTR_LONG( eF, m_clientId );
	ADD_ATTR_STRING( eF, m_acctCode );
	ADD_ATTR_STRING( eF, m_time );
	ADD_ATTR_STRING( eF, m_symbol );
	ADD_ATTR_STRING( eF, m_secType );
	ADD_ATTR_STRING( eF, m_exchange );
	ADD_ATTR_STRING( eF, m_side );
}

void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::TagValue& tV )
{
	static const IB::TagValue dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	ADD_ATTR_STRING( tV, tag );
	ADD_ATTR_STRING( tV, value );
}

void conv_ib2xml( xmlNodePtr parent, const char* name,
	const IB::OrderComboLeg& oCL )
{
	char tmp[128];
	static const IB::OrderComboLeg dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	ADD_ATTR_DOUBLE( oCL, price );
}

void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Order& o )
{
	char tmp[128];
	static const IB::Order dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	ADD_ATTR_LONG( o, orderId );
	ADD_ATTR_LONG( o, clientId );
	ADD_ATTR_LONG( o, permId );
	ADD_ATTR_STRING( o, action );
	ADD_ATTR_LONG( o, totalQuantity );
	ADD_ATTR_STRING( o, orderType );
	ADD_ATTR_DOUBLE( o, lmtPrice );
	ADD_ATTR_DOUBLE( o, auxPrice );
	ADD_ATTR_STRING( o, tif );
	ADD_ATTR_STRING( o, ocaGroup );
	ADD_ATTR_INT( o, ocaType );
	ADD_ATTR_STRING( o, orderRef );
	ADD_ATTR_BOOL( o, transmit );
	ADD_ATTR_LONG( o, parentId );
	ADD_ATTR_BOOL( o, blockOrder );
	ADD_ATTR_BOOL( o, sweepToFill );
	ADD_ATTR_INT( o, displaySize );
	ADD_ATTR_INT( o, triggerMethod );
	ADD_ATTR_BOOL( o, outsideRth );
	ADD_ATTR_BOOL( o, hidden );
	ADD_ATTR_STRING( o, goodAfterTime );
	ADD_ATTR_STRING( o, goodTillDate );
	ADD_ATTR_STRING( o, rule80A );
	ADD_ATTR_BOOL( o, allOrNone );
	ADD_ATTR_INT( o, minQty );
	ADD_ATTR_DOUBLE( o, percentOffset );
	ADD_ATTR_BOOL( o, overridePercentageConstraints );
	ADD_ATTR_DOUBLE( o, trailStopPrice );
	ADD_ATTR_DOUBLE( o, trailingPercent );
	ADD_ATTR_STRING( o, faGroup );
	ADD_ATTR_STRING( o, faProfile );
	ADD_ATTR_STRING( o, faMethod );
	ADD_ATTR_STRING( o, faPercentage );
	ADD_ATTR_STRING( o, openClose );
	ADD_ATTR_INT( o, origin ); // TODO that's an enum!
	ADD_ATTR_INT( o, shortSaleSlot );
	ADD_ATTR_STRING( o, designatedLocation );
	ADD_ATTR_INT( o, exemptCode );
	ADD_ATTR_DOUBLE( o, discretionaryAmt );
	ADD_ATTR_BOOL( o, eTradeOnly );
	ADD_ATTR_BOOL( o, firmQuoteOnly );
	ADD_ATTR_DOUBLE( o, nbboPriceCap );
	ADD_ATTR_BOOL( o, optOutSmartRouting );
	ADD_ATTR_INT( o, auctionStrategy );
	ADD_ATTR_DOUBLE( o, startingPrice );
	ADD_ATTR_DOUBLE( o, stockRefPrice );
	ADD_ATTR_DOUBLE( o, delta );
	ADD_ATTR_DOUBLE( o, stockRangeLower );
	ADD_ATTR_DOUBLE( o, stockRangeUpper );
	ADD_ATTR_DOUBLE( o, volatility );
	ADD_ATTR_INT( o, volatilityType );
	ADD_ATTR_STRING( o,  deltaNeutralOrderType );
	ADD_ATTR_DOUBLE( o, deltaNeutralAuxPrice );
	ADD_ATTR_LONG( o, deltaNeutralConId );
	ADD_ATTR_STRING( o, deltaNeutralSettlingFirm );
	ADD_ATTR_STRING( o, deltaNeutralClearingAccount );
	ADD_ATTR_STRING( o, deltaNeutralClearingIntent );
	ADD_ATTR_STRING( o, deltaNeutralOpenClose );
	ADD_ATTR_BOOL( o, deltaNeutralShortSale );
	ADD_ATTR_INT( o, deltaNeutralShortSaleSlot );
	ADD_ATTR_STRING( o, deltaNeutralDesignatedLocation );
	ADD_ATTR_BOOL( o, continuousUpdate );
	ADD_ATTR_INT( o, referencePriceType );
	ADD_ATTR_DOUBLE( o, basisPoints );
	ADD_ATTR_INT( o, basisPointsType );
	ADD_ATTR_INT( o, scaleInitLevelSize );
	ADD_ATTR_INT( o, scaleSubsLevelSize );
	ADD_ATTR_DOUBLE( o, scalePriceIncrement );
	ADD_ATTR_DOUBLE( o, scalePriceAdjustValue );
	ADD_ATTR_INT( o, scalePriceAdjustInterval );
	ADD_ATTR_DOUBLE( o, scaleProfitOffset );
	ADD_ATTR_BOOL( o, scaleAutoReset );
	ADD_ATTR_INT( o, scaleInitPosition );
	ADD_ATTR_INT( o, scaleInitFillQty );
	ADD_ATTR_BOOL( o, scaleRandomPercent );
	ADD_ATTR_STRING( o, hedgeType );
	ADD_ATTR_STRING( o, hedgeParam );
	ADD_ATTR_STRING( o, account );
	ADD_ATTR_STRING( o, settlingFirm );
	ADD_ATTR_STRING( o, clearingAccount );
	ADD_ATTR_STRING( o, clearingIntent );
	ADD_ATTR_STRING( o, algoStrategy );

	ADD_CHILD_TAGVALUELIST( o, algoParams );
	ADD_CHILD_TAGVALUELIST( o, smartComboRoutingParams );

	ADD_ATTR_BOOL( o, whatIf );
	ADD_ATTR_BOOL( o, notHeld );
	if( o.orderComboLegs.get() != NULL ) {
		xmlNodePtr np = xmlNewChild( ne, NULL, (xmlChar*)"orderComboLegs", NULL);
		for( IB::Order::OrderComboLegList::const_iterator it
			 = o.orderComboLegs->begin(); it != o.orderComboLegs->end(); ++it) {
			conv_ib2xml( np, "orderComboLeg", **it );
		}
	}
}

void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::OrderState& os)
{
	char tmp[128];
	static const IB::OrderState dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(const xmlChar*)name, NULL);

	ADD_ATTR_STRING( os, status );
	ADD_ATTR_STRING( os, initMargin );
	ADD_ATTR_STRING( os, maintMargin );
	ADD_ATTR_STRING( os, equityWithLoan );
	ADD_ATTR_DOUBLE( os, commission );
	ADD_ATTR_DOUBLE( os, minCommission );
	ADD_ATTR_DOUBLE( os, maxCommission );
	ADD_ATTR_STRING( os, commissionCurrency );
	ADD_ATTR_STRING( os, warningText );
}




void conv_xml2ib( IB::ComboLeg* cl, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_LONG( cl, conId );
	GET_ATTR_LONG( cl, ratio );
	GET_ATTR_STRING( cl, action );
	GET_ATTR_STRING( cl, exchange );
	GET_ATTR_LONG( cl, openClose );
	GET_ATTR_LONG( cl, shortSaleSlot );
	GET_ATTR_STRING( cl, designatedLocation );
	GET_ATTR_INT( cl, exemptCode );
}

void conv_xml2ib( IB::UnderComp* uc, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_LONG( uc, conId );
	GET_ATTR_DOUBLE( uc, delta );
	GET_ATTR_DOUBLE( uc, price );
}

void conv_xml2ib( IB::Contract* c, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_LONG( c, conId );
	GET_ATTR_STRING( c, symbol );
	GET_ATTR_STRING( c, secType );
	GET_ATTR_STRING( c, expiry );
	GET_ATTR_DOUBLE( c, strike );
	GET_ATTR_STRING( c, right );
	GET_ATTR_STRING( c, multiplier );
	GET_ATTR_STRING( c, exchange );
	GET_ATTR_STRING( c, primaryExchange );
	GET_ATTR_STRING( c, currency );
	GET_ATTR_STRING( c, localSymbol );
	GET_ATTR_STRING( c, tradingClass );
	GET_ATTR_BOOL( c, includeExpired );
	GET_ATTR_STRING( c, secIdType );
	GET_ATTR_STRING( c, secId );
	GET_ATTR_STRING( c, comboLegsDescrip );

	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type != XML_ELEMENT_NODE ) {
			continue;
		}
		if( strcmp((char*) p->name, "comboLegs") == 0 ) {
			c->comboLegs = IB::Contract::ComboLegListSPtr(
				new IB::Contract::ComboLegList() );
			for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
				if( q->type != XML_ELEMENT_NODE
					|| (strcmp((char*) q->name, "comboLeg") != 0)) {
					continue;
				}
				IB::ComboLeg *cl = new IB::ComboLeg();
				conv_xml2ib( cl, q );
				c->comboLegs->push_back(IB::ComboLegSPtr(cl));
			}
		} else if( strcmp((char*) p->name, "underComp") == 0 ) {
			if( c->underComp != NULL ) {
				/* This can only happen if the caller gave us an uninitialized
				   contract (programming error) or if the xml wrongly contains
				   more than one underComp tag. For the second case we have to
				   avoid a memleak here */
				delete c->underComp;
			}
			c->underComp = new IB::UnderComp();
			conv_xml2ib( c->underComp, p );
		}
	}
}


void conv_xml2ib( IB::ContractDetails* cd, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_STRING( cd, marketName );
	/* for compatibility we move tradingClass attribute to the contract */
	GET_ATTR_STRING( (&cd->summary), tradingClass );
	GET_ATTR_DOUBLE( cd, minTick );
	GET_ATTR_STRING( cd, orderTypes );
	GET_ATTR_STRING( cd, validExchanges );
	GET_ATTR_LONG( cd, priceMagnifier );
	GET_ATTR_INT( cd, underConId );
	GET_ATTR_STRING( cd, longName );
	GET_ATTR_STRING( cd, contractMonth );
	GET_ATTR_STRING( cd, industry );
	GET_ATTR_STRING( cd, category );
	GET_ATTR_STRING( cd, subcategory );
	GET_ATTR_STRING( cd, timeZoneId );
	GET_ATTR_STRING( cd, tradingHours );
	GET_ATTR_STRING( cd, liquidHours );
	GET_ATTR_STRING( cd, evRule );
	GET_ATTR_DOUBLE( cd, evMultiplier );

	// BOND values
	GET_ATTR_STRING( cd, cusip );
	GET_ATTR_STRING( cd, ratings );
	GET_ATTR_STRING( cd, descAppend );
	GET_ATTR_STRING( cd, bondType );
	GET_ATTR_STRING( cd, couponType );
	GET_ATTR_BOOL( cd, callable );
	GET_ATTR_BOOL( cd, putable );
	GET_ATTR_DOUBLE( cd, coupon );
	GET_ATTR_BOOL( cd, convertible );
	GET_ATTR_STRING( cd, maturity );
	GET_ATTR_STRING( cd, issueDate );
	GET_ATTR_STRING( cd, nextOptionDate );
	GET_ATTR_STRING( cd, nextOptionType );
	GET_ATTR_BOOL( cd, nextOptionPartial );
	GET_ATTR_STRING( cd, notes );

	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type != XML_ELEMENT_NODE ) {
			continue;
		}
		if( strcmp((char*) p->name, "summary") == 0 ) {
			conv_xml2ib( &cd->summary, p );
		}
		else if(p->name && (strcmp((char*) p->name, "secIdList") == 0)) {
			GET_CHILD_TAGVALUELIST( cd->secIdList );
		}
	}
}


void conv_xml2ib( IB::Execution* e, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_STRING( e, execId );
	GET_ATTR_STRING( e, time );
	GET_ATTR_STRING( e, acctNumber );
	GET_ATTR_STRING( e, exchange );
	GET_ATTR_STRING( e, side );
	GET_ATTR_INT( e, shares );
	GET_ATTR_DOUBLE( e, price );
	GET_ATTR_INT( e, permId );
	GET_ATTR_LONG( e, clientId );
	GET_ATTR_LONG( e, orderId );
	GET_ATTR_INT( e, liquidation );
	GET_ATTR_INT( e, cumQty );
	GET_ATTR_DOUBLE( e, avgPrice );
	GET_ATTR_STRING( e, orderRef );
	GET_ATTR_STRING( e, evRule );
	GET_ATTR_DOUBLE( e, evMultiplier );
}


void conv_xml2ib( IB::ExecutionFilter* eF, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_LONG( eF, m_clientId );
	GET_ATTR_STRING( eF, m_acctCode );
	GET_ATTR_STRING( eF, m_time );
	GET_ATTR_STRING( eF, m_symbol );
	GET_ATTR_STRING( eF, m_secType );
	GET_ATTR_STRING( eF, m_exchange );
	GET_ATTR_STRING( eF, m_side );
}

void conv_xml2ib( IB::TagValue* tV, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_STRING( tV, tag );
	GET_ATTR_STRING( tV, value );
}

void conv_xml2ib( IB::OrderComboLeg* oCL, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_DOUBLE( oCL, price );
}

void conv_xml2ib( IB::Order* o, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_LONG( o, orderId );
	GET_ATTR_LONG( o, clientId );
	GET_ATTR_LONG( o, permId );
	GET_ATTR_STRING( o, action );
	GET_ATTR_LONG( o, totalQuantity );
	GET_ATTR_STRING( o, orderType );
	GET_ATTR_DOUBLE( o, lmtPrice );
	GET_ATTR_DOUBLE( o, auxPrice );
	GET_ATTR_STRING( o, tif );
	GET_ATTR_STRING( o, ocaGroup );
	GET_ATTR_INT( o, ocaType );
	GET_ATTR_STRING( o, orderRef );
	GET_ATTR_BOOL( o, transmit );
	GET_ATTR_LONG( o, parentId );
	GET_ATTR_BOOL( o, blockOrder );
	GET_ATTR_BOOL( o, sweepToFill );
	GET_ATTR_INT( o, displaySize );
	GET_ATTR_INT( o, triggerMethod );
	GET_ATTR_BOOL( o, outsideRth );
	GET_ATTR_BOOL( o, hidden );
	GET_ATTR_STRING( o, goodAfterTime );
	GET_ATTR_STRING( o, goodTillDate );
	GET_ATTR_STRING( o, rule80A );
	GET_ATTR_BOOL( o, allOrNone );
	GET_ATTR_INT( o, minQty );
	GET_ATTR_DOUBLE( o, percentOffset );
	GET_ATTR_BOOL( o, overridePercentageConstraints );
	GET_ATTR_DOUBLE( o, trailStopPrice );
	GET_ATTR_DOUBLE( o, trailingPercent );
	GET_ATTR_STRING( o, faGroup );
	GET_ATTR_STRING( o, faProfile );
	GET_ATTR_STRING( o, faMethod );
	GET_ATTR_STRING( o, faPercentage );
	GET_ATTR_STRING( o, openClose );

	tmp = (char*) xmlGetProp( node, (xmlChar*) "origin" );
	if(tmp) {
		int orderOriginInt = atoi( tmp );
		free(tmp);
		o->origin = (IB::Origin) orderOriginInt;
	}
	GET_ATTR_INT( o, shortSaleSlot );
	GET_ATTR_STRING( o, designatedLocation );
	GET_ATTR_INT( o, exemptCode );
	GET_ATTR_DOUBLE( o, discretionaryAmt );
	GET_ATTR_BOOL( o, eTradeOnly );
	GET_ATTR_BOOL( o, firmQuoteOnly );
	GET_ATTR_DOUBLE( o, nbboPriceCap );
	GET_ATTR_BOOL( o, optOutSmartRouting );
	GET_ATTR_INT( o, auctionStrategy );
	GET_ATTR_DOUBLE( o, startingPrice );
	GET_ATTR_DOUBLE( o, stockRefPrice );
	GET_ATTR_DOUBLE( o, delta );
	GET_ATTR_DOUBLE( o, stockRangeLower );
	GET_ATTR_DOUBLE( o, stockRangeUpper );
	GET_ATTR_DOUBLE( o, volatility );
	GET_ATTR_INT( o, volatilityType );
	GET_ATTR_STRING( o,  deltaNeutralOrderType );
	GET_ATTR_DOUBLE( o, deltaNeutralAuxPrice );
	GET_ATTR_LONG( o, deltaNeutralConId );
	GET_ATTR_STRING( o, deltaNeutralSettlingFirm );
	GET_ATTR_STRING( o, deltaNeutralClearingAccount );
	GET_ATTR_STRING( o, deltaNeutralClearingIntent );
	GET_ATTR_STRING( o, deltaNeutralOpenClose );
	GET_ATTR_BOOL( o, deltaNeutralShortSale );
	GET_ATTR_INT( o, deltaNeutralShortSaleSlot );
	GET_ATTR_STRING( o, deltaNeutralDesignatedLocation );
	GET_ATTR_BOOL( o, continuousUpdate );
	GET_ATTR_INT( o, referencePriceType );
	GET_ATTR_DOUBLE( o, basisPoints );
	GET_ATTR_INT( o, basisPointsType );
	GET_ATTR_INT( o, scaleInitLevelSize );
	GET_ATTR_INT( o, scaleSubsLevelSize );
	GET_ATTR_DOUBLE( o, scalePriceIncrement );
	GET_ATTR_DOUBLE( o, scalePriceAdjustValue );
	GET_ATTR_INT( o, scalePriceAdjustInterval );
	GET_ATTR_DOUBLE( o, scaleProfitOffset );
	GET_ATTR_BOOL( o, scaleAutoReset );
	GET_ATTR_INT( o, scaleInitPosition );
	GET_ATTR_INT( o, scaleInitFillQty );
	GET_ATTR_BOOL( o, scaleRandomPercent );
	GET_ATTR_STRING( o, hedgeType );
	GET_ATTR_STRING( o, hedgeParam );
	GET_ATTR_STRING( o, account );
	GET_ATTR_STRING( o, settlingFirm );
	GET_ATTR_STRING( o, clearingAccount );
	GET_ATTR_STRING( o, clearingIntent );
	GET_ATTR_STRING( o, algoStrategy );
	GET_ATTR_BOOL( o, whatIf );
	GET_ATTR_BOOL( o, notHeld );

	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type != XML_ELEMENT_NODE ) {
			continue;
		}
		if( strcmp((char*) p->name, "algoParams") == 0 ) {
			GET_CHILD_TAGVALUELIST( o->algoParams );
		} else if( strcmp((char*) p->name, "smartComboRoutingParams") == 0 ) {
			GET_CHILD_TAGVALUELIST( o->smartComboRoutingParams );
		}
		else if( strcmp((char*) p->name, "orderComboLegs") == 0 ) {
			o->orderComboLegs = IB::Order::OrderComboLegListSPtr(
				new IB::Order::OrderComboLegList() );
			for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
				IB::OrderComboLegSPtr oCL( new IB::OrderComboLeg());
				if( q->type != XML_ELEMENT_NODE
				    || (strcmp((char*) q->name, "orderComboLeg") != 0)) {
					continue;
				}
				conv_xml2ib( oCL.get(), q );
				o->orderComboLegs->push_back(oCL);
			}
		}
	}
}

void conv_xml2ib( IB::OrderState* os, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_STRING( os, status );
	GET_ATTR_STRING( os, initMargin );
	GET_ATTR_STRING( os, maintMargin );
	GET_ATTR_STRING( os, equityWithLoan );
	GET_ATTR_DOUBLE( os, commission );
	GET_ATTR_DOUBLE( os, minCommission );
	GET_ATTR_DOUBLE( os, maxCommission );
	GET_ATTR_STRING( os, commissionCurrency );
	GET_ATTR_STRING( os, warningText );
}


void to_xml( xmlNodePtr parent, const ContractDetailsRequest& cdr)
{
	xmlNodePtr nqry = xmlNewChild( parent, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( nqry, "reqContract", cdr.ibContract() );
}

void to_xml( xmlNodePtr parent, const HistRequest& hr)
{
	char tmp[128];
	static const HistRequest dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( ne, "reqContract", hr.ibContract );
	ADD_ATTR_STRING( hr, endDateTime );
	ADD_ATTR_STRING( hr, durationStr );
	ADD_ATTR_STRING( hr, barSizeSetting );
	ADD_ATTR_STRING( hr, whatToShow );
	ADD_ATTR_INT( hr, useRTH );
	ADD_ATTR_INT( hr, formatDate );
}

void to_xml( xmlNodePtr parent, const AccStatusRequest &aR )
{
	static const AccStatusRequest dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"query", NULL);
	ADD_ATTR_BOOL( aR, subscribe );
	ADD_ATTR_STRING( aR, acctCode );
}

void to_xml( xmlNodePtr parent, const ExecutionsRequest &eR )
{
	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( ne, "executionFilter", eR.executionFilter );
}

void to_xml( xmlNodePtr parent, const OrdersRequest& /*oR*/)
{
	/*xmlNodePtr ne =*/ xmlNewChild( parent, NULL, (xmlChar*)"query", NULL);
}

void to_xml( xmlNodePtr parent, const PlaceOrder& po)
{
	char tmp[128];
	static const PlaceOrder dflt;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( ne, "contract", po.contract );
	conv_ib2xml( ne, "order", po.order );
	ADD_ATTR_LONG( po, orderId );
	ADD_ATTR_LONGLONG( po, time_sent );
}

void to_xml( xmlNodePtr parent, const MktDataRequest& co)
{
	/* not implemented yet */
	assert( false );
}




void from_xml( ContractDetailsRequest *cdr, const xmlNodePtr node )
{
	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE
			&& strcmp((char*)p->name, "reqContract") == 0 )  {
			IB::Contract c;
			conv_xml2ib( &c, p);
			cdr->initialize(c);
		}
	}
}

void from_xml( HistRequest *hR, const xmlNodePtr node )
{
	char* tmp;

	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE
			&& strcmp((char*)p->name, "reqContract") == 0 )  {
			conv_xml2ib( &hR->ibContract, p);
		}
	}

	GET_ATTR_STRING( hR, endDateTime );
	GET_ATTR_STRING( hR, durationStr );
	GET_ATTR_STRING( hR, barSizeSetting );
	GET_ATTR_STRING( hR, whatToShow );
	GET_ATTR_INT( hR, useRTH );
	GET_ATTR_INT( hR, formatDate );
}

void from_xml( AccStatusRequest *aR, const xmlNodePtr node )
{
	char* tmp;

	GET_ATTR_BOOL( aR, subscribe );
	GET_ATTR_STRING( aR, acctCode );
}

void from_xml( ExecutionsRequest *eR, const xmlNodePtr node )
{
	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE
			&& strcmp((char*)p->name, "executionFilter") == 0 )  {
			conv_xml2ib( &eR->executionFilter, p);
		}
	}
}

void from_xml( OrdersRequest* /*oR*/, const xmlNodePtr /*node*/ )
{
}

void from_xml( PlaceOrder* po, const xmlNodePtr node )
{
	char* tmp;

	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE ) {
			if( strcmp((char*)p->name, "contract") == 0 )  {
				conv_xml2ib( &po->contract, p);
			} else if( strcmp((char*)p->name, "order") == 0 )  {
				conv_xml2ib( &po->order, p);
			}
		}
	}

	GET_ATTR_LONG( po, orderId );
	GET_ATTR_LONGLONG( po, time_sent );
}

void from_xml( MktDataRequest* mdr, const xmlNodePtr node )
{
	char* tmp;

	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE
			&& strcmp((char*)p->name, "reqContract") == 0 )  {
			conv_xml2ib( &mdr->ibContract, p);
		}
	}

	GET_ATTR_STRING( mdr, genericTicks );
	GET_ATTR_BOOL( mdr, snapshot );
}




static void to_xml( xmlNodePtr parent, const RowError& row )
{
	char tmp[128];

	xmlNodePtr nrow = xmlNewChild( parent,
		NULL, (const xmlChar*)"error", NULL);
	A_ADD_ATTR_INT( nrow, row, id );
	A_ADD_ATTR_INT( nrow, row, code );
	A_ADD_ATTR_STRING( nrow, row, msg );
}

void to_xml( xmlNodePtr parent, const char* name, const RowHist& r)
{
	char tmp[128];
	static const RowHist &dflt = dflt_RowHist;

	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)name, NULL);
	ADD_ATTR_STRING( r, date );
	ADD_ATTR_DOUBLE( r, open );
	ADD_ATTR_DOUBLE( r, high );
	ADD_ATTR_DOUBLE( r, low );
	ADD_ATTR_DOUBLE( r, close );
	ADD_ATTR_INT( r, volume );
	ADD_ATTR_INT( r, count );
	ADD_ATTR_DOUBLE( r, WAP );
	ADD_ATTR_BOOL( r, hasGaps );
}

void to_xml( xmlNodePtr parent, const RowAcc& row )
{
	char tmp[128];
	switch( row.type ) {
	case RowAcc::t_AccVal:
		{
			const RowAccVal &d = *(RowAccVal*)row.data;
			xmlNodePtr nrow = xmlNewChild( parent,
				NULL, (const xmlChar*)"AccVal", NULL);
			A_ADD_ATTR_STRING( nrow, d, key );
			A_ADD_ATTR_STRING( nrow, d, val );
			A_ADD_ATTR_STRING( nrow, d, currency );
			A_ADD_ATTR_STRING( nrow, d, accountName );
		}
		break;
	case RowAcc::t_Prtfl:
		{
			const RowPrtfl &d = *(RowPrtfl*)row.data;
			xmlNodePtr nrow = xmlNewChild( parent,
				NULL, (const xmlChar*)"Prtfl", NULL);
			conv_ib2xml( nrow, "contract", d.contract );
			A_ADD_ATTR_INT( nrow, d, position );
			A_ADD_ATTR_DOUBLE( nrow, d, marketPrice );
			A_ADD_ATTR_DOUBLE( nrow, d, marketValue );
			A_ADD_ATTR_DOUBLE( nrow, d, averageCost );
			A_ADD_ATTR_DOUBLE( nrow, d, unrealizedPNL );
			A_ADD_ATTR_DOUBLE( nrow, d, realizedPNL );
			A_ADD_ATTR_STRING( nrow, d, accountName );
		}
		break;
	case RowAcc::t_stamp:
		{
			const std::string &d = *(std::string*)row.data;
			xmlNodePtr nrow = xmlNewChild( parent,
				NULL, (const xmlChar*)"stamp", NULL);
			xmlNewProp ( nrow, (xmlChar*) "timeStamp",
				(const xmlChar*) d.c_str() );
		}
		break;
	case RowAcc::t_end:
		{
			const std::string &d = *(std::string*)row.data;
			xmlNodePtr nrow = xmlNewChild( parent,
				NULL, (const xmlChar*)"end", NULL);
			xmlNewProp ( nrow, (xmlChar*) "accountName",
				(const xmlChar*) d.c_str() );
		}
		break;
	}
}


void to_xml( xmlNodePtr parent, const RowExecution &row )
{
	xmlNodePtr nrow = xmlNewChild( parent,
		NULL, (const xmlChar*)"ExecDetails", NULL);
	conv_ib2xml( nrow, "contract", row.contract );
	conv_ib2xml( nrow, "execution", row.execution );
}

static void to_xml( xmlNodePtr parent, const RowOrderStatus &d )
{
	char tmp[128];
	xmlNodePtr nrow = xmlNewChild( parent,
		NULL, (const xmlChar*)"OrderStatus", NULL);
	A_ADD_ATTR_LONG(nrow, d, id);
	A_ADD_ATTR_STRING( nrow, d, status );
	A_ADD_ATTR_INT( nrow, d, filled );
	A_ADD_ATTR_INT( nrow, d, remaining );
	A_ADD_ATTR_DOUBLE( nrow, d, avgFillPrice );
	A_ADD_ATTR_INT( nrow, d, permId );
	A_ADD_ATTR_INT( nrow, d, parentId );
	A_ADD_ATTR_DOUBLE( nrow, d, lastFillPrice );
	A_ADD_ATTR_INT( nrow, d, clientId );
	A_ADD_ATTR_STRING( nrow, d, whyHeld );
}

static void to_xml( xmlNodePtr parent, const RowOpenOrder &d )
{
	char tmp[128];
	xmlNodePtr nrow = xmlNewChild( parent,
		NULL, (const xmlChar*)"OpenOrder", NULL);
	A_ADD_ATTR_LONG(nrow, d, orderId);
	conv_ib2xml( nrow, "contract", d.contract );
	conv_ib2xml( nrow, "order", d.order );
	conv_ib2xml( nrow, "orderState", d.orderState );
}

void to_xml( xmlNodePtr parent, const TwsRow &row)
{
	switch( row.type ) {
	case t_error:
		to_xml( parent, *(RowError*)row.data );
		break;
	case t_orderStatus:
		to_xml( parent, *(RowOrderStatus*)row.data );
		break;
	case t_openOrder:
		to_xml( parent, *(RowOpenOrder*)row.data );
		break;
	}
}





static void from_xml( RowError*, const xmlNodePtr /*node*/ )
{
	/* not implemented yet */
	assert( false );
}

void from_xml( RowHist *row, const xmlNodePtr node )
{
	char* tmp;
	*row = dflt_RowHist;

	GET_ATTR_STRING( row, date );
	GET_ATTR_DOUBLE( row, open );
	GET_ATTR_DOUBLE( row, high );
	GET_ATTR_DOUBLE( row, low );
	GET_ATTR_DOUBLE( row, close );
	GET_ATTR_INT( row, volume );
	GET_ATTR_INT( row, count );
	GET_ATTR_DOUBLE( row, WAP );
	GET_ATTR_BOOL( row, hasGaps );
}

void from_xml( RowAcc* /*row*/, const xmlNodePtr /*node*/ )
{
	/* not implemented yet */
	assert( false );
}

void from_xml( RowExecution* /*row*/, const xmlNodePtr /*node*/ )
{
	/* not implemented yet */
	assert( false );
}

static void from_xml( RowOrderStatus* /*row*/, const xmlNodePtr /*node*/ )
{
	/* not implemented yet */
	assert( false );
}

static void from_xml( RowOpenOrder* /*row*/, const xmlNodePtr /*node*/ )
{
	/* not implemented yet */
	assert( false );
}

void from_xml( TwsRow* /*row*/, const xmlNodePtr /*node*/ )
{
	/* not implemented yet */
	assert( false );
}




static int find_form_feed( const char *s, int n )
{
	int i;
	for( i=0; i<n; i++ ) {
		if( s[i] == '\f' ) {
			break;
		}
	}
	return i;
}

#define CHUNK_SIZE 1024
#define BUF_SIZE 1024 * 1024


TwsXml::TwsXml() :
	file(NULL),
	buf_size(0),
	buf_len(0),
	buf(NULL),
	curDoc(NULL),
	curNode(NULL)
{
	resize_buf();
}

TwsXml::~TwsXml()
{
	if( file != NULL ) {
		fclose((FILE*)file);
	}
	free(buf);
	if( curDoc != NULL ) {
		xmlFreeDoc( curDoc );
	}
}

void TwsXml::resize_buf()
{
	buf_size = buf_size + BUF_SIZE;
	buf = (char*) realloc( buf, buf_size );
}

bool TwsXml::openFile( const char *filename )
{
	if( filename == NULL ) {
		int tty = isatty(STDIN_FILENO);
		if( tty ) {
			fprintf( stderr, "error, No file specified and stdin is a tty.\n" );
			return false;
		}
		if( errno == EBADF ) {
			fprintf( stderr, "error, %s\n", strerror(errno) );
			return false;
		}
		file = stdin;
	} else {
		file = fopen(filename, "rb");
		if( file == NULL ) {
			fprintf( stderr, "error, %s: '%s'\n", strerror(errno), filename );
			return false;
		}
	}

	assert( file != NULL );
	return true;
}

xmlDocPtr TwsXml::nextXmlDoc()
{
	xmlDocPtr doc = NULL;
	if( file == NULL ) {
		return doc;
	}

	/* This is ugly code. If we will rewrite it to use xml push parser then we
	   could avoid resize_buf(), memmove() and even find_form_feed(). */
	int jump_ff = 0;
	char *cp = buf;
	int tmp_len = buf_len;
	while( true ) {
		int ff = find_form_feed(cp, tmp_len);
		if( ff < tmp_len ) {
			cp += ff;
			jump_ff = 1;
			break;
		}
		cp += tmp_len;

		if( (buf_len + CHUNK_SIZE) >= buf_size ) {
			resize_buf();
			cp = buf + buf_len;
		}
		tmp_len = fread(cp, 1, CHUNK_SIZE, (FILE*)file);
		if( tmp_len <=0 ) {
			jump_ff = 0;
			break;
		}
		buf_len += tmp_len;
	}

	doc = xmlReadMemory( buf, cp-buf, "URL", NULL, 0 );

	buf_len -= cp - buf + jump_ff;
	memmove( buf, cp + jump_ff, buf_len );

	return doc;
}

xmlNodePtr TwsXml::nextXmlRoot()
{
	if( curDoc != NULL ) {
		xmlFreeDoc(curDoc);
	}

	while( (curDoc = nextXmlDoc()) != NULL ) {
		xmlNodePtr root = xmlDocGetRootElement(curDoc);
		if( root != NULL ) {
			assert( root->type == XML_ELEMENT_NODE );
			if( strcmp((char*)root->name, "TWSXML") == 0 ) {
// 				fprintf(stderr, "Notice, return root '%s'.\n", root->name);
				return root;
			} else {
				fprintf(stderr, "Warning, ignore unknown root '%s'.\n",
					root->name);
			}
		} else {
			fprintf(stderr, "Warning, no root element found.\n");
		}
		xmlFreeDoc(curDoc);
	}
// 	fprintf(stderr, "Notice, all roots parsed.\n");
	return NULL;
}

xmlNodePtr TwsXml::nextXmlNode()
{
	if( curNode!= NULL ) {
		curNode = curNode->next;
	}
	for( ; curNode!= NULL; curNode = curNode->next ) {
		if( curNode->type == XML_ELEMENT_NODE ) {
// 			fprintf(stderr, "Notice, return element '%s'.\n", curNode->name);
			return curNode;
		} else {
// 			fprintf(stderr, "Warning, ignore element '%s'.\n", curNode->name);
		}
	}
	assert( curNode == NULL );

	xmlNodePtr root;
	while( (root = nextXmlRoot()) != NULL ) {
		for( xmlNodePtr p = root->children; p!= NULL; p=p->next) {
			if( p->type == XML_ELEMENT_NODE ) {
				curNode = p;
				break;
			} else {
// 				fprintf(stderr, "Warning, ignore element '%s'.\n", p->name);
			}
		}
		if( curNode != NULL ) {
			break;
		} else {
			fprintf(stderr, "Warning, no usable element found.\n");
		}
	}

	if( curNode != NULL ) {
// 		fprintf(stderr, "Notice, return element '%s'.\n", curNode->name);
	} else {
// 		fprintf(stderr, "Notice, all elements parsed.\n");
#if defined HAVE_MALLOC_TRIM
		malloc_trim(0);
#endif
	}

	return curNode;
}




bool TwsXml::_skip_defaults = false;
const bool& TwsXml::skip_defaults = _skip_defaults;

void TwsXml::setSkipDefaults( bool b )
{
	_skip_defaults = b;
}

xmlNodePtr TwsXml::newDocRoot()
{
	xmlDocPtr doc = xmlNewDoc( (const xmlChar*) "1.0");
	xmlNodePtr root = xmlNewDocNode( doc, NULL,
		(const xmlChar*)"TWSXML", NULL );
	xmlDocSetRootElement( doc, root );

	//caller has to free root.doc
	return root;
}

void TwsXml::dumpAndFree( xmlNodePtr root )
{
	xmlDocFormatDump(stdout, root->doc, 1);
	//HACK print form feed as xml file separator
	printf("\f");

	xmlFreeDoc(root->doc);
}

