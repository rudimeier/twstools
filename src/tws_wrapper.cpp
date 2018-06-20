/*** tws_wrapper.cpp -- simple "IB/API wrapper" just for debugging
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_wrapper.h"
#include "twsdo.h"
#include "tws_meta.h"
#include "tws_util.h"
#include "debug.h"
#include "config.h"

#include <twsapi/twsapi_config.h>
#include <twsapi/Contract.h>
#include <twsapi/Order.h>
#include <twsapi/OrderState.h>
#include <twsapi/CommissionReport.h>

TwsDlWrapper::TwsDlWrapper( TwsDL* parent ) :
	parentTwsDL(parent)
{
}

TwsDlWrapper::~TwsDlWrapper()
{
}

void TwsDlWrapper::tickPrice( TickerId tickerId, TickType field,
	double price, const TickAttrib& ta)
{
#if 0
	DEBUG_PRINTF( "TICK_PRICE: %ld %s %g %d",
		tickerId, ibToString(field).c_str(), price, ta.canAutoExecute);
#endif
	parentTwsDL->twsTickPrice( tickerId, field, price, ta.canAutoExecute );
}

void TwsDlWrapper::tickSize( TickerId tickerId, TickType field,
	int size )
{
#if 0
	DEBUG_PRINTF( "TICK_SIZE: %ld %s %d",
		tickerId, ibToString(field).c_str(), size );
#endif
	parentTwsDL->twsTickSize( tickerId, field, size );
}

void TwsDlWrapper::tickOptionComputation ( TickerId tickerId,
	TickType tickType, double impliedVol, double delta, double optPrice,
	double pvDividend, double gamma, double vega, double theta,
	double undPrice )
{
#if 0
	DEBUG_PRINTF( "TICK_OPTION_COMPUTATION: %ld %s %g %g %g %g %g %g %g %g",
		tickerId, ibToString(tickType).c_str(), impliedVol, delta,
		optPrice, pvDividend, gamma, vega, theta, undPrice );
#endif
	parentTwsDL->twsTickOptionComputation(tickerId, tickType, impliedVol,
		delta, optPrice, pvDividend, gamma, vega, theta, undPrice);
}

void TwsDlWrapper::tickGeneric( TickerId tickerId, TickType tickType,
	double value )
{
#if 0
	DEBUG_PRINTF( "TICK_GENERIC: %ld %s %g",
		tickerId, ibToString(tickType).c_str(), value );
#endif
	parentTwsDL->twsTickGeneric(tickerId, tickType, value);
}

void TwsDlWrapper::tickString( TickerId tickerId, TickType tickType,
	const IBString& value )
{
#if 0
	if( tickType == LAST_TIMESTAMP ) {
		// here we format unix timestamp value
	}
	DEBUG_PRINTF( "TICK_STRING: %ld %s %s",
		tickerId, ibToString(tickType).c_str(), value.c_str() );
#endif
	parentTwsDL->twsTickString( tickerId, tickType, value);
}

void TwsDlWrapper::tickEFP( TickerId tickerId, TickType tickType,
	double basisPoints, const IBString& formattedBasisPoints,
	double totalDividends, int holdDays, const IBString& futureExpiry,
	double dividendImpact, double dividendsToExpiry )
{
	// TODO
	DEBUG_PRINTF( "TICK_EFP: %ld %s", tickerId, ibToString(tickType).c_str() );
}

void TwsDlWrapper::orderStatus ( OrderId orderId,
	const IBString &status, double filled, double remaining, double avgFillPrice,
	int permId, int parentId, double lastFillPrice, int clientId,
	const IBString& whyHeld, double mktCapPrice)
{
#if 1
	DEBUG_PRINTF( "ORDER_STATUS: "
		"orderId:%ld, status:%s, filled:%g, remaining:%g, %d %d %g %g %d, %s",
		orderId, status.c_str(), filled, remaining, permId, parentId,
		avgFillPrice, lastFillPrice, clientId, whyHeld.c_str());
#endif
	RowOrderStatus row = { orderId, status, filled, remaining,
		avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld };
	parentTwsDL->twsOrderStatus(row);
}

void TwsDlWrapper::openOrder( OrderId orderId,
	const Contract &contract, const Order &order,
	const OrderState &orderState )
{
#if 1
	DEBUG_PRINTF( "OPEN_ORDER: %ld %s %s "
		"warnTxt:%s, status:%s, com:%g, comCur:%s, minCom:%g, maxCom:%g, "
		"iMargB:%s, mMargB:%s, ewlB:%s, "
		"iMargC:%s, mMargC:%s, ewlC:%s, "
		"iMargA:%s, mMargA:%s, ewlA:%s",
		orderId, contract.symbol.c_str(), order.action.c_str(),
		orderState.warningText.c_str(),
		orderState.status.c_str(),
		orderState.commission,
		orderState.commissionCurrency.c_str(),
		orderState.minCommission,
		orderState.maxCommission,
		orderState.initMarginBefore.c_str(),
		orderState.maintMarginBefore.c_str(),
		orderState.equityWithLoanBefore.c_str(),
		orderState.initMarginChange.c_str(),
		orderState.maintMarginChange.c_str(),
		orderState.equityWithLoanChange.c_str(),
		orderState.initMarginAfter.c_str(),
		orderState.maintMarginAfter.c_str(),
		orderState.equityWithLoanAfter.c_str() );
#endif
	RowOpenOrder row = { orderId, contract, order, orderState };
	parentTwsDL->twsOpenOrder(row);
}

void TwsDlWrapper::openOrderEnd()
{
#if 1
	DEBUG_PRINTF( "OPEN_ORDER_END" );
#endif
	parentTwsDL->twsOpenOrderEnd();
}

void TwsDlWrapper::winError( const IBString &str, int lastError )
{
	DEBUG_PRINTF( "WIN_ERROR: %s %d", str.c_str(), lastError );
}


void TwsDlWrapper::connectionClosed()
{
	//DEBUG_PRINTF( "CONNECTION_CLOSED" );
	parentTwsDL->twsConnectionClosed();
}

void TwsDlWrapper::updateAccountValue( const IBString& key,
	const IBString& val, const IBString& currency,
	const IBString& accountName )
{
#if 0
	DEBUG_PRINTF( "ACCT_VALUE: %s %s %s %s",
		key.c_str(), val.c_str(), currency.c_str(), accountName.c_str() );
#endif
	RowAccVal row = { key, val, currency, accountName };
	parentTwsDL->twsUpdateAccountValue( row );
}

void TwsDlWrapper::updatePortfolio( const Contract& contract,
	double position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IBString& accountName)
{
#if 0
	DEBUG_PRINTF( "PORTFOLIO_VALUE: %s %s %g %g %g %g %g %g %s",
		contract.symbol.c_str(), contract.localSymbol.c_str(), position,
		marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL,
		accountName.c_str() );
#endif
	RowPrtfl row = { contract, position, marketPrice, marketValue,
		averageCost, unrealizedPNL, realizedPNL, accountName};
	parentTwsDL->twsUpdatePortfolio( row );
}

void TwsDlWrapper::updateAccountTime( const IBString& timeStamp )
{
#if 0
	DEBUG_PRINTF( "ACCT_UPDATE_TIME: %s", timeStamp.c_str() );
#endif
	parentTwsDL->twsUpdateAccountTime( timeStamp );
}

void TwsDlWrapper::accountDownloadEnd( const IBString& accountName )
{
#if 1
	DEBUG_PRINTF( "ACCT_DOWNLOAD_END: %s", accountName.c_str() );
#endif
	parentTwsDL->twsAccountDownloadEnd( accountName );
}

void TwsDlWrapper::nextValidId( OrderId orderId )
{
#if 1
	DEBUG_PRINTF( "NEXT_VALID_ID: %ld", orderId );
#endif
	parentTwsDL->nextValidId( orderId );
}

void TwsDlWrapper::contractDetails( int reqId,
	const ContractDetails& contractDetails )
{
#if 0
	DEBUG_PRINTF( "CONTRACT_DATA: %d %s %s %s %g %s %s %s %s %s %s",
		reqId,
		contractDetails.summary.symbol.c_str(),
		contractDetails.summary.secType.c_str(),
		contractDetails.summary.lastTradeDateOrContractMonth.c_str(),
		contractDetails.summary.strike,
		contractDetails.summary.right.c_str(),
		contractDetails.summary.exchange.c_str(),
		contractDetails.summary.currency.c_str(),
		contractDetails.summary.localSymbol.c_str(),
		contractDetails.marketName.c_str(),
		contractDetails.summary.tradingClass.c_str()
		);
#endif
	parentTwsDL->twsContractDetails( reqId, contractDetails );
}

void TwsDlWrapper::bondContractDetails( int reqId,
	const ContractDetails& contractDetails )
{
#if 0
	//TODO
	DEBUG_PRINTF( "BOND_CONTRACT_DATA: %d %s %s %s %g %s %s %s %s %s %s",
		reqId,
		contractDetails.summary.symbol.c_str(),
		contractDetails.summary.secType.c_str(),
		contractDetails.summary.lastTradeDateOrContractMonth.c_str(),
		contractDetails.summary.strike,
		contractDetails.summary.right.c_str(),
		contractDetails.summary.exchange.c_str(),
		contractDetails.summary.currency.c_str(),
		contractDetails.summary.localSymbol.c_str(),
		contractDetails.marketName.c_str(),
		contractDetails.summary.tradingClass.c_str()
		);
#endif
	parentTwsDL->twsBondContractDetails(
		reqId, contractDetails );
}

void TwsDlWrapper::contractDetailsEnd( int reqId )
{
#if 0
	DEBUG_PRINTF( "CONTRACT_DATA_END: %d", reqId );
#endif
	parentTwsDL->twsContractDetailsEnd(reqId);
}

void TwsDlWrapper::execDetails ( int reqId, const Contract& contract,
	const Execution& execution )
{
#if 1
	DEBUG_PRINTF( "EXECUTION_DATA: %d %s %s %ld %s", reqId,
		contract.symbol.c_str(), contract.localSymbol.c_str(), contract.conId,
		ibToString(execution).c_str());
#endif
	RowExecution row = { contract, execution };
	parentTwsDL->twsExecDetails(  reqId, row );
}

void TwsDlWrapper::execDetailsEnd( int reqId )
{
#if 1
	DEBUG_PRINTF( "EXECUTION_DATA_END: %d", reqId );
#endif
	parentTwsDL->twsExecDetailsEnd( reqId );
}

void TwsDlWrapper::error(int id, int errorCode, const IBString& errorString)
{
#if 0
	DEBUG_PRINTF( "ERR_MSG: %d %d %s", id, errorCode, errorString.c_str() );
#endif
	RowError row = { id, errorCode, errorString };
	parentTwsDL->twsError( row );
}

void TwsDlWrapper::updateMktDepth( TickerId id, int position,
	int operation, int side, double price, int size )
{
	// TODO
	DEBUG_PRINTF( "MARKET_DEPTH: %ld", id );
}

void TwsDlWrapper::updateMktDepthL2( TickerId id, int position,
	const IBString& mktMaker, int operation, int side, double price, int size)
{
	// TODO
	DEBUG_PRINTF( "MARKET_DEPTH_L2: %ld", id );
}

void TwsDlWrapper::updateNewsBulletin( int msgId, int msgType,
	const IBString& newsMessage, const IBString& originExch )
{
	// TODO
	DEBUG_PRINTF( "NEWS_BULLETINS: %d", msgId );
}

void TwsDlWrapper::managedAccounts( const IBString& accountsList )
{
	DEBUG_PRINTF( "MANAGED_ACCTS: %s", accountsList.c_str() );
}

void TwsDlWrapper::receiveFA( faDataType pFaDataType,
	const IBString& cxml )
{
	// TODO
	DEBUG_PRINTF( "RECEIVE_FA: %s", cxml.c_str() );
}

void TwsDlWrapper::historicalData( TickerId reqId, const Bar& bar)
{
#if 0
	DEBUG_PRINTF( "HISTORICAL_DATA: %ld %s %g %g %g %g %lld %d %g", reqId,
		bar.time.c_str(), bar.open, bar.high, bar.low, bar.close, bar.volume, bar.count, bar.wap );
#endif
	/* TODO remove RowHist and use Bar directly */
	RowHist row = { bar.time, bar.open, bar.high, bar.low, bar.close, bar.volume, bar.count, bar.wap, false };
	parentTwsDL->twsHistoricalData( reqId, row );
}

