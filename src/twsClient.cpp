#include "twsClient.h"
#include "twsUtil.h"
#include "debug.h"

// from global installed ibtws
#include "ibtws/Contract.h"
#include "ibtws/Order.h"
#include "ibtws/OrderState.h"
#include "ibtws/Execution.h"
#include "ibtws/TwsSocketClientErrors.h"
#include "ibtws/EWrapper.h"
#include "ibtws/EPosixClientSocket.h"

#include <QtCore/QTimer>
#include <QtCore/QMetaType>
#include <unistd.h>
#include <errno.h>


// debug verbosity 0...n
#define tws_debug_level 1
#define TWS_DEBUG( _level, _msg )        \
	if( tws_debug_level >= _level ) {    \
		qDebug() << _msg;                    \
	}



class TWSClient::MyEWrapper : public IB::EWrapper
{
	public:
		MyEWrapper( TWSClient* parent );
		~MyEWrapper();
		
		// events
		void tickPrice( IB::TickerId, IB::TickType, double price,
			int canAutoExecute );
		void tickSize( IB::TickerId, IB::TickType, int size );
		void tickOptionComputation( IB::TickerId, IB::TickType,
			double impliedVol, double delta, double modelPrice,
			double pvDividend );
		void tickGeneric( IB::TickerId, IB::TickType, double value );
		void tickString( IB::TickerId, IB::TickType, const IB::IBString& val );
		void tickEFP( IB::TickerId, IB::TickType, double basisPoints,
			const IB::IBString& formattedBasisPoints, double totalDividends,
			int holdDays, const IB::IBString& futureExpiry,
			double dividendImpact, double dividendsToExpiry );
		void orderStatus( IB::OrderId, const IB::IBString& status,
			int filled, int remaining, double avgFillPrice,
			int permId, int parentId, double lastFillPrice,
			int clientId, const IB::IBString& whyHeld );
		void openOrder( IB::OrderId, const IB::Contract&, const IB::Order&,
			const IB::OrderState& );
		void openOrderEnd();
		void winError( const IB::IBString& str, int lastError );
		void connectionClosed();
		void updateAccountValue( const IB::IBString& key,
			const IB::IBString& val, const IB::IBString& currency,
			const IB::IBString& accountName );
		void updatePortfolio( const IB::Contract&, int position,
			double marketPrice, double marketValue, double averageCost,
			double unrealizedPNL, double realizedPNL,
			const IB::IBString& accountName );
		void updateAccountTime( const IB::IBString& timeStamp );
		void accountDownloadEnd( const IB::IBString& accountName );
		void nextValidId( IB::OrderId orderId );
		void contractDetails( int reqId, const IB::ContractDetails& );
		void bondContractDetails( int reqId, const IB::ContractDetails& );
		void contractDetailsEnd( int reqId );
		void execDetails( int reqId, const IB::Contract& contract,
			const IB::Execution& execution );
		void execDetailsEnd( int reqId );
		void error( const int id, const int errorCode,
			const IB::IBString errorString );
		void updateMktDepth( IB::TickerId id, int position,
			int operation, int side, double price, int size );
		void updateMktDepthL2( IB::TickerId id, int position,
			IB::IBString marketMaker, int operation, int side,
			double price, int size );
		void updateNewsBulletin( int msgId, int msgType,
			const IB::IBString& newsMessage,
			const IB::IBString& originExch );
		void managedAccounts( const IB::IBString& accountsList );
		void receiveFA( IB::faDataType pFaDataType,
			const IB::IBString& cxml );
		void historicalData( IB::TickerId reqId, const IB::IBString& date,
			double open, double high, double low, double close,
			int volume, int barCount, double WAP, int hasGaps );
		void scannerParameters( const IB::IBString& xml );
		void scannerData( int reqId, int rank, const IB::ContractDetails&,
			const IB::IBString& distance, const IB::IBString& benchmark,
			const IB::IBString& projection, const IB::IBString& legsStr );
		void scannerDataEnd( int reqId );
		void realtimeBar( IB::TickerId reqId, long time,
			double open, double high, double low, double close,
			long volume, double wap, int count );
		void currentTime( long time );
		void fundamentalData( IB::TickerId reqId,
			const IB::IBString& data );
		void deltaNeutralValidation(int reqId,
			const IB::UnderComp& underComp );
		void tickSnapshotEnd( int reqId );
		
