/***  twsWrapper.cpp -- simple "IB/API wrapper" just for debugging
 *
 * Copyright (C) 2010-2012 Ruediger Meier
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

#include "tws_wrapper.h"
#include "tws_util.h"
#include "debug.h"
#include "config.h"

#ifdef HAVE_TWSAPI_TWSAPI_CONFIG_H
# include "twsapi/twsapi_config.h"
#endif

// from global installed ibapi
#include "twsapi/Contract.h"
#include "twsapi/Order.h"
#include "twsapi/OrderState.h"
#if TWSAPI_IB_VERSION_NUMBER > 966
# include "twsapi/CommissionReport.h"
#endif



DebugTwsWrapper::~DebugTwsWrapper()
{
}





void DebugTwsWrapper::tickPrice( IB::TickerId tickerId, IB::TickType field,
	double price, int canAutoExecute )
{
	DEBUG_PRINTF( "TICK_PRICE: %ld %s %g %d",
		tickerId, ibToString(field).c_str(), price, canAutoExecute);
}


void DebugTwsWrapper::tickSize( IB::TickerId tickerId, IB::TickType field,
	int size )
{
	DEBUG_PRINTF( "TICK_SIZE: %ld %s %d",
		tickerId, ibToString(field).c_str(), size );
}


void DebugTwsWrapper::tickOptionComputation ( IB::TickerId tickerId,
	IB::TickType tickType, double impliedVol, double delta, double optPrice,
	double pvDividend, double gamma, double vega, double theta,
	double undPrice )
{
	DEBUG_PRINTF( "TICK_OPTION_COMPUTATION: %ld %s %g %g %g %g %g %g %g %g",
		tickerId, ibToString(tickType).c_str(), impliedVol, delta,
		optPrice, pvDividend, gamma, vega, theta, undPrice );
}


void DebugTwsWrapper::tickGeneric( IB::TickerId tickerId, IB::TickType tickType,
	double value )
{
	DEBUG_PRINTF( "TICK_GENERIC: %ld %s %g",
		tickerId, ibToString(tickType).c_str(), value );
}


void DebugTwsWrapper::tickString( IB::TickerId tickerId, IB::TickType tickType,
	const IB::IBString& value )
{
	if( tickType == IB::LAST_TIMESTAMP ) {
		// here we format unix timestamp value
	}
	DEBUG_PRINTF( "TICK_STRING: %ld %s %s",
		tickerId, ibToString(tickType).c_str(), value.c_str() );
}


void DebugTwsWrapper::tickEFP( IB::TickerId tickerId, IB::TickType tickType,
	double basisPoints, const IB::IBString& formattedBasisPoints,
	double totalDividends, int holdDays, const IB::IBString& futureExpiry,
	double dividendImpact, double dividendsToExpiry )
{
	// TODO
	DEBUG_PRINTF( "TICK_EFP: %ld %s", tickerId, ibToString(tickType).c_str() );
}


void DebugTwsWrapper::orderStatus ( IB::OrderId orderId,
	const IB::IBString &status, int filled, int remaining, double avgFillPrice,
	int permId, int parentId, double lastFillPrice, int clientId,
	const IB::IBString& whyHeld )
{
	DEBUG_PRINTF( "ORDER_STATUS: "
		"orderId:%ld, status:%s, filled:%d, remaining:%d, %d %d %g %g %d, %s",
		orderId, status.c_str(), filled, remaining, permId, parentId,
		avgFillPrice, lastFillPrice, clientId, whyHeld.c_str());
}


void DebugTwsWrapper::openOrder( IB::OrderId orderId,
	const IB::Contract &contract, const IB::Order &order,
	const IB::OrderState &orderState )
{
	DEBUG_PRINTF( "OPEN_ORDER: %ld %s %s "
		"warnTxt:%s, status:%s, com:%g, comCur:%s, minCom:%g, maxCom:%g, "
		"initMarg:%s, maintMarg:%s, ewl:%s",
		orderId, contract.symbol.c_str(), order.action.c_str(),
		orderState.warningText.c_str(),
		orderState.status.c_str(),
		orderState.commission,
		orderState.commissionCurrency.c_str(),
		orderState.minCommission,
		orderState.maxCommission,
		orderState.initMargin.c_str(),
		orderState.maintMargin.c_str(),
		orderState.equityWithLoan.c_str() );
}


void DebugTwsWrapper::openOrderEnd()
{
	DEBUG_PRINTF( "OPEN_ORDER_END" );
}


void DebugTwsWrapper::winError( const IB::IBString &str, int lastError )
{
	DEBUG_PRINTF( "WIN_ERROR: %s %d", str.c_str(), lastError );
}


void DebugTwsWrapper::connectionClosed()
{
	DEBUG_PRINTF( "CONNECTION_CLOSED" );
}


void DebugTwsWrapper::updateAccountValue( const IB::IBString& key,
	const IB::IBString& val, const IB::IBString& currency,
	const IB::IBString& accountName )
{
	DEBUG_PRINTF( "ACCT_VALUE: %s %s %s %s",
		key.c_str(), val.c_str(), currency.c_str(), accountName.c_str() );
}


void DebugTwsWrapper::updatePortfolio( const IB::Contract& contract,
	int position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IB::IBString& accountName)
{
	DEBUG_PRINTF( "PORTFOLIO_VALUE: %s %s %d %g %g %g %g %g %s",
		contract.symbol.c_str(), contract.localSymbol.c_str(), position,
		marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL,
		accountName.c_str() );
}


void DebugTwsWrapper::updateAccountTime( const IB::IBString& timeStamp )
{
	DEBUG_PRINTF( "ACCT_UPDATE_TIME: %s", timeStamp.c_str() );
}


void DebugTwsWrapper::accountDownloadEnd( const IB::IBString& accountName )
{
	DEBUG_PRINTF( "ACCT_DOWNLOAD_END: %s", accountName.c_str() );
}


void DebugTwsWrapper::nextValidId( IB::OrderId orderId )
{
	DEBUG_PRINTF( "NEXT_VALID_ID: %ld", orderId );
}


void DebugTwsWrapper::contractDetails( int reqId,
	const IB::ContractDetails& contractDetails )
{
	DEBUG_PRINTF( "CONTRACT_DATA: %d %s %s %s %g %s %s %s %s %s %s",
		reqId,
		contractDetails.summary.symbol.c_str(),
		contractDetails.summary.secType.c_str(),
		contractDetails.summary.expiry.c_str(),
		contractDetails.summary.strike,
		contractDetails.summary.right.c_str(),
		contractDetails.summary.exchange.c_str(),
		contractDetails.summary.currency.c_str(),
		contractDetails.summary.localSymbol.c_str(),
		contractDetails.marketName.c_str(),
		contractDetails.tradingClass.c_str() );
}


void DebugTwsWrapper::bondContractDetails( int reqId,
	const IB::ContractDetails& contractDetails )
{
	//TODO
	DEBUG_PRINTF( "BOND_CONTRACT_DATA: %d %s %s %s %g %s %s %s %s %s %s",
		reqId,
		contractDetails.summary.symbol.c_str(),
		contractDetails.summary.secType.c_str(),
		contractDetails.summary.expiry.c_str(),
		contractDetails.summary.strike,
		contractDetails.summary.right.c_str(),
		contractDetails.summary.exchange.c_str(),
		contractDetails.summary.currency.c_str(),
		contractDetails.summary.localSymbol.c_str(),
		contractDetails.marketName.c_str(),
		contractDetails.tradingClass.c_str() );
}


void DebugTwsWrapper::contractDetailsEnd( int reqId )
{
	DEBUG_PRINTF( "CONTRACT_DATA_END: %d", reqId );
}


void DebugTwsWrapper::execDetails ( int orderId, const IB::Contract& contract,
	const IB::Execution& execution )
{
	DEBUG_PRINTF( "EXECUTION_DATA: %d %s %s %ld %s", orderId,
		contract.symbol.c_str(), contract.localSymbol.c_str(), contract.conId,
		ibToString(execution).c_str());
}


void DebugTwsWrapper::execDetailsEnd( int reqId )
{
	DEBUG_PRINTF( "EXECUTION_DATA_END: %d", reqId );
}


void DebugTwsWrapper::error( int id, int errorCode,
	const IB::IBString errorMsg )
{
	DEBUG_PRINTF( "ERR_MSG: %d %d %s", id, errorCode, errorMsg.c_str() );
}


void DebugTwsWrapper::updateMktDepth( IB::TickerId id, int position,
	int operation, int side, double price, int size )
{
	// TODO
	DEBUG_PRINTF( "MARKET_DEPTH: %ld", id );
}


void DebugTwsWrapper::updateMktDepthL2( IB::TickerId id, int position,
	IB::IBString marketMaker, int operation, int side, double price,
	int size )
{
	// TODO
	DEBUG_PRINTF( "MARKET_DEPTH_L2: %ld", id );
}


void DebugTwsWrapper::updateNewsBulletin( int msgId, int msgType,
	const IB::IBString& newsMessage, const IB::IBString& originExch )
{
	// TODO
	DEBUG_PRINTF( "NEWS_BULLETINS: %d", msgId );
}


void DebugTwsWrapper::managedAccounts( const IB::IBString& accountsList )
{
	DEBUG_PRINTF( "MANAGED_ACCTS: %s", accountsList.c_str() );
}


void DebugTwsWrapper::receiveFA( IB::faDataType pFaDataType,
	const IB::IBString& cxml )
{
	// TODO
	DEBUG_PRINTF( "RECEIVE_FA: %s", cxml.c_str() );
}


void DebugTwsWrapper::historicalData( IB::TickerId reqId,
	const IB::IBString& date, double open, double high, double low,
	double close, int volume, int barCount, double WAP, int hasGaps )
{
	DEBUG_PRINTF( "HISTORICAL_DATA: %ld %s %g %g %g %g %d %d %g %d", reqId,
		date.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps );
}


void DebugTwsWrapper::scannerParameters( const IB::IBString &xml )
{
	DEBUG_PRINTF( "SCANNER_PARAMETERS: %s", xml.c_str() );
}


void DebugTwsWrapper::scannerData( int reqId, int rank,
	const IB::ContractDetails &contractDetails, const IB::IBString &distance,
	const IB::IBString &benchmark, const IB::IBString &projection,
	const IB::IBString &legsStr )
{
	// TODO
	DEBUG_PRINTF( "SCANNER_DATA: %d", reqId );
}


void DebugTwsWrapper::scannerDataEnd(int reqId)
{
	DEBUG_PRINTF( "SCANNER_DATA_END: %d", reqId );
}


void DebugTwsWrapper::realtimeBar( IB::TickerId reqId, long time, double open,
	double high, double low, double close, long volume, double wap, int count )
{
	// TODO
	DEBUG_PRINTF( "REAL_TIME_BARS: %ld %ld", reqId, time );
}


void DebugTwsWrapper::currentTime( long time )
{
	DEBUG_PRINTF( "CURRENT_TIME: %ld", time );
}


void DebugTwsWrapper::fundamentalData( IB::TickerId reqId,
	const IB::IBString& data )
{
	DEBUG_PRINTF( "FUNDAMENTAL_DATA: %ld %s", reqId, data.c_str() );
}


void DebugTwsWrapper::deltaNeutralValidation( int reqId,
	const IB::UnderComp& underComp )
{
	// TODO
	DEBUG_PRINTF( "DELTA_NEUTRAL_VALIDATION: %d", reqId );
}


void DebugTwsWrapper::tickSnapshotEnd( int reqId )
{
	DEBUG_PRINTF( "TICK_SNAPSHOT_END: %d", reqId );
}

void DebugTwsWrapper::marketDataType( IB::TickerId reqId, int marketDataType )
{
	DEBUG_PRINTF( "MARKET_DATA_TYPE: %ld %d", reqId, marketDataType );
}

/* nobody should reference this callback except a newer twsapi */
#if TWSAPI_IB_VERSION_NUMBER > 966
void DebugTwsWrapper::commissionReport( const IB::CommissionReport &cr )
{
	DEBUG_PRINTF( "COMMISSION_REPORT %s %g %s %g %g %d", cr.execId.c_str(),
		cr.commission, cr.currency.c_str(), cr.realizedPNL, cr.yield,
		cr.yieldRedemptionDate );
}
#endif
