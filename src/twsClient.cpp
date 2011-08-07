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
#include <string.h>


// debug verbosity 0...n
#define tws_debug_level 1
#define TWS_DEBUG( _level, _fmt, _msg... )        \
	if( tws_debug_level >= _level ) {    \
		DEBUG_PRINTF("tws debug: " _fmt, ## _msg); \
	}




TWSClient::TWSClient( IB::EWrapper *ew ) :
	myEWrapper(ew),
	ePosixClient(NULL)
{
	twsHost  = "otto";
	twsPort  = 7497;
	clientId = 579;
}


TWSClient::~TWSClient()
{
	if( ePosixClient != NULL ) {
		delete ePosixClient;
	}
}


bool TWSClient::isConnected() const
{
	return ( (ePosixClient != NULL) && ePosixClient->isConnected() );
}


std::string TWSClient::getTWSHost() const
{
	return twsHost;
}


int TWSClient::getTWSPort() const
{
	return twsPort;
}


int     TWSClient::getClientId() const
{
	return clientId;
}


void TWSClient::setTWSHost( const std::string &host )
{
	if ( isConnected() ) {
		assert(false); // TODO handle that
	}
	this->twsHost = host;
}


void TWSClient::setTWSPort( const int port )
{
	if ( isConnected() ) {
		assert(false); // TODO handle that
	}
	this->twsPort  = port;
}


void TWSClient::setClientId( const int clientId )
{
	if ( isConnected() ) {
		assert(false); // TODO handle that
	}
	this->clientId = clientId;
}


void TWSClient::connectTWS()
{
	//TODO connectTWS( host, port, clientId ) should call connectTWS()
// 	connectTWS( twsHost, twsPort, clientId, new FU );
}


void TWSClient::connectTWS( const std::string &host, int port, int clientId )
{
	DEBUG_PRINTF("connect: %s:%d, clientId: %d", host.c_str(), port, clientId);
	
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
	
	ePosixClient->eConnect( host.c_str(), port, clientId );
}


void TWSClient::disconnectTWS()
{
	DEBUG_PRINTF("disconnect TWS");
	
	if ( !isConnected()) {
		return;
	}
	
	ePosixClient->eDisconnect();
	disconnected();
}


void TWSClient::disconnected()
{
	assert( !isConnected() );
	myEWrapper->connectionClosed();
	DEBUG_PRINTF("We are disconnected");
}


void TWSClient::selectStuff( int msec )
{
	struct timeval tval;
	tval.tv_usec = msec * 1000;
	tval.tv_sec = 0;
	
	fd_set readSet, writeSet, errorSet;
	
	FD_ZERO( &readSet);
	errorSet = writeSet = readSet;
	
	int fd = -1;
	if( isConnected() ) {
		// if not connected then all sets are zero and select will just timeout
		fd = ePosixClient->fd();
		assert( fd >= 0 );
		
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
		TWS_DEBUG( 1 , "Select failed with failed with errno: %s.",
			strerror(errno) );
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


int TWSClient::serverVersion()
{
	return ePosixClient->serverVersion();
}

std::string TWSClient::TwsConnectionTime()
{
	return ePosixClient->TwsConnectionTime();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////


void TWSClient::reqMktData(int tickerId, const IB::Contract &contract,
	const  std::string &genericTickList, bool snapshot)
{
	DEBUG_PRINTF( "REQ_MKT_DATA %d "
		"'%s' '%s' '%s' '%s' '%s' '%g' '%s' '%s' '%s' %d",
		tickerId, contract.symbol.c_str(), contract.exchange.c_str(),
		contract.secType.c_str(), contract.expiry.c_str(),
		contract.right.c_str(), contract.strike, contract.currency.c_str(),
		contract.localSymbol.c_str(), genericTickList.c_str(), snapshot );
	
	ePosixClient->reqMktData( tickerId, contract, genericTickList, snapshot );
}


void TWSClient::cancelMktData ( int tickerId )
{
	DEBUG_PRINTF("CANCEL_MKT_DATA %d", tickerId);
	
	ePosixClient->cancelMktData( tickerId );
}


void TWSClient::placeOrder ( int id, const IB::Contract &contract,
	const IB::Order &order )
{
	DEBUG_PRINTF("PLACE_ORDER %d '%s' %ld '%s' %g '%s'",
		id, order.orderType.c_str(), order.totalQuantity, order.action.c_str(),
		order.lmtPrice, contract.symbol.c_str() );
	
	ePosixClient->placeOrder( id, contract, order );
}


void TWSClient::cancelOrder ( int id )
{
	DEBUG_PRINTF("CANCEL_ORDER %d", id);
	
	ePosixClient->cancelOrder( id );
}


void TWSClient::reqOpenOrders()
{
	DEBUG_PRINTF("REQ_OPEN_ORDERS");
	
	ePosixClient->reqOpenOrders();
}


void TWSClient::reqAllOpenOrders()
{
	DEBUG_PRINTF("REQ_ALL_OPEN_ORDERS");
	ePosixClient->reqAllOpenOrders();
}


void TWSClient::reqAutoOpenOrders( bool bAutoBind )
{
	DEBUG_PRINTF("REQ_AUTO_OPEN_ORDERS %d", bAutoBind);
	ePosixClient->reqAutoOpenOrders( bAutoBind );
}


void TWSClient::reqAccountUpdates( bool subscribe, const std::string &acctCode )
{
	DEBUG_PRINTF("REQ_ACCOUNT_DATA %d '%s'", subscribe, acctCode.c_str() );
	
	ePosixClient->reqAccountUpdates( subscribe, acctCode );
}


void TWSClient::reqExecutions(int reqId, const IB::ExecutionFilter& filter)
{
	DEBUG_PRINTF("REQ_EXECUTIONS %d", reqId);
	
	ePosixClient->reqExecutions(reqId, filter);
}


void TWSClient::reqIds( int numIds)
{
	DEBUG_PRINTF("REQ_IDS %d", numIds);
	
	ePosixClient->reqIds( numIds );
}


void TWSClient::reqContractDetails( int reqId, const IB::Contract &contract )
{
	DEBUG_PRINTF("REQ_CONTRACT_DATA %d '%s' '%s' '%s'",
		reqId, contract.symbol.c_str(), contract.secType.c_str(),
		contract.exchange.c_str() );
	
	ePosixClient->reqContractDetails( reqId, contract );
}


void TWSClient::setServerLogLevel( int logLevel )
{
	DEBUG_PRINTF("SET_SERVER_LOGLEVEL %d", logLevel);
	
	ePosixClient->setServerLogLevel( logLevel );
}


void TWSClient::reqHistoricalData ( int tickerId, const IB::Contract &contract,
	const std::string &endDateTime, const std::string &durationStr,
	const std::string &barSizeSetting, const std::string &whatToShow,
	int useRTH, int formatDate )
{
	DEBUG_PRINTF("REQ_HISTORICAL_DATA %d "
		"'%s' '%s' '%s' '%s' '%s' '%s' '%s' %d %d",
		tickerId, contract.symbol.c_str(), contract.secType.c_str(),
		contract.exchange.c_str(), endDateTime.c_str(), durationStr.c_str(),
		barSizeSetting.c_str(), whatToShow.c_str(), useRTH, formatDate );
	
	ePosixClient->reqHistoricalData( tickerId, contract, endDateTime,
		durationStr, barSizeSetting, whatToShow, useRTH, formatDate );
}


void TWSClient::reqCurrentTime()
{
	DEBUG_PRINTF("REQ_CURRENT_TIME");
	
	ePosixClient->reqCurrentTime();
}