	private:
		TWSClient *parentTWSClient;
};


TWSClient::MyEWrapper::MyEWrapper( TWSClient* parent ) :
	IB::EWrapper(),
	parentTWSClient(parent)
{
}


TWSClient::MyEWrapper::~MyEWrapper()
{
}


void TWSClient::MyEWrapper::tickPrice( IB::TickerId tickerId,
	IB::TickType tickType, double price, int canAutoExecute )
{
	TWS_DEBUG( 3, "");
	emit parentTWSClient->tickPrice(
		tickerId, tickType, price, canAutoExecute );
}


void TWSClient::MyEWrapper::tickSize( IB::TickerId tickerId,
	IB::TickType tickType, int size )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->tickSize(
		tickerId, tickType, size );
}


void TWSClient::MyEWrapper::tickOptionComputation( IB::TickerId tickerId,
	IB::TickType tickType, double impliedVol, double delta, double modelPrice,
	double pvDividend )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->tickOptionComputation(
		tickerId, tickType, impliedVol, delta, modelPrice, pvDividend );
}


void TWSClient::MyEWrapper::tickGeneric( IB::TickerId tickerId,
	IB::TickType tickType, double value )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->tickGeneric(
		tickerId, tickType, value );
}


void TWSClient::MyEWrapper::tickString( IB::TickerId tickerId,
	IB::TickType tickType, const IB::IBString& val )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->tickString(
		tickerId, tickType, toQString(val) );
}


