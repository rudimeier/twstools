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

#include <unistd.h>
#include <errno.h>


// debug verbosity 0...n
#define tws_debug_level 1
#define TWS_DEBUG( _level, _msg )        \
	if( tws_debug_level >= _level ) {    \
		qDebug() << _msg;                    \
	}




TWSClient::TWSClient( IB::EWrapper *ew ) :
	myEWrapper(ew),
	ePosixClient(NULL)
{
	twsHost  = "otto";
	twsPort  = 7497;
	clientId = 579;
	
	//start();
	qDebug() << "running";
}


TWSClient::~TWSClient()
{
	qDebug() << "called";
	
	if( ePosixClient != NULL ) {
		delete ePosixClient;
	}
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
// 	connectTWS( twsHost, twsPort, clientId, new FU );
}


void TWSClient::connectTWS( const QString &host, quint16 port, int clientId )
{
	qDebug() << "called:" <<  QString("%1:%2, clientId: %3").arg(host).arg(port).arg(clientId);
	
	if( isConnected() ) {
		myEWrapper->error( IB::NO_VALID_ID, IB::ALREADY_CONNECTED.code(),
			IB::ALREADY_CONNECTED.msg() );
		return;
	} else if( ePosixClient != NULL ) {
		delete ePosixClient;
	}
	
	ePosixClient = new IB::EPosixClientSocket(myEWrapper);
	
	this->twsHost  = host;
	this->twsPort  = port;
	this->clientId = clientId;
	
	ePosixClient->eConnect( host.toUtf8().constData(), port, clientId );
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
	myEWrapper->connectionClosed();
	qDebug() << "We are disconnected";
}


void TWSClient::selectStuff( int msec )
{
	int fd = ePosixClient->fd();
	
	Q_ASSERT( fd >= 0 );
	
	struct timeval tval;
	tval.tv_usec = msec * 1000;
	tval.tv_sec = 0;
	
	fd_set readSet, writeSet, errorSet;
	
	FD_ZERO( &readSet);
	errorSet = writeSet = readSet;
	
	if( isConnected() ) {
		// if not connected then all sets are zero and select will just timeout
		FD_SET( fd, &readSet);
		if( !ePosixClient->isOutBufferEmpty()) {
			FD_SET( fd, &writeSet);
		}
		FD_CLR( fd, &errorSet);
	}
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
	
	if( FD_ISSET( fd, &errorSet)) {
		TWS_DEBUG( 1 ,"Error on socket." );
		ePosixClient->onError(); // might disconnect us
		if( !isConnected() ) {
			return;
		}
	}
	
	if( FD_ISSET( fd, &writeSet)) {
		TWS_DEBUG( 1 ,"Socket is ready for writing." );
		ePosixClient->onSend(); // might disconnect us on socket errors
		if( !isConnected() ) {
			return;
		}
	}
	
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

