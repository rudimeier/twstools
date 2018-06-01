/*** tws_wrapper.cpp -- simple "IB/API wrapper" just for debugging
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_wrapper.h"
#include "tws_util.h"
#include "debug.h"
#include "config.h"

#include <twsapi/twsapi_config.h>
#include <twsapi/Contract.h>
#include <twsapi/Order.h>
#include <twsapi/OrderState.h>
#include <twsapi/CommissionReport.h>

#if TWSAPI_IB_VERSION_NUMBER < 97200
# define lastTradeDateOrContractMonth expiry
#endif

DebugTwsWrapper::~DebugTwsWrapper()
{
}





void DebugTwsWrapper::tickPrice( TickerId tickerId, TickType field,
	double price
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	, const TickAttrib& ta)
#else
	, int canAutoExecute)
#endif
{
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	int canAutoExecute = ta.canAutoExecute;
#endif
	DEBUG_PRINTF( "TICK_PRICE: %ld %s %g %d",
		tickerId, ibToString(field).c_str(), price, canAutoExecute);
}


void DebugTwsWrapper::tickSize( TickerId tickerId, TickType field,
	int size )
{
	DEBUG_PRINTF( "TICK_SIZE: %ld %s %d",
		tickerId, ibToString(field).c_str(), size );
}


void DebugTwsWrapper::tickOptionComputation ( TickerId tickerId,
	TickType tickType, double impliedVol, double delta, double optPrice,
	double pvDividend, double gamma, double vega, double theta,
	double undPrice )
{
	DEBUG_PRINTF( "TICK_OPTION_COMPUTATION: %ld %s %g %g %g %g %g %g %g %g",
		tickerId, ibToString(tickType).c_str(), impliedVol, delta,
		optPrice, pvDividend, gamma, vega, theta, undPrice );
}


void DebugTwsWrapper::tickGeneric( TickerId tickerId, TickType tickType,
	double value )
{
	DEBUG_PRINTF( "TICK_GENERIC: %ld %s %g",
		tickerId, ibToString(tickType).c_str(), value );
}


void DebugTwsWrapper::tickString( TickerId tickerId, TickType tickType,
	const IBString& value )
{
	if( tickType == LAST_TIMESTAMP ) {
		// here we format unix timestamp value
	}
	DEBUG_PRINTF( "TICK_STRING: %ld %s %s",
		tickerId, ibToString(tickType).c_str(), value.c_str() );
}


void DebugTwsWrapper::tickEFP( TickerId tickerId, TickType tickType,
	double basisPoints, const IBString& formattedBasisPoints,
	double totalDividends, int holdDays, const IBString& futureExpiry,
	double dividendImpact, double dividendsToExpiry )
{
	// TODO
	DEBUG_PRINTF( "TICK_EFP: %ld %s", tickerId, ibToString(tickType).c_str() );
}


void DebugTwsWrapper::orderStatus ( OrderId orderId,
	const IBString &status, POS_TYPE filled, POS_TYPE remaining, double avgFillPrice,
	int permId, int parentId, double lastFillPrice, int clientId,
	const IBString& whyHeld
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	, double mktCapPrice
#endif
)
{
	DEBUG_PRINTF( "ORDER_STATUS: "
		"orderId:%ld, status:%s, filled:%g, remaining:%g, %d %d %g %g %d, %s",
		orderId, status.c_str(), (double)filled, (double)remaining, permId, parentId,
		avgFillPrice, lastFillPrice, clientId, whyHeld.c_str());
}


void DebugTwsWrapper::openOrder( OrderId orderId,
	const Contract &contract, const Order &order,
	const OrderState &orderState )
{
	DEBUG_PRINTF( "OPEN_ORDER: %ld %s %s "
		"warnTxt:%s, status:%s, com:%g, comCur:%s, minCom:%g, maxCom:%g, "
#if TWSAPI_VERSION_NUMBER >= 17300
		"iMargB:%s, mMargB:%s, ewlB:%s, "
		"iMargC:%s, mMargC:%s, ewlC:%s, "
#endif
		"iMargA:%s, mMargA:%s, ewlA:%s",
		orderId, contract.symbol.c_str(), order.action.c_str(),
		orderState.warningText.c_str(),
		orderState.status.c_str(),
		orderState.commission,
		orderState.commissionCurrency.c_str(),
		orderState.minCommission,
		orderState.maxCommission,
#if TWSAPI_VERSION_NUMBER >= 17300
		orderState.initMarginBefore.c_str(),
		orderState.maintMarginBefore.c_str(),
		orderState.equityWithLoanBefore.c_str(),
		orderState.initMarginChange.c_str(),
		orderState.maintMarginChange.c_str(),
		orderState.equityWithLoanChange.c_str(),
#endif
		orderState.initMarginAfter.c_str(),
		orderState.maintMarginAfter.c_str(),
		orderState.equityWithLoanAfter.c_str() );
}


void DebugTwsWrapper::openOrderEnd()
{
	DEBUG_PRINTF( "OPEN_ORDER_END" );
}


void DebugTwsWrapper::winError( const IBString &str, int lastError )
{
	DEBUG_PRINTF( "WIN_ERROR: %s %d", str.c_str(), lastError );
}


void DebugTwsWrapper::connectionClosed()
{
	DEBUG_PRINTF( "CONNECTION_CLOSED" );
}


void DebugTwsWrapper::updateAccountValue( const IBString& key,
	const IBString& val, const IBString& currency,
	const IBString& accountName )
{
	DEBUG_PRINTF( "ACCT_VALUE: %s %s %s %s",
		key.c_str(), val.c_str(), currency.c_str(), accountName.c_str() );
}


void DebugTwsWrapper::updatePortfolio( const Contract& contract,
	POS_TYPE position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IBString& accountName)
{
	DEBUG_PRINTF( "PORTFOLIO_VALUE: %s %s %g %g %g %g %g %g %s",
		contract.symbol.c_str(), contract.localSymbol.c_str(), (double)position,
		marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL,
		accountName.c_str() );
}


void DebugTwsWrapper::updateAccountTime( const IBString& timeStamp )
{
	DEBUG_PRINTF( "ACCT_UPDATE_TIME: %s", timeStamp.c_str() );
}


void DebugTwsWrapper::accountDownloadEnd( const IBString& accountName )
{
	DEBUG_PRINTF( "ACCT_DOWNLOAD_END: %s", accountName.c_str() );
}


void DebugTwsWrapper::nextValidId( OrderId orderId )
{
	DEBUG_PRINTF( "NEXT_VALID_ID: %ld", orderId );
}


void DebugTwsWrapper::contractDetails( int reqId,
	const ContractDetails& contractDetails )
{
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
}


void DebugTwsWrapper::bondContractDetails( int reqId,
	const ContractDetails& contractDetails )
{
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
}


void DebugTwsWrapper::contractDetailsEnd( int reqId )
{
	DEBUG_PRINTF( "CONTRACT_DATA_END: %d", reqId );
}


void DebugTwsWrapper::execDetails ( int orderId, const Contract& contract,
	const Execution& execution )
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
#if TWSAPI_VERSION_NUMBER >= 17300
	const IBString& errorMsg
#else
	const IBString errorMsg
#endif
	)
{
	DEBUG_PRINTF( "ERR_MSG: %d %d %s", id, errorCode, errorMsg.c_str() );
}


void DebugTwsWrapper::updateMktDepth( TickerId id, int position,
	int operation, int side, double price, int size )
{
	// TODO
	DEBUG_PRINTF( "MARKET_DEPTH: %ld", id );
}


void DebugTwsWrapper::updateMktDepthL2( TickerId id, int position,
#if TWSAPI_VERSION_NUMBER >= 17300
	const IBString& marketMaker,
#else
	IBString marketMaker,
#endif
	int operation, int side, double price, int size )
{
	// TODO
	DEBUG_PRINTF( "MARKET_DEPTH_L2: %ld", id );
}


void DebugTwsWrapper::updateNewsBulletin( int msgId, int msgType,
	const IBString& newsMessage, const IBString& originExch )
{
	// TODO
	DEBUG_PRINTF( "NEWS_BULLETINS: %d", msgId );
}


void DebugTwsWrapper::managedAccounts( const IBString& accountsList )
{
	DEBUG_PRINTF( "MANAGED_ACCTS: %s", accountsList.c_str() );
}


void DebugTwsWrapper::receiveFA( faDataType pFaDataType,
	const IBString& cxml )
{
	// TODO
	DEBUG_PRINTF( "RECEIVE_FA: %s", cxml.c_str() );
}

#if TWSAPI_IB_VERSION_NUMBER >= 97300
void DebugTwsWrapper::historicalData( TickerId reqId,
#if TWSAPI_VERSION_NUMBER >= 17300
	const Bar& bar
#else
	Bar bar
#endif
	)
{
	DEBUG_PRINTF( "HISTORICAL_DATA: %ld %s %g %g %g %g %lld %d %g", reqId,
		bar.time.c_str(), bar.open, bar.high, bar.low, bar.close, bar.volume, bar.count, bar.wap );
}

void DebugTwsWrapper::historicalDataEnd(int reqId,
#if TWSAPI_VERSION_NUMBER >= 17300
	const IBString& startDateStr, const IBString& endDateStr
#else
	IBString startDateStr, IBString endDateStr
#endif
	)
{
}
#else
void DebugTwsWrapper::historicalData( TickerId reqId,
	const IBString& date, double open, double high, double low,
	double close, int volume, int barCount, double WAP, int hasGaps )
{
	DEBUG_PRINTF( "HISTORICAL_DATA: %ld %s %g %g %g %g %d %d %g %d", reqId,
		date.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps );
}
#endif

void DebugTwsWrapper::scannerParameters( const IBString &xml )
{
	DEBUG_PRINTF( "SCANNER_PARAMETERS: %s", xml.c_str() );
}


void DebugTwsWrapper::scannerData( int reqId, int rank,
	const ContractDetails &contractDetails, const IBString &distance,
	const IBString &benchmark, const IBString &projection,
	const IBString &legsStr )
{
	// TODO
	DEBUG_PRINTF( "SCANNER_DATA: %d", reqId );
}


void DebugTwsWrapper::scannerDataEnd(int reqId)
{
	DEBUG_PRINTF( "SCANNER_DATA_END: %d", reqId );
}


void DebugTwsWrapper::realtimeBar( TickerId reqId, long time, double open,
	double high, double low, double close, long volume, double wap, int count )
{
	// TODO
	DEBUG_PRINTF( "REAL_TIME_BARS: %ld %ld", reqId, time );
}


void DebugTwsWrapper::currentTime( long time )
{
	DEBUG_PRINTF( "CURRENT_TIME: %ld", time );
}


void DebugTwsWrapper::fundamentalData( TickerId reqId,
	const IBString& data )
{
	DEBUG_PRINTF( "FUNDAMENTAL_DATA: %ld %s", reqId, data.c_str() );
}


void DebugTwsWrapper::deltaNeutralValidation( int reqId,
	const DeltaNeutralContract& dnc )
{
	// TODO
	DEBUG_PRINTF( "DELTA_NEUTRAL_VALIDATION: %d", reqId );
}


void DebugTwsWrapper::tickSnapshotEnd( int reqId )
{
	DEBUG_PRINTF( "TICK_SNAPSHOT_END: %d", reqId );
}

void DebugTwsWrapper::marketDataType( TickerId reqId, int marketDataType )
{
	DEBUG_PRINTF( "MARKET_DATA_TYPE: %ld %d", reqId, marketDataType );
}

void DebugTwsWrapper::commissionReport( const CommissionReport &cr )
{
	DEBUG_PRINTF( "COMMISSION_REPORT %s %g %s %g %g %d", cr.execId.c_str(),
		cr.commission, cr.currency.c_str(), cr.realizedPNL, cr.yield,
		cr.yieldRedemptionDate );
}

void DebugTwsWrapper::position( const IBString& account,
	const Contract& c, POS_TYPE pos, double avgCost )
{
	DEBUG_PRINTF( "POSITION: %s %s %s %g %g", account.c_str(),
		c.symbol.c_str(), c.localSymbol.c_str(), (double)pos, avgCost );
}

void DebugTwsWrapper::positionEnd()
{
	DEBUG_PRINTF( "POSITION_END" );
}

void DebugTwsWrapper::accountSummary( int reqId, const IBString& account,
	const IBString& tag, const IBString& value,
	const IBString& currency )
{
	DEBUG_PRINTF( "ACCOUNT_SUMMARY: %d %s %s %s %s", reqId, account.c_str(),
		tag.c_str(), value.c_str(), currency.c_str() );
}

void DebugTwsWrapper::accountSummaryEnd( int reqId )
{
	DEBUG_PRINTF( "ACCOUNT_SUMMARY_END: %d", reqId );
}

#if TWSAPI_IB_VERSION_NUMBER >= 971
void DebugTwsWrapper::verifyMessageAPI( const IBString& apiData)
{
	DEBUG_PRINTF( "VERIFY_MESSAGE_API: %s", apiData.c_str() );
}

void DebugTwsWrapper::verifyCompleted( bool isSuccessful, const IBString& errorText)
{
	DEBUG_PRINTF( "VERIFY_COMPLETED: %s %s", isSuccessful ? "true" : "false",
		errorText.c_str() );
}

void DebugTwsWrapper::displayGroupList( int reqId, const IBString& groups)
{
	DEBUG_PRINTF( "DISPLAY_GROUP_LIST: %d %s", reqId, groups.c_str() );
}

void DebugTwsWrapper::displayGroupUpdated( int reqId, const IBString& contractInfo)
{
	DEBUG_PRINTF( "DISPLAY_GROUP_UPDATED: %d %s", reqId, contractInfo.c_str() );
}
#endif

#if TWSAPI_IB_VERSION_NUMBER >= 97200
void DebugTwsWrapper::verifyAndAuthMessageAPI( const IBString& apiData,
	const IBString& xyzChallange)
{
}
void DebugTwsWrapper::verifyAndAuthCompleted( bool isSuccessful,
	const IBString& errorText)
{
}

void DebugTwsWrapper::connectAck()
{
	DEBUG_PRINTF( "CONNECT_ACK");
}
void DebugTwsWrapper::positionMulti( int reqId, const std::string& account,
	const std::string& modelCode, const Contract& contract,
	double pos, double avgCost)
{
}
void DebugTwsWrapper::positionMultiEnd( int reqId)
{
}
void DebugTwsWrapper::accountUpdateMulti( int reqId, const std::string& account,
	const std::string& modelCode, const std::string& key,
	const std::string& value, const std::string& currency)
{
}
void DebugTwsWrapper::accountUpdateMultiEnd( int reqId)
{
}
void DebugTwsWrapper::securityDefinitionOptionalParameter(int reqId,
	const std::string& exchange, int underlyingConId,
	const std::string& tradingClass, const std::string& multiplier,
#if TWSAPI_VERSION_NUMBER >= 17300
	const std::set<std::string>& expirations, const std::set<double>& strikes
#else
	std::set<std::string> expirations, std::set<double> strikes
#endif
	)
{
}
void DebugTwsWrapper::securityDefinitionOptionalParameterEnd(int reqId)
{
}
void DebugTwsWrapper::softDollarTiers(int reqId,
	const std::vector<SoftDollarTier> &tiers)
{
}
#endif

#if TWSAPI_IB_VERSION_NUMBER >= 97300
void DebugTwsWrapper::familyCodes(const std::vector<FamilyCode> &familyCodes)
{
}
void DebugTwsWrapper::symbolSamples(int reqId,
	const std::vector<ContractDescription> &contractDescriptions)
{
}
void DebugTwsWrapper::mktDepthExchanges(
	const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)
{
}
void DebugTwsWrapper::tickNews(int tickerId, time_t timeStamp,
	const std::string& providerCode, const std::string& articleId,
	const std::string& headline, const std::string& extraData)
{
}
void DebugTwsWrapper::smartComponents(int reqId,
#if TWSAPI_VERSION_NUMBER >= 17300
	const SmartComponentsMap& theMap
#else
	SmartComponentsMap theMap
#endif
	)
{
}
void DebugTwsWrapper::tickReqParams(int tickerId,
	double minTick,
#if TWSAPI_VERSION_NUMBER >= 17300
	const std::string& bboExchange,
#else
	std::string bboExchange,
#endif
	int snapshotPermissions)
{
}
void DebugTwsWrapper::newsProviders(const std::vector<NewsProvider> &newsProviders)
{
}
void DebugTwsWrapper::newsArticle(int requestId, int articleType,
	const std::string& articleText)
{
}
void DebugTwsWrapper::historicalNews(int requestId, const std::string& time,
	const std::string& providerCode, const std::string& articleId,
	const std::string& headline)
{
}
void DebugTwsWrapper::historicalNewsEnd(int requestId, bool hasMore)
{
}
void DebugTwsWrapper::headTimestamp(int reqId, const std::string& headTimestamp)
{
}
void DebugTwsWrapper::histogramData(int reqId,
#if TWSAPI_VERSION_NUMBER >= 17300
	const HistogramDataVector& data
#else
	HistogramDataVector data
#endif
	)
{
}
void DebugTwsWrapper::historicalDataUpdate(TickerId reqId,
#if TWSAPI_VERSION_NUMBER >= 17300
	const Bar& bar
#else
	Bar bar
#endif
	)
{
}
void DebugTwsWrapper::rerouteMktDataReq(int reqId, int conid,
	const std::string& exchange)
{
}
void DebugTwsWrapper::rerouteMktDepthReq(int reqId, int conid,
	const std::string& exchange)
{
}
void DebugTwsWrapper::marketRule(int marketRuleId,
	const std::vector<PriceIncrement> &priceIncrements)
{
}
void DebugTwsWrapper::pnl(int reqId, double dailyPnL, double unrealizedPnL,
	double realizedPnL)
{
}
void DebugTwsWrapper::pnlSingle(int reqId, int pos, double dailyPnL,
	double unrealizedPnL, double realizedPnL, double value)
{
}
void DebugTwsWrapper::historicalTicks(int reqId,
	const std::vector<HistoricalTick> &ticks, bool done)
{
}
void DebugTwsWrapper::historicalTicksBidAsk(int reqId,
	const std::vector<HistoricalTickBidAsk> &ticks, bool done)
{
}
void DebugTwsWrapper::historicalTicksLast(int reqId,
	const std::vector<HistoricalTickLast> &ticks, bool done)
{
}
void DebugTwsWrapper::tickByTickAllLast(int reqId, int tickType, time_t time,
	double price, int size, const TickAttrib& attribs,
	const std::string& exchange, const std::string& specialConditions)
{
}
void DebugTwsWrapper::tickByTickBidAsk(int reqId, time_t time, double bidPrice,
	double askPrice, int bidSize, int askSize,
	const TickAttrib& attribs)
{
}
void DebugTwsWrapper::tickByTickMidPoint(int reqId, time_t time, double midPoint)
{
}

#if TWSAPI_IB_VERSION_NUMBER >= 97307
void DebugTwsWrapper::orderBound(long long orderId, int apiClientId,
	int apiOrderId)
{
}
#endif

#endif