void TWSClient::MyEWrapper::tickEFP( IB::TickerId, IB::TickType, double /*basisPoints*/,
	const IB::IBString& /*formattedBasisPoints*/, double /*totalDividends*/,
	int /*holdDays*/, const IB::IBString& /*futureExpiry*/,
	double /*dividendImpact*/, double /*dividendsToExpiry*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::orderStatus( IB::OrderId orderId,
	const IB::IBString &status, int filled, int remaining, double avgFillPrice,
	int permId, int parentId, double lastFillPrice, int clientId,
	const IB::IBString& whyHeld )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->orderStatus(
		orderId, toQString(status), filled, remaining, avgFillPrice,
		permId, parentId, lastFillPrice, clientId, toQString(whyHeld) );
}


void TWSClient::MyEWrapper::openOrder( IB::OrderId oid, const IB::Contract& c,
	const IB::Order& o, const IB::OrderState& os)
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->openOrder(
		oid, c, o, os );
}


void TWSClient::MyEWrapper::openOrderEnd()
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::winError( const IB::IBString &/*str*/, int /*lastError*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::connectionClosed()
{
	TWS_DEBUG( 3, "");
	
	parentTWSClient->disconnected();
}


void TWSClient::MyEWrapper::updateAccountValue( const IB::IBString& key,
	const IB::IBString& val, const IB::IBString& currency,
	const IB::IBString& accountName )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->updateAccountValue(
		toQString(key), toQString(val), toQString(currency),
		toQString(accountName) );
}


void TWSClient::MyEWrapper::updatePortfolio( const IB::Contract& c,
	int position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IB::IBString& accountName )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->updatePortfolio(
		c, position, marketPrice, marketValue, averageCost, unrealizedPNL,
		realizedPNL, toQString(accountName) );
}


void TWSClient::MyEWrapper::updateAccountTime( const IB::IBString& timeStamp )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->updateAccountTime(
		toQString(timeStamp) );
}


void TWSClient::MyEWrapper::accountDownloadEnd( const IB::IBString& /*accountName*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::nextValidId( IB::OrderId orderId )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->nextValidId(
		orderId );
}


void TWSClient::MyEWrapper::contractDetails( int reqId, const IB::ContractDetails& cD)
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->contractDetails(
		reqId, cD );
}


void TWSClient::MyEWrapper::bondContractDetails( int reqId, const IB::ContractDetails& cD)
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->bondContractDetails(
		reqId, cD );
}


void TWSClient::MyEWrapper::contractDetailsEnd( int reqId )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->contractDetailsEnd(
		reqId );
}


void TWSClient::MyEWrapper::execDetails( int reqId, const IB::Contract& c,
	const IB::Execution& exec )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->execDetails(
		reqId, c, exec );
}


void TWSClient::MyEWrapper::execDetailsEnd( int /*reqId*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::error( const int id, const int errorCode,
	const IB::IBString errorString )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->error(
		id, errorCode, toQString(errorString) );
}


void TWSClient::MyEWrapper::updateMktDepth( IB::TickerId /*id*/, int /*position*/,
	int /*operation*/, int /*side*/, double /*price*/, int /*size*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::updateMktDepthL2( IB::TickerId /*id*/, int /*position*/,
	IB::IBString /*marketMaker*/, int /*operation*/, int /*side*/,
	double /*price*/, int /*size*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::updateNewsBulletin( int /*msgId*/, int /*msgType*/,
	const IB::IBString& /*newsMessage*/,
	const IB::IBString& /*originExch*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::managedAccounts( const IB::IBString& accountsList )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->managedAccounts(
		toQString(accountsList) );
	
}


void TWSClient::MyEWrapper::receiveFA( IB::faDataType /*pFaDataType*/,
const IB::IBString& /*cxml*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::historicalData( IB::TickerId reqId,
	const IB::IBString& date, double open, double high, double low,
	double close, int volume, int barCount, double WAP, int hasGaps )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->historicalData(
		reqId, toQString(date), open, high, low, close, volume,
		barCount, WAP, hasGaps);
}


void TWSClient::MyEWrapper::scannerParameters( const IB::IBString& /*xml*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::scannerData( int /*reqId*/, int /*rank*/, const IB::ContractDetails&,
	const IB::IBString& /*distance*/, const IB::IBString& /*benchmark*/,
	const IB::IBString& /*projection*/, const IB::IBString& /*legsStr*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::scannerDataEnd( int /*reqId*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::realtimeBar( IB::TickerId /*reqId*/, long /*time*/, double /*open*/, double /*high*/,
	double /*low*/, double /*close*/, long /*volume*/, double /*wap*/, int /*count*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::currentTime( long time )
{
	TWS_DEBUG( 3, "");
	
	emit parentTWSClient->currentTime(
		time );
}


void TWSClient::MyEWrapper::fundamentalData( IB::TickerId /*reqId*/, const IB::IBString& /*data*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::deltaNeutralValidation(int /*reqId*/, const IB::UnderComp& /*underComp*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::MyEWrapper::tickSnapshotEnd( int /*reqId*/ )
{
	TWS_DEBUG( 3, "");
}


void TWSClient::registerMetaTypes()
{
	static bool qMetaTypesRegistered = false;
	
	if( !qMetaTypesRegistered ) {
		qRegisterMetaType<IB::Contract>("IB::Contract");
		qRegisterMetaType<IB::ContractDetails>("IB::ContractDetails");
		qRegisterMetaType<IB::Order>("IB::Order");
		qRegisterMetaType<IB::Order>("IB::OrderState");
		qRegisterMetaType<IB::Execution>("IB::Execution");
		qMetaTypesRegistered = true;
	}
}


TWSClient::TWSClient()
	: QObject(),
	ePosixClient(NULL)
{
	registerMetaTypes();
	
	twsHost  = "otto";
	twsPort  = 7497;
	clientId = 579;
	
	myEWrapper = new MyEWrapper( this );
	
	//start();
	qDebug() << "running";
	
	selectTimer = new QTimer();
	selectTimer->setInterval(0);
	selectTimer->setSingleShot(false);
	connect( selectTimer, SIGNAL(timeout()), this, SLOT(selectStuff()) );
	
	// TODO do something like that:
	//connect ( tcpSocket, SIGNAL(disconnected()), this, SLOT(disconnected()) );
	//connect ( tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(tcpError(/*QAbstractSocket::SocketError*/)) );
}


