#include "twsWrapper.h"
#include "twsUtil.h"
#include "utilities/debug.h"

// from global installed ibapi
#include "ibtws/EWrapper.h"
#include "ibtws/Contract.h"
#include "ibtws/Order.h"
#include "ibtws/OrderState.h"


TWSWrapper::~TWSWrapper() {
	qDebug() << "Destructor TWSWrapper called/finished";
}

void TWSWrapper::moveMeToThread(QThread *thread) {
			this->moveToThread(thread);
}

void TWSWrapper::connectAllSignals(QObject *a, QObject *b, Qt::ConnectionType type){

	connectError                 (a ,b, type);
	connectTickPrice             (a, b, type);
	connectTickSize              (a, b, type);
	connectOrderStatus           (a, b, type);
	connectOpenOrder             (a, b, type);
	connectUpdateAccountValue    (a, b, type);
	connectUpdatePortfolio       (a, b, type);
	connectUpdateAccountTime     (a, b, type);
	connectNextValidId           (a, b, type);
	connectContractDetails       (a, b, type);
	connectContractDetailsEnd    (a, b, type);
	connectExecDetails           (a, b, type);
	connectManagedAccounts       (a, b, type);
	connectHistoricalData        (a, b, type);
	connectTickOptionComputation (a, b, type);
	connectTickGeneric           (a, b, type);
	connectTickString            (a, b, type);
	connectCurrentTime           (a, b, type);
}


void TWSWrapper::connectError(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect(a, SIGNAL(error(const QString &)),
			b, SLOT  (error(const QString &)), type );
	connect(a, SIGNAL(error(int, int, const QString &)),
			b, SLOT  (error(int, int, const QString &)), type );
}
void TWSWrapper::disconnectError(QObject *a, QObject *b) {
	disconnect(a, SIGNAL(error(const QString &)),
			b, SLOT  (error(const QString &)) );
	disconnect(a, SIGNAL(error(int, int, const QString &)),
			b, SLOT  (error(int, int, const QString &)));
}

void TWSWrapper::connectTickPrice(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(tickPrice(int, int, double, int)),
		b, SLOT(tickPrice(int, int, double, int)), type );
}
void TWSWrapper::disconnectTickPrice(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(tickPrice(int, int, double, int)),
		b, SLOT(tickPrice(int, int, double, int)) );
}

void TWSWrapper::connectTickSize(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(tickSize(int, int, int)),
		b, SLOT(tickSize(int, int, int)), type);
}
void TWSWrapper::disconnectTickSize(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(tickSize(int, int, int)),
		b, SLOT(tickSize(int, int, int)));
}

void TWSWrapper::connectOrderStatus(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(orderStatus(int, const QString &, int, int, double, int, int, double, int, const QString &)),
		b, SLOT(orderStatus(int, const QString &, int, int, double, int, int, double, int, const QString &)), type );
}
void TWSWrapper::disconnectOrderStatus(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(orderStatus(int, const QString &, int, int, double, int, int, double, int, const QString &)),
		b, SLOT(orderStatus(int, const QString &, int, int, double, int, int, double, int, const QString &)) );
}

void TWSWrapper::connectOpenOrder(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(openOrder( int, const IB::Contract &, const IB::Order &, const IB::OrderState &)),
		b, SLOT(openOrder( int, const IB::Contract &, const IB::Order &, const IB::OrderState &)), type );
}
void TWSWrapper::disconnectOpenOrder(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(openOrder( int, const IB::Contract &, const IB::Order &, const IB::OrderState &)),
		b, SLOT(openOrder( int, const IB::Contract &, const IB::Order &, const IB::OrderState &)) );
}

void TWSWrapper::connectUpdateAccountValue(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(updateAccountValue(const QString & , const QString &, const QString &, const QString & )),
		b, SLOT(updateAccountValue(const QString &, const QString &, const QString &, const QString & )), type );
}
void TWSWrapper::disconnectUpdateAccountValue(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(updateAccountValue(const QString & , const QString &, const QString &, const QString & )),
		b, SLOT(updateAccountValue(const QString &, const QString &, const QString &, const QString & )) );
}

void TWSWrapper::connectUpdatePortfolio(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(updatePortfolio(const IB::Contract &, int, double, double, double, double, double, const QString &)),
		b, SLOT(updatePortfolio(const IB::Contract &, int, double, double, double, double, double, const QString &)), type );
}
void TWSWrapper::disconnectUpdatePortfolio(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(updatePortfolio(const IB::Contract &, int, double, double, double, double, double, const QString &)),
		b, SLOT(updatePortfolio(const IB::Contract &, int, double, double, double, double, double, const QString &)) );
}