void TwsDlWrapper::historicalDataEnd(int reqId,
	const IBString& startDateStr, const IBString& endDateStr)
{
#if 0
	DEBUG_PRINTF( "HISTORICAL_DATA_END: %d", reqId );
#endif
	RowHist row = dflt_RowHist;
	row.date = IBString("finished-") + startDateStr + "-" + endDateStr;
	parentTwsDL->twsHistoricalData( reqId, row );
}

void TwsDlWrapper::scannerParameters( const IBString &xml )
{
	DEBUG_PRINTF( "SCANNER_PARAMETERS: %s", xml.c_str() );
}


void TwsDlWrapper::scannerData( int reqId, int rank,
	const ContractDetails &contractDetails, const IBString &distance,
	const IBString &benchmark, const IBString &projection,
	const IBString &legsStr )
{
	// TODO
	DEBUG_PRINTF( "SCANNER_DATA: %d", reqId );
}

void TwsDlWrapper::scannerDataEnd(int reqId)
{
	DEBUG_PRINTF( "SCANNER_DATA_END: %d", reqId );
}

void TwsDlWrapper::realtimeBar( TickerId reqId, long time, double open,
	double high, double low, double close, long volume, double wap, int count )
{
	// TODO
	DEBUG_PRINTF( "REAL_TIME_BARS: %ld %ld", reqId, time );
}

