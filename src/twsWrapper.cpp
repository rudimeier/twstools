#include "twsWrapper.h"
#include "twsUtil.h"
#include "debug.h"

// from global installed ibapi
#include "ibtws/Contract.h"
#include "ibtws/Order.h"
#include "ibtws/OrderState.h"


DebugTwsWrapper::~DebugTwsWrapper()
{
}





void DebugTwsWrapper::tickPrice( IB::TickerId tickerId, IB::TickType field,
	double price, int canAutoExecute )
{
	qDebug() << "TICK_PRICE:" << tickerId << ibToString(field) << price
		<< canAutoExecute;
}


void DebugTwsWrapper::tickSize( IB::TickerId tickerId, IB::TickType field,
	int size )
{
	qDebug() << "TICK_SIZE:" << tickerId << ibToString(field) << size;
}


void DebugTwsWrapper::tickOptionComputation ( IB::TickerId tickerId,
	IB::TickType tickType, double impliedVol, double delta, double optPrice,
	double pvDividend, double gamma, double vega, double theta,
	double undPrice )
{
	qDebug() << "TICK_OPTION_COMPUTATION:" << tickerId << ibToString(tickType)
		<< impliedVol << delta << optPrice << pvDividend << gamma << vega
		<< theta << undPrice;
}


void DebugTwsWrapper::tickGeneric( IB::TickerId tickerId, IB::TickType tickType,
	double value )
{
	qDebug() << "TICK_GENERIC:" << tickerId << ibToString(tickType) << value;
}


void DebugTwsWrapper::tickString( IB::TickerId tickerId, IB::TickType tickType,
	const IB::IBString& value )
{
	QString _val = toQString(value);
	if( tickType == IB::LAST_TIMESTAMP ) {
		_val = QDateTime::fromTime_t(_val.toInt()).toString();
	}
	qDebug() << "TICK_STRING:" << tickerId << ibToString(tickType) << _val;
}


void DebugTwsWrapper::tickEFP( IB::TickerId tickerId, IB::TickType tickType,
	double basisPoints, const IB::IBString& formattedBasisPoints,
	double totalDividends, int holdDays, const IB::IBString& futureExpiry,
	double dividendImpact, double dividendsToExpiry )
{
	// TODO
	qDebug() << "TICK_EFP:";
}


void DebugTwsWrapper::orderStatus ( IB::OrderId orderId,
	const IB::IBString &status, int filled, int remaining, double avgFillPrice,
	int permId, int parentId, double lastFillPrice, int clientId,
	const IB::IBString& whyHeld )
{
	qDebug() << "ORDER_STATUS:"
		<< QString("orderId:%1, status:%2, filled:%3, remaining:%4, ")
		.arg(orderId).arg(toQString(status)).arg(filled).arg(remaining)
		<< permId << parentId << avgFillPrice << lastFillPrice << clientId
		<< toQString(whyHeld);
}


void DebugTwsWrapper::openOrder( IB::OrderId orderId,
	const IB::Contract &contract, const IB::Order &order,
	const IB::OrderState &orderState )
{
	qDebug() << "OPEN_ORDER:" << orderId << toQString(contract.symbol)
		<< toQString(order.action)
		<< "warnTxt:" << toQString(orderState.warningText)
		<< "status:" << toQString(orderState.status)
		<< "com:" << orderState.commission
		<< "comCur" << toQString(orderState.commissionCurrency)
		<< "minCom:" << orderState.minCommission
		<< "maxCom:" << orderState.maxCommission
		<< "initMarg:" << toQString(orderState.initMargin)
		<< "maintMarg:" << toQString(orderState.maintMargin)
		<< "ewl:" << toQString(orderState.equityWithLoan);
}


void DebugTwsWrapper::openOrderEnd()
{
	qDebug() << "OPEN_ORDER_END";
}


void DebugTwsWrapper::winError( const IB::IBString &str, int lastError )
{
	qDebug() << "WIN_ERROR" << toQString(str) << lastError;
}


void DebugTwsWrapper::connectionClosed()
{
	qDebug() << "CONNECTION_CLOSED";
}


void DebugTwsWrapper::updateAccountValue( const IB::IBString& key,
	const IB::IBString& val, const IB::IBString& currency,
	const IB::IBString& accountName )
{
	qDebug() << "ACCT_VALUE:" << toQString(key) << toQString(val)
		<< toQString(currency) << toQString(accountName);
}


void DebugTwsWrapper::updatePortfolio( const IB::Contract& contract,
	int position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IB::IBString& accountName)
{
	qDebug() << "PORTFOLIO_VALUE:" << toQString(contract.symbol)
		<< toQString(contract.localSymbol) << position << marketPrice
		<< marketValue << averageCost << unrealizedPNL << realizedPNL
		<< toQString(accountName);
}


void DebugTwsWrapper::updateAccountTime( const IB::IBString& timeStamp )
{
	qDebug() << "ACCT_UPDATE_TIME:" << toQString(timeStamp);
}


void DebugTwsWrapper::accountDownloadEnd( const IB::IBString& accountName )
{
	qDebug() << "ACCT_DOWNLOAD_END:" << toQString(accountName);
}


void DebugTwsWrapper::nextValidId( IB::OrderId orderId )
{
	qDebug() << "NEXT_VALID_ID:" << orderId;
}