TWSClient::~TWSClient()
{
	qDebug() << "called";
	
	delete selectTimer;
	
	if( ePosixClient != NULL ) {
		delete ePosixClient;
	}
	delete myEWrapper;
}


bool TWSClient::isConnected() const
{
	return ( (ePosixClient != NULL) && ePosixClient->isConnected() );
}


QString TWSClient::getTWSHost() const
{
	return twsHost;
}


quint16 TWSClient::getTWSPort() const
{
	return twsPort;
}


int     TWSClient::getClientId() const
{
	return clientId;
}


void TWSClient::setTWSHost( const QString &host )
{
	if ( isConnected() ) {
		Q_ASSERT(false); // TODO handle that
	}
	this->twsHost = host;
}


void TWSClient::setTWSPort( const quint16 &port )
{
	if ( isConnected() ) {
		Q_ASSERT(false); // TODO handle that
	}
	this->twsPort  = port;
}


void TWSClient::setClientId( const int clientId )
{
	if ( isConnected() ) {
		Q_ASSERT(false); // TODO handle that
	}
	this->clientId = clientId;
}


void TWSClient::connectTWS()
{
	//TODO connectTWS( host, port, clientId ) should call connectTWS()
	connectTWS( twsHost, twsPort, clientId );
}


void TWSClient::connectTWS( const QString &host, quint16 port, int clientId )
{
	qDebug() << "called:" <<  QString("%1:%2, clientId: %3").arg(host).arg(port).arg(clientId);
	
	if( isConnected() ) {
		emit error ( IB::NO_VALID_ID, IB::ALREADY_CONNECTED.code(),
		             toQString(IB::ALREADY_CONNECTED.msg()) );
		return;
	} else if( ePosixClient != NULL ) {
		delete ePosixClient;
	}
	
	ePosixClient = new IB::EPosixClientSocket(myEWrapper);
	
	this->twsHost  = host;
	this->twsPort  = port;
	this->clientId = clientId;
	
	ePosixClient->eConnect( host.toUtf8().constData(), port, clientId );
	
	if( !ePosixClient->isConnected() ) {
		qDebug() << "Connection to TWS failed:"; //TODO print a specific error
		emit connected( false );
		return;
	} else {
		//TODO print client/server version and m_TwsTime
		emit connected( true );
		startSelect();
	}
}


void TWSClient::disconnectTWS()
{
	qDebug();
	
	if ( !isConnected()) {
		return;
	}
	
	ePosixClient->eDisconnect();
	disconnected();
}


void TWSClient::disconnected()
{
	Q_ASSERT( !isConnected() );
	stopSelect();
	emit connected( false );
	qDebug() << "We are disconnected";
}


void TWSClient::tcpError(/*QAbstractSocket::SocketError socketError*/)
{
	//TODO: handle SocketErrors somehow
	// should we call disconnected() here or is it _always_ called anyway?
	//qDebug() << tcpSocket->errorString();
}


void TWSClient::startSelect()
{
	qDebug();
	selectTimer->start();
}


void TWSClient::stopSelect()
{
	qDebug();
	selectTimer->stop();
}