void TwsDlWrapper::currentTime( long time )
{
#if 1
	DEBUG_PRINTF( "CURRENT_TIME: %ld", time );
#endif
	parentTwsDL->twsCurrentTime( time );
}

void TwsDlWrapper::fundamentalData( TickerId reqId,
	const IBString& data )
{
	DEBUG_PRINTF( "FUNDAMENTAL_DATA: %ld %s", reqId, data.c_str() );
}

void TwsDlWrapper::deltaNeutralValidation( int reqId,
	const DeltaNeutralContract& dnc )
{
	// TODO
	DEBUG_PRINTF( "DELTA_NEUTRAL_VALIDATION: %d", reqId );
}

void TwsDlWrapper::tickSnapshotEnd( int reqId )
{
	DEBUG_PRINTF( "TICK_SNAPSHOT_END: %d", reqId );
}

void TwsDlWrapper::marketDataType( TickerId reqId, int marketDataType )
{
	DEBUG_PRINTF( "MARKET_DATA_TYPE: %ld %d", reqId, marketDataType );
}

void TwsDlWrapper::commissionReport( const CommissionReport &cr )
{
	DEBUG_PRINTF( "COMMISSION_REPORT %s %g %s %g %g %d", cr.execId.c_str(),
		cr.commission, cr.currency.c_str(), cr.realizedPNL, cr.yield,
		cr.yieldRedemptionDate );
}