void DebugTwsWrapper::contractDetails( int reqId,
	const IB::ContractDetails& contractDetails )
{
	qDebug() << "CONTRACT_DATA:" << reqId
		<< toQString(contractDetails.summary.symbol)
		<< toQString(contractDetails.summary.secType)
		<< toQString(contractDetails.summary.expiry)
		<< contractDetails.summary.strike
		<< toQString(contractDetails.summary.right)
		<< toQString(contractDetails.summary.exchange)
		<< toQString(contractDetails.summary.currency)
		<< toQString(contractDetails.summary.localSymbol)
		<< toQString(contractDetails.marketName)
		<< toQString(contractDetails.tradingClass)
		<< toQString(contractDetails.validExchanges);
}


void DebugTwsWrapper::bondContractDetails( int reqId,
	const IB::ContractDetails& contractDetails )
{
	//TODO
	qDebug() << "BOND_CONTRACT_DATA:" << reqId
		<< toQString(contractDetails.summary.symbol)
		<< toQString(contractDetails.summary.secType)
		<< toQString(contractDetails.summary.expiry)
		<< contractDetails.summary.strike
		<< toQString(contractDetails.summary.right)
		<< toQString(contractDetails.summary.exchange)
		<< toQString(contractDetails.summary.currency)
		<< toQString(contractDetails.summary.localSymbol)
		<< toQString(contractDetails.marketName)
		<< toQString(contractDetails.tradingClass)
		<< toQString(contractDetails.validExchanges);
}


void DebugTwsWrapper::contractDetailsEnd( int reqId )
{
	qDebug() << "CONTRACT_DATA_END:" << reqId;
}


void DebugTwsWrapper::execDetails ( int orderId, const IB::Contract& contract,
	const IB::Execution& execution )
{
	qDebug() << "EXECUTION_DATA:" << orderId
		<< toQString(contract.localSymbol) << ibToString(execution);
}


void DebugTwsWrapper::execDetailsEnd( int reqId )
{
	qDebug() << "EXECUTION_DATA_END:" << reqId;
}


void DebugTwsWrapper::error( int id, int errorCode,
	const IB::IBString errorMsg )
{
	qDebug() << "ERR_MSG:" << id << errorCode << toQString(errorMsg);
}


void DebugTwsWrapper::updateMktDepth( IB::TickerId id, int position,
	int operation, int side, double price, int size )
{
	// TODO
	qDebug() << "MARKET_DEPTH:";
}


void DebugTwsWrapper::updateMktDepthL2( IB::TickerId id, int position,
	IB::IBString marketMaker, int operation, int side, double price,
	int size )
{
	// TODO
	qDebug() << "MARKET_DEPTH_L2:";
}


void DebugTwsWrapper::updateNewsBulletin( int msgId, int msgType,
	const IB::IBString& newsMessage, const IB::IBString& originExch )
{
	// TODO
	qDebug() << "NEWS_BULLETINS:";
}


void DebugTwsWrapper::managedAccounts( const IB::IBString& accountsList )
{
	qDebug() << "MANAGED_ACCTS:" << toQString(accountsList);
}


void DebugTwsWrapper::receiveFA( IB::faDataType pFaDataType,
	const IB::IBString& cxml )
{
	// TODO
	qDebug() << "RECEIVE_FA:" << toQString(cxml);
}


void DebugTwsWrapper::historicalData( IB::TickerId reqId,
	const IB::IBString& date, double open, double high, double low,
	double close, int volume, int barCount, double WAP, int hasGaps )
{
	qDebug() << "HISTORICAL_DATA:" << reqId << toQString(date) << open << high
		<< low << close << volume << barCount << WAP << hasGaps;
}


void DebugTwsWrapper::scannerParameters( const IB::IBString &xml )
{
	qDebug() << "SCANNER_PARAMETERS:" << toQString(xml);
}


void DebugTwsWrapper::scannerData( int reqId, int rank,
	const IB::ContractDetails &contractDetails, const IB::IBString &distance,
	const IB::IBString &benchmark, const IB::IBString &projection,
	const IB::IBString &legsStr )
{
	// TODO
	qDebug() << "SCANNER_DATA:" << reqId;
}


void DebugTwsWrapper::scannerDataEnd(int reqId)
{
	qDebug() << "SCANNER_DATA_END:" << reqId;
}


void DebugTwsWrapper::realtimeBar( IB::TickerId reqId, long time, double open,
	double high, double low, double close, long volume, double wap, int count )
{
	// TODO
	qDebug() << "REAL_TIME_BARS:" << reqId;
}


void DebugTwsWrapper::currentTime( long time )
{
	qDebug() << "CURRENT_TIME:" << time; //QDateTime:: fromTime_t(time);
}


void DebugTwsWrapper::fundamentalData( IB::TickerId reqId,
	const IB::IBString& data )
{
	qDebug() << "FUNDAMENTAL_DATA:" << reqId << toQString(data);
}


void DebugTwsWrapper::deltaNeutralValidation( int reqId,
	const IB::UnderComp& underComp )
{
	// TODO
	qDebug() << "DELTA_NEUTRAL_VALIDATION:" << reqId;
}


void DebugTwsWrapper::tickSnapshotEnd( int reqId )
{
	qDebug() << "TICK_SNAPSHOT_END:" << reqId;
}