void TWSClient::selectStuff()
{
	int fd = ePosixClient->fd();
	
	Q_ASSERT( fd >= 0 && isConnected() );
	
	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 1;
	
	fd_set readSet, writeSet, errorSet;
	
	FD_ZERO( &readSet);
	errorSet = writeSet = readSet;
	
	FD_SET( fd, &readSet);
	if( !ePosixClient->isOutBufferEmpty()) {
		FD_SET( fd, &writeSet);
	}
	FD_CLR( fd, &errorSet);
	
	int ret = select( fd + 1,
		&readSet, &writeSet, &errorSet, &tval );
	/////  blocking  ///////////////////////////////////////
	
	if( ret == 0) {
		TWS_DEBUG( 5 , "Select timeouted." );
		return;
	} else if( ret < 0) {
		TWS_DEBUG( 1 , QString("Select failed with failed with errno: %1.")
			.arg(strerror(errno)) );
		disconnectTWS();
		return;
	}
	
	Q_ASSERT( fd == ePosixClient->fd() ); //can't change in the meanwhile
	if( FD_ISSET( fd, &errorSet)) {
		TWS_DEBUG( 1 ,"Error on socket." );
		ePosixClient->onError(); // might disconnect us
		if( !isConnected() ) {
			return;
		}
	}
	
	Q_ASSERT( fd == ePosixClient->fd() );
	if( FD_ISSET( fd, &writeSet)) {
		TWS_DEBUG( 1 ,"Socket is ready for writing." );
		ePosixClient->onSend(); // might disconnect us on socket errors
		if( !isConnected() ) {
			return;
		}
	}
	
	Q_ASSERT( fd == ePosixClient->fd() );
	if( FD_ISSET( fd, &readSet)) {
		TWS_DEBUG( 6 ,"Socket is ready for reading." );
		ePosixClient->onReceive(); // might disconnect us on socket errors
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////


void TWSClient::reqMktData(int tickerId, const IB::Contract &contract,
	const  QString &genericTickList, bool snapshot)
{
	qDebug() << "REQ_MKT_DATA" << tickerId << toQString(contract.symbol) <<  toQString(contract.exchange)
		<< toQString(contract.secType) << toQString(contract.expiry) << toQString(contract.right)
		<< contract.strike << toQString(contract.currency)  << toQString(contract.localSymbol)
		<< genericTickList;
	
	ePosixClient->reqMktData( tickerId, contract, toIBString(genericTickList), snapshot );
}


void TWSClient::cancelMktData ( int tickerId )
{
	qDebug() << "CANCEL_MKT_DATA" << tickerId;
	
	ePosixClient->cancelMktData( tickerId );
}


void TWSClient::placeOrder ( int id, const IB::Contract &contract,
	const IB::Order &order )
{
	qDebug() << "PLACE_ORDER" << id << toQString(order.orderType) << order.totalQuantity << toQString(order.action) << order.lmtPrice << toQString(contract.symbol);
	
	ePosixClient->placeOrder( id, contract, order );
}


void TWSClient::cancelOrder ( int id )
{
	qDebug() << "CANCEL_ORDER" << id;
	
	ePosixClient->cancelOrder( id );
}


void TWSClient::reqOpenOrders()
{
	qDebug() << "REQ_OPEN_ORDERS";
	
	ePosixClient->reqOpenOrders();
}


void TWSClient::reqAccountUpdates( bool subscribe, const QString &acctCode )
{
	qDebug() << "REQ_ACCOUNT_DATA" << subscribe << acctCode;
	
	ePosixClient->reqAccountUpdates( subscribe, toIBString(acctCode) );
}


void TWSClient::reqIds( int numIds)
{
	qDebug() << "REQ_IDS" << numIds;
	
	ePosixClient->reqIds( numIds );
}


void TWSClient::reqContractDetails( int reqId, const IB::Contract &contract )
{
	qDebug() << "REQ_CONTRACT_DATA" << reqId << toQString(contract.symbol)
		<< toQString(contract.secType) << toQString(contract.exchange);
	
	ePosixClient->reqContractDetails( reqId, contract );
}


void TWSClient::setServerLogLevel( int logLevel )
{
	qDebug() << "SET_SERVER_LOGLEVEL" << logLevel;
	
	ePosixClient->setServerLogLevel( logLevel );
}


void TWSClient::reqHistoricalData ( int tickerId, const IB::Contract &contract,
	const QString &endDateTime, const QString &durationStr,
	const QString &barSizeSetting, const QString &whatToShow, int useRTH,
	int formatDate )
{
	qDebug() << "REQ_HISTORICAL_DATA" << tickerId << toQString(contract.symbol)
		<< toQString(contract.secType) << toQString(contract.exchange) << endDateTime << durationStr
		<< barSizeSetting << whatToShow << useRTH << formatDate;
	
	ePosixClient->reqHistoricalData( tickerId, contract, toIBString(endDateTime),
		toIBString(durationStr), toIBString(barSizeSetting),
		toIBString(whatToShow), useRTH, formatDate );
}


void TWSClient::reqCurrentTime()
{
	qDebug() << "REQ_CURRENT_TIME";
	
	ePosixClient->reqCurrentTime();
}

