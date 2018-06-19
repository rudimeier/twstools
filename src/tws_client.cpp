/*** tws_client.cpp -- IB/API TWSClient class
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_client.h"
#include "tws_util.h"
#include "debug.h"

#include <twsapi/twsapi_config.h>
#include <twsapi/Contract.h>
#include <twsapi/Order.h>
#include <twsapi/OrderState.h>
#include <twsapi/Execution.h>
#include <twsapi/TwsSocketClientErrors.h>
#include <twsapi/EWrapper.h>
#if ! defined TWS_ORIG_CLIENT
# include <twsapi/EPosixClientSocket.h>
#else
# include <twsapi/EReader.h>
# include <twsapi/EClientSocket.h>
#endif

#if defined HAVE_CONFIG_H
# include "config.h"
#endif  /* HAVE_CONFIG_H */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#if ! defined __CYGWIN__
# if defined HAVE_WINSOCK2_H
#  include "winsock2.h"
# endif
#endif /* __CYGWIN__ */

// debug verbosity 0...n
#define tws_debug_level 1
#define TWS_DEBUG( _level, _fmt, _msg... )        \
	if( tws_debug_level >= _level ) {    \
		DEBUG_PRINTF("tws debug: " _fmt, ## _msg); \
	}

#if TWSAPI_IB_VERSION_NUMBER < 97200
# define lastTradeDateOrContractMonth expiry
#endif

TWSClient::TWSClient( EWrapper *ew ) :
	myEWrapper(ew)
{
#if ! defined TWS_ORIG_CLIENT
	ePosixClient = new EPosixClientSocket(ew);
#else
	eSignal = new EReaderOSSignal(2000);
	ePosixClient = new EClientSocket(ew, eSignal);
#endif

#if TWSAPI_IB_VERSION_NUMBER >= 97200
	ePosixClient->asyncEConnect(false);
#endif
}


TWSClient::~TWSClient()
{
	delete ePosixClient;
}


bool TWSClient::isConnected() const
{
	return ePosixClient->isConnected();
}


bool TWSClient::connectTWS( const std::string &host, int port, int clientId,
	int ai_family )
{
	DEBUG_PRINTF("connect: %s:%d, clientId: %d", host.c_str(), port, clientId);

#if ! defined TWS_ORIG_CLIENT
	return ePosixClient->eConnect2( host.c_str(), port, clientId, ai_family );
#else
	bool ret = ePosixClient->eConnect( host.c_str(), port, clientId );
	if (ret) {
		eReader = new EReader(ePosixClient, eSignal);
		eReader->start();
	}
	return ret;
#endif
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
	assert( msec >= 0 );
	//DEBUG_PRINTF("usleep ....." );
	//usleep(2000 * 1000);

#if ! defined TWS_ORIG_CLIENT
# if TWSAPI_IB_VERSION_NUMBER >= 97200
	ePosixClient->select_timeout(msec);
# else
	struct timeval tval;
	tval.tv_sec = msec / 1000 ;
	tval.tv_usec = (msec % 1000) * 1000;

	fd_set readSet, writeSet;

	FD_ZERO( &readSet);
	FD_ZERO( &writeSet);

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
		TWS_DEBUG( 1 , "Select failed: %s, fd: %d, timval: (%lds, %ldus).",
			strerror(errno), fd, (long)tval.tv_sec, (long)tval.tv_usec );
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
# endif
#else
	DEBUG_PRINTF("waitForSignal ....." );
	eSignal->waitForSignal();
	errno = 0;
	DEBUG_PRINTF("processMsgs ....." );
	eReader->processMsgs();
#endif
}


int TWSClient::serverVersion()
{
#if TWSAPI_IB_VERSION_NUMBER >= 97200
	return ePosixClient->EClient::serverVersion();
#else
	return ePosixClient->serverVersion();
#endif
}

std::string TWSClient::TwsConnectionTime()
{
	return ePosixClient->TwsConnectionTime();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////


void TWSClient::reqMktData(int tickerId, const Contract &contract,
	const  std::string &genericTickList, bool snapshot)
{
	DEBUG_PRINTF( "REQ_MKT_DATA %d "
		"'%s' '%ld' '%s' '%s' '%s' '%s' '%g' '%s' '%s' '%s' %d",
		tickerId, contract.symbol.c_str(), contract.conId, contract.exchange.c_str(),
		contract.secType.c_str(), contract.lastTradeDateOrContractMonth.c_str(),
		contract.right.c_str(), contract.strike, contract.currency.c_str(),
		contract.localSymbol.c_str(), genericTickList.c_str(), snapshot );

	ePosixClient->reqMktData( tickerId, contract, genericTickList, snapshot
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	, false
#endif
#if TWSAPI_IB_VERSION_NUMBER >= 971
	, TagValueListSPtr()
#endif
	);
}


void TWSClient::cancelMktData ( int tickerId )
{
	DEBUG_PRINTF("CANCEL_MKT_DATA %d", tickerId);

	ePosixClient->cancelMktData( tickerId );
}


void TWSClient::placeOrder ( int id, const Contract &contract,
	const Order &order )
{
	DEBUG_PRINTF("PLACE_ORDER %d '%s' %g '%s' %g '%s' %ld",
		id, order.orderType.c_str(), (double)order.totalQuantity, order.action.c_str(),
		order.lmtPrice, contract.symbol.c_str(), contract.conId );

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


void TWSClient::reqExecutions(int reqId, const ExecutionFilter& filter)
{
	DEBUG_PRINTF("REQ_EXECUTIONS %d", reqId);

	ePosixClient->reqExecutions(reqId, filter);
}


void TWSClient::reqIds( int numIds)
{
	DEBUG_PRINTF("REQ_IDS %d", numIds);

	ePosixClient->reqIds( numIds );
}


void TWSClient::reqContractDetails( int reqId, const Contract &contract )
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


void TWSClient::reqHistoricalData ( int tickerId, const Contract &contract,
	const std::string &endDateTime, const std::string &durationStr,
	const std::string &barSizeSetting, const std::string &whatToShow,
	int useRTH, int formatDate )
{
#if 0
	DEBUG_PRINTF("REQ_HISTORICAL_DATA %d "
		"'%s' '%s' '%s' '%s' '%s' '%s' '%s' %d %d",
		tickerId, contract.symbol.c_str(), contract.secType.c_str(),
		contract.exchange.c_str(), endDateTime.c_str(), durationStr.c_str(),
		barSizeSetting.c_str(), whatToShow.c_str(), useRTH, formatDate );
#endif

	ePosixClient->reqHistoricalData( tickerId, contract, endDateTime,
		durationStr, barSizeSetting, whatToShow, useRTH, formatDate
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	,false
#endif
#if TWSAPI_IB_VERSION_NUMBER >= 971
	, TagValueListSPtr()
#endif
	);
}


void TWSClient::reqCurrentTime()
{
	DEBUG_PRINTF("REQ_CURRENT_TIME");

	ePosixClient->reqCurrentTime();
}

void TWSClient::reqMarketDataType(int marketDataType)
{
	DEBUG_PRINTF("REQ_MARKET_DATA_TYPE %d", marketDataType);

	ePosixClient->reqMarketDataType(marketDataType);
}

void TWSClient::reqSecDefOptParams(int reqId, const Contract &c)
{
	DEBUG_PRINTF("REQ_OPT_PARAMS %d '%s' '%s' '%s' '%ld'", reqId,
		c.symbol.c_str(), c.exchange.c_str(), c.secType.c_str(), c.conId);

	ePosixClient->reqSecDefOptParams(reqId,
		c.symbol, c.exchange, c.secType, c.conId);
}
