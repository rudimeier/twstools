#include "twsClient.h"
#include "twsUtil.h"
#include "debug.h"

// from global installed twsapi
#include "twsapi/Contract.h"
#include "twsapi/Order.h"
#include "twsapi/OrderState.h"
#include "twsapi/Execution.h"
#include "twsapi/TwsSocketClientErrors.h"
#include "twsapi/EWrapper.h"
#include "twsapi/EPosixClientSocket.h"

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
	ePosixClient( new IB::EPosixClientSocket(ew) )
{
}


TWSClient::~TWSClient()
{
	delete ePosixClient;
}


bool TWSClient::isConnected() const
{
	return ePosixClient->isConnected();
}


bool TWSClient::connectTWS( const std::string &host, int port, int clientId )
{
	DEBUG_PRINTF("connect: %s:%d, clientId: %d", host.c_str(), port, clientId);
	
	return ePosixClient->eConnect( host.c_str(), port, clientId );
}


void TWSClient::disconnectTWS()
{
	DEBUG_PRINTF("disconnect TWS");
	
	if ( !isConnected()) {
		return;
	}
	
	ePosixClient->eDisconnect();
	myEWrapper->connectionClosed();
	DEBUG_PRINTF("We are disconnected");
}


void TWSClient::selectStuff( int msec )
{
	struct timeval tval;
	tval.tv_usec = msec * 1000;
	tval.tv_sec = 0;
	
	fd_set readSet, writeSet;
	
	FD_ZERO( &readSet);
	writeSet = readSet;
	
	int fd = -1;
	if( isConnected() ) {
		// if not connected then all sets are zero and select will just timeout
		fd = ePosixClient->fd();
		assert( fd >= 0 );
		
		FD_SET( fd, &readSet);
		if( !ePosixClient->isOutBufferEmpty()) {
			FD_SET( fd, &writeSet);
		}
	}
	int ret = select( fd + 1,
		&readSet, &writeSet, NULL, &tval );
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