void TWSWrapper::connectTickGeneric(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(tickGeneric(int, int, double)),
		b, SLOT(tickGeneric(int, int, double)), type );
}
void TWSWrapper::disconnectTickGeneric(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(tickGeneric(int, int, double)),
		b, SLOT(tickGeneric(int, int, double)));
}

void TWSWrapper::connectUpdateAccountTime(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(updateAccountTime(const QString &)),
		b, SLOT(updateAccountTime(const QString &)), type );
}
void TWSWrapper::disconnectUpdateAccountTime(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(updateAccountTime(const QString &)),
		b, SLOT(updateAccountTime(const QString &)));
}

void TWSWrapper::connectNextValidId(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(nextValidId(int)),
		b, SLOT(nextValidId(int)), type );
}
void TWSWrapper::disconnectNextValidId(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(nextValidId(int)),
		b, SLOT(nextValidId(int)));
}

void TWSWrapper::connectContractDetails(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(contractDetails(int, const IB::ContractDetails &)),
		b, SLOT(contractDetails(int, const IB::ContractDetails &)), type );
}
void TWSWrapper::disconnectContractDetails(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(contractDetails(int, const IB::ContractDetails &)),
		b, SLOT(contractDetails(int, const IB::ContractDetails &)));
}

void TWSWrapper::connectContractDetailsEnd(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(contractDetailsEnd(int)),
		b, SLOT(contractDetailsEnd(int)), type );
}
void TWSWrapper::disconnectContractDetailsEnd(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(contractDetailsEnd(int)),
		b, SLOT(contractDetailsEnd(int)));
}

void TWSWrapper::connectExecDetails(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(execDetails (int, const IB::Contract &, const IB::Execution &)),
		b, SLOT(execDetails (int, const IB::Contract &, const IB::Execution &)), type );
}
void TWSWrapper::disconnectExecDetails(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(execDetails (int, const IB::Contract &, const IB::Execution &)),
		b, SLOT(execDetails (int, const IB::Contract &, const IB::Execution &)));
}

void TWSWrapper::connectManagedAccounts(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(managedAccounts(const QString &)),
		b, SLOT(managedAccounts(const QString &)), type );
}
void TWSWrapper::disconnectManagedAccounts(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(managedAccounts(const QString &)),
		b, SLOT(managedAccounts(const QString &)));
}

void TWSWrapper::connectTickOptionComputation(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(tickOptionComputation(int, int, double, double, double, double)),
		b, SLOT(tickOptionComputation(int, int, double, double, double, double)), type );
}
void TWSWrapper::disconnectTickOptionComputation(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(tickOptionComputation(int, int, double, double, double, double)),
		b, SLOT(tickOptionComputation(int, int, double, double, double, double)) );
}

void TWSWrapper::connectHistoricalData(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(historicalData(int, const QString &, double, double, double,
			double, int, int, double, bool)),
		b, SLOT(historicalData(int, const QString &, double, double, double,
			double, int, int, double, bool)), type );
}
void TWSWrapper::disconnectHistoricalData(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(historicalData(int, const QString &, double, double, double,
			double, int, int, double, bool)),
		b, SLOT(historicalData(int, const QString &, double, double, double,
			double, int, int, double, bool)) );
}

void TWSWrapper::connectTickString(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(tickString(int, int, const QString &)),
		b, SLOT(tickString(int, int, const QString &)), type );
}
void TWSWrapper::disconnectTickString(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(tickString(int, int, const QString &)),
		b, SLOT(tickString(int, int, const QString &)) );
}

void TWSWrapper::connectCurrentTime(QObject *a, QObject *b, Qt::ConnectionType type) {
	connect ( a, SIGNAL(currentTime(long)),
		b, SLOT(currentTime(long)), type );
}
void TWSWrapper::disconnectCurrentTime(QObject *a, QObject *b) {
	disconnect ( a, SIGNAL(currentTime(long)),
		b, SLOT(currentTime(long)) );
}

///// Wrapper functions ////////////////////////////////////////
// this is the callback interface
// TODO: actually only useful for debugging

void TWSWrapper::error( const QString &msg ) {
	qDebug() << "ERR_MSG_SHORT:" << msg;
}
void TWSWrapper::error( int id, int errorCode, const QString &errorMsg ) {
	qDebug() << "ERR_MSG_LONG:" << id << errorCode << errorMsg;
}