void TwsDlWrapper::position( const IBString& account,
	const Contract& c, double pos, double avgCost )
{
	DEBUG_PRINTF( "POSITION: %s %s %s %g %g", account.c_str(),
		c.symbol.c_str(), c.localSymbol.c_str(), pos, avgCost );
}

void TwsDlWrapper::positionEnd()
{
	DEBUG_PRINTF( "POSITION_END" );
}

void TwsDlWrapper::accountSummary( int reqId, const IBString& account,
	const IBString& tag, const IBString& value,
	const IBString& currency )
{
	DEBUG_PRINTF( "ACCOUNT_SUMMARY: %d %s %s %s %s", reqId, account.c_str(),
		tag.c_str(), value.c_str(), currency.c_str() );
}

void TwsDlWrapper::accountSummaryEnd( int reqId )
{
	DEBUG_PRINTF( "ACCOUNT_SUMMARY_END: %d", reqId );
}

void TwsDlWrapper::verifyMessageAPI( const IBString& apiData)
{
	DEBUG_PRINTF( "VERIFY_MESSAGE_API: %s", apiData.c_str() );
}

void TwsDlWrapper::verifyCompleted( bool isSuccessful, const IBString& errorText)
{
	DEBUG_PRINTF( "VERIFY_COMPLETED: %s %s", isSuccessful ? "true" : "false",
		errorText.c_str() );
}

void TwsDlWrapper::displayGroupList( int reqId, const IBString& groups)
{
	DEBUG_PRINTF( "DISPLAY_GROUP_LIST: %d %s", reqId, groups.c_str() );
}

void TwsDlWrapper::displayGroupUpdated( int reqId, const IBString& contractInfo)
{
	DEBUG_PRINTF( "DISPLAY_GROUP_UPDATED: %d %s", reqId, contractInfo.c_str() );
}

void TwsDlWrapper::verifyAndAuthMessageAPI( const IBString& apiData,
	const IBString& xyzChallange)
{
}

void TwsDlWrapper::verifyAndAuthCompleted( bool isSuccessful,
	const IBString& errorText)
{
}

void TwsDlWrapper::connectAck()
{
#if 1
	DEBUG_PRINTF( "CONNECT_ACK");
#endif
	parentTwsDL->twsConnectAck();
}

void TwsDlWrapper::positionMulti( int reqId, const std::string& account,
	const std::string& modelCode, const Contract& contract,
	double pos, double avgCost)
{
}

void TwsDlWrapper::positionMultiEnd( int reqId)
{
}

void TwsDlWrapper::accountUpdateMulti( int reqId, const std::string& account,
	const std::string& modelCode, const std::string& key,
	const std::string& value, const std::string& currency)
{
}

void TwsDlWrapper::accountUpdateMultiEnd( int reqId)
{
}
void TwsDlWrapper::securityDefinitionOptionalParameter(int reqId,
	const std::string& exchange, int underlyingConId,
	const std::string& tradingClass, const std::string& multiplier,
	const std::set<std::string>& expirations, const std::set<double>& strikes)
{
}

void TwsDlWrapper::securityDefinitionOptionalParameterEnd(int reqId)
{
}

void TwsDlWrapper::softDollarTiers(int reqId,
	const std::vector<SoftDollarTier> &tiers)
{
}

void TwsDlWrapper::familyCodes(const std::vector<FamilyCode> &familyCodes)
{
}

void TwsDlWrapper::symbolSamples(int reqId,
	const std::vector<ContractDescription> &contractDescriptions)
{
}

void TwsDlWrapper::mktDepthExchanges(
	const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)
{
}

void TwsDlWrapper::tickNews(int tickerId, time_t timeStamp,
	const std::string& providerCode, const std::string& articleId,
	const std::string& headline, const std::string& extraData)
{
}

void TwsDlWrapper::smartComponents(int reqId,
	const SmartComponentsMap& theMap)
{
}

void TwsDlWrapper::tickReqParams(int tickerId,
	double minTick,	const std::string& bboExchange, int snapshotPermissions)
{
}

void TwsDlWrapper::newsProviders(const std::vector<NewsProvider> &newsProviders)
{
}

void TwsDlWrapper::newsArticle(int requestId, int articleType,
	const std::string& articleText)
{
}

void TwsDlWrapper::historicalNews(int requestId, const std::string& time,
	const std::string& providerCode, const std::string& articleId,
	const std::string& headline)
{
}

void TwsDlWrapper::historicalNewsEnd(int requestId, bool hasMore)
{
}

void TwsDlWrapper::headTimestamp(int reqId, const std::string& headTimestamp)
{
}

void TwsDlWrapper::histogramData(int reqId, const HistogramDataVector& data)
{
}

void TwsDlWrapper::historicalDataUpdate(TickerId reqId, const Bar& bar)
{
}

void TwsDlWrapper::rerouteMktDataReq(int reqId, int conid,
	const std::string& exchange)
{
}

void TwsDlWrapper::rerouteMktDepthReq(int reqId, int conid,
	const std::string& exchange)
{
}

void TwsDlWrapper::marketRule(int marketRuleId,
	const std::vector<PriceIncrement> &priceIncrements)
{
}

void TwsDlWrapper::pnl(int reqId, double dailyPnL, double unrealizedPnL,
	double realizedPnL)
{
}

void TwsDlWrapper::pnlSingle(int reqId, int pos, double dailyPnL,
	double unrealizedPnL, double realizedPnL, double value)
{
}

void TwsDlWrapper::historicalTicks(int reqId,
	const std::vector<HistoricalTick> &ticks, bool done)
{
}

void TwsDlWrapper::historicalTicksBidAsk(int reqId,
	const std::vector<HistoricalTickBidAsk> &ticks, bool done)
{
}

void TwsDlWrapper::historicalTicksLast(int reqId,
	const std::vector<HistoricalTickLast> &ticks, bool done)
{
}

void TwsDlWrapper::tickByTickAllLast(int reqId, int tickType, time_t time,
	double price, int size, const TickAttrib& attribs,
	const std::string& exchange, const std::string& specialConditions)
{
}

void TwsDlWrapper::tickByTickBidAsk(int reqId, time_t time, double bidPrice,
	double askPrice, int bidSize, int askSize,
	const TickAttrib& attribs)
{
}

void TwsDlWrapper::tickByTickMidPoint(int reqId, time_t time, double midPoint)
{
}

void TwsDlWrapper::orderBound(long long orderId, int apiClientId,
	int apiOrderId)
{
}