void TWSWrapper::tickPrice( int tickerId, int field, double price, int canAutoExecute ){
	qDebug() << "TICK_PRICE:" << tickerId << ibToString(field) << price << canAutoExecute;
}
void TWSWrapper::tickSize( int tickerId, int field, int size ){
	qDebug() << "TICK_SIZE:" << tickerId << ibToString(field) << size;
}
void TWSWrapper::orderStatus ( int orderId, const QString &status, int filled, int remaining, double avgFillPrice,
	int permId, int parentId, double lastFillPrice, int clientId, const QString &whyHeld ) {
	qDebug() << "ORDER_STATUS:" << QString("orderId:%1, status:%2, filled:%3, remaining:%4, ")
		.arg(orderId).arg(status).arg(filled).arg(remaining)
		<< permId << parentId << avgFillPrice << lastFillPrice << clientId << whyHeld;
}
void TWSWrapper::openOrder( int orderId, const IB::Contract &contract, const IB::Order &order, const IB::OrderState &orderState ) {
	qDebug() << "OPEN_ORDER:" << orderId << toQString(contract.symbol) << toQString(order.action)
		<< "warnTxt:"  << toQString(orderState.warningText)   << "status:"    << toQString(orderState.status)
		<< "com:"      << orderState.commission    << "comCur"     << toQString(orderState.commissionCurrency)
		<< "minCom:"   << orderState.minCommission << "maxCom:"    << orderState.maxCommission
		<< "initMarg:" << toQString(orderState.initMargin)    << "maintMarg:" << toQString(orderState.maintMargin)
		<< "ewl:"      << toQString(orderState.equityWithLoan);
}
void TWSWrapper::updateAccountValue( const QString &key, const QString &value, const QString &currency, const QString &accountName ) {
	qDebug() << "ACCT_VALUE:" << key << value << currency << accountName;
}
void TWSWrapper::updatePortfolio( const IB::Contract &contract, int position, double marketPrice, double marketValue,
			double averageCost, double unrealizedPNL, double realizedPNL, const QString &accountName ) {
	qDebug() << "PORTFOLIO_VALUE:" << toQString(contract.symbol) << toQString(contract.localSymbol) << position << marketPrice << marketValue
		<< averageCost << unrealizedPNL << realizedPNL << accountName;
}
void TWSWrapper::updateAccountTime( const QString &timeStamp ) {
	qDebug() << "ACCT_UPDATE_TIME:" << timeStamp;
}
void TWSWrapper::nextValidId(int orderId) {
	qDebug() << "NEXT_VALID_ID:" << orderId;
}
void TWSWrapper::contractDetails( int reqId, const IB::ContractDetails &ibContractDetails ) {
	qDebug() << "CONTRACT_DATA:" << reqId
		<< toQString(ibContractDetails.summary.symbol)   << toQString(ibContractDetails.summary.secType)
		<< toQString(ibContractDetails.summary.expiry)   << ibContractDetails.summary.strike
		<< toQString(ibContractDetails.summary.right)    << toQString(ibContractDetails.summary.exchange)
		<< toQString(ibContractDetails.summary.currency) << toQString(ibContractDetails.summary.localSymbol)
		<< toQString(ibContractDetails.marketName)       << toQString(ibContractDetails.tradingClass)
		<< toQString(ibContractDetails.validExchanges);
}
void TWSWrapper::contractDetailsEnd( int reqId ) {
	qDebug() << "CONTRACT_DATA_END:" << reqId;
}
void TWSWrapper::execDetails (int orderId, const IB::Contract &contract, const IB::Execution &exec ) {
	qDebug() << "EXECUTION_DATA" << orderId
		<< toQString(contract.localSymbol) << ibToString(exec);
}
void TWSWrapper::managedAccounts ( const QString &accountsList ) {
	qDebug() << "MANAGED_ACCTS" << accountsList;
}
void TWSWrapper::historicalData(int reqId, const QString &date, double open, double high, double low,
		double close, int volume, int count, double WAP, bool hasGaps) {
	qDebug() << "HISTORICAL_DATA:" << reqId
		<< date << open << high << low << close << volume << count << WAP << hasGaps;
}
void TWSWrapper::tickOptionComputation ( int tickerId, int tickType, double impliedVol,
	double delta, double modelPrice, double pvDividend ) {
	qDebug() << "TICK_OPTION_COMPUTATION:" << tickerId << ibToString(tickType) << impliedVol 
		<< delta << modelPrice << pvDividend;
}
void TWSWrapper::tickGeneric( int tickerId, int tickType, double value ){
	qDebug() << "TICK_GENERIC:" << tickerId << ibToString(tickType) << value;
}
void TWSWrapper::tickString( int tickerId, int tickType, const QString &value ){
	qDebug() << "TICK_STRING:" << tickerId << ibToString(tickType)
		<< ( (tickType != IB::LAST_TIMESTAMP) ? value : QDateTime::fromTime_t(value.toInt()).toString() );
}
void TWSWrapper::currentTime( long time ) {
	qDebug() << "CURRENT_TIME:" << time; //QDateTime:: fromTime_t(time);
}
///// end Wrapper functions ////////////////////////////////////
