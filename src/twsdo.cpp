/*** twsdo.cpp -- TWS job processing tool
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

#include "twsdo.h"
#include "tws_meta.h"
#include "tws_query.h"
#include "tws_util.h"
#include "tws_client.h"
#include "tws_wrapper.h"
#include "debug.h"

// from global installed twsapi
#include "twsapi/Contract.h"

#if defined HAVE_CONFIG_H
# include "config.h"
#endif  /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>

#if defined _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif




ConfigTwsdo::ConfigTwsdo()
{
	workfile = "";
	skipdef = 0;
	tws_host = "localhost";
	tws_port = 7474;
	tws_client_id = 123;
	ai_family = AF_UNSPEC;
	
	get_account = 0;
	tws_account_name = "";
	get_exec = 0;
	get_order = 0;
	do_mm = 0;
	
	tws_conTimeout = 30000;
	tws_reqTimeout = 1200000;
	tws_maxRequests = 60;
	tws_pacingInterval = 605000;
	tws_minPacingTime = 1000;
	tws_violationPause = 60000;
}

void ConfigTwsdo::init_ai_family( int ipv4, int ipv6 )
{
	if( !ipv4 && !ipv6 ) {
		/* default is ipv4 only */
		ai_family = AF_INET;
		return;
	}
#if ! defined TWSAPI_IPV6
	DEBUG_PRINTF( "Warning, specifying address family is not supported in "
		"twsapi (" TWSAPI_VERSION "), upgrade it and recompile.");
#endif
	if( ipv4 && ipv6 ) {
		/* this will be the default one day */
		ai_family = AF_UNSPEC;
	} else if( ipv4 ) {
		ai_family = AF_INET;
	} else if( ipv6 ) {
		ai_family = AF_INET6;
	} else {
		/* no more cases */
		assert(false);
	}
}




class TwsDlWrapper : public DebugTwsWrapper
{
	public:
		TwsDlWrapper( TwsDL* parent );
		virtual ~TwsDlWrapper();
		
		void connectionClosed();
		
		void error( const int id, const int errorCode,
			const IB::IBString errorString );
		void contractDetails( int reqId,
			const IB::ContractDetails& contractDetails );
		void bondContractDetails( int reqId,
			const IB::ContractDetails& contractDetails );
		void contractDetailsEnd( int reqId );
		void historicalData( IB::TickerId reqId, const IB::IBString& date,
			double open, double high, double low, double close, int volume,
			int barCount, double WAP, int hasGaps );
		void updateAccountValue( const std::string& key,
			const IB::IBString& val, const std::string& currency,
			const IB::IBString& accountName );
		void updatePortfolio( const IB::Contract& contract, int position,
			double marketPrice, double marketValue, double averageCost,
			double unrealizedPNL, double realizedPNL,
			const IB::IBString& accountName);
		void updateAccountTime( const IB::IBString& timeStamp );
		void accountDownloadEnd( const IB::IBString& accountName );
		void execDetails( int reqId, const IB::Contract& contract,
			const IB::Execution& execution );
		void execDetailsEnd( int reqId );
		void orderStatus( IB::OrderId orderId, const IB::IBString &status,
			int filled, int remaining, double avgFillPrice, int permId,
			int parentId, double lastFillPrice, int clientId,
			const IB::IBString& whyHeld );
		void openOrder( IB::OrderId orderId, const IB::Contract&,
			const IB::Order&, const IB::OrderState& );
		void openOrderEnd();
		void currentTime( long time );
		void nextValidId( IB::OrderId orderId );
		
	private:
		TwsDL* parentTwsDL;
};


TwsDlWrapper::TwsDlWrapper( TwsDL* parent ) :
	parentTwsDL(parent)
{
}


TwsDlWrapper::~TwsDlWrapper()
{
}


void TwsDlWrapper::connectionClosed()
{
	parentTwsDL->twsConnectionClosed();
}


void TwsDlWrapper::error( const int id, const int errorCode,
	const IB::IBString errorString )
{
	RowError row = { id, errorCode, errorString };
	parentTwsDL->twsError( row );
}


void TwsDlWrapper::contractDetails( int reqId,
	const IB::ContractDetails& contractDetails )
{
	parentTwsDL->twsContractDetails(
		reqId, contractDetails );
}


void TwsDlWrapper::bondContractDetails( int reqId,
	const IB::ContractDetails& contractDetails )
{
	parentTwsDL->twsBondContractDetails(
		reqId, contractDetails );
}


void TwsDlWrapper::contractDetailsEnd( int reqId )
{
	parentTwsDL->twsContractDetailsEnd(
		reqId);
}


void TwsDlWrapper::historicalData( IB::TickerId reqId, const IB::IBString& date,
	double open, double high, double low, double close, int volume,
	int count, double WAP, int hasGaps )
{
	RowHist row = { date, open, high, low, close, volume, count, WAP, hasGaps };
	parentTwsDL->twsHistoricalData( reqId, row );
}


void TwsDlWrapper::updateAccountValue( const IB::IBString& key,
	const IB::IBString& val, const IB::IBString& currency,
	const IB::IBString& accountName )
{
	RowAccVal row = { key, val, currency, accountName };
	parentTwsDL->twsUpdateAccountValue( row );
}

void TwsDlWrapper::updatePortfolio( const IB::Contract& contract,
	int position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IB::IBString& accountName)
{
	RowPrtfl row = { contract, position, marketPrice, marketValue, averageCost,
		unrealizedPNL, realizedPNL, accountName};
	parentTwsDL->twsUpdatePortfolio( row );
}

void TwsDlWrapper::updateAccountTime( const IB::IBString& timeStamp )
{
	parentTwsDL->twsUpdateAccountTime( timeStamp );
}

void TwsDlWrapper::accountDownloadEnd( const IB::IBString& accountName )
{
	DebugTwsWrapper::accountDownloadEnd( accountName );
	parentTwsDL->twsAccountDownloadEnd( accountName );
}

void TwsDlWrapper::execDetails( int reqId, const IB::Contract& contract,
	const IB::Execution& execution )
{
	RowExecution row = { contract, execution };
	parentTwsDL->twsExecDetails(  reqId, row );
}

void TwsDlWrapper::execDetailsEnd( int reqId )
{
	DebugTwsWrapper::execDetailsEnd( reqId );
	parentTwsDL->twsExecDetailsEnd( reqId );
}

void TwsDlWrapper::orderStatus( IB::OrderId orderId, const IB::IBString &status,
	int filled, int remaining, double avgFillPrice, int permId,
	int parentId, double lastFillPrice, int clientId,
	const IB::IBString& whyHeld )
{
	RowOrderStatus row = { orderId, status, filled, remaining, avgFillPrice,
		permId, parentId, lastFillPrice, clientId, whyHeld };
	parentTwsDL->twsOrderStatus(row);
}

void TwsDlWrapper::openOrder( IB::OrderId orderId, const IB::Contract& c,
	const IB::Order& o, const IB::OrderState& os)
{
	RowOpenOrder row = { orderId, c, o, os };
	parentTwsDL->twsOpenOrder(row);
}

void TwsDlWrapper::openOrderEnd()
{
	DebugTwsWrapper::openOrderEnd();
	parentTwsDL->twsOpenOrderEnd();
}

void TwsDlWrapper::currentTime( long time )
{
	DebugTwsWrapper::currentTime(time);
	parentTwsDL->twsCurrentTime( time );
}

void TwsDlWrapper::nextValidId( IB::OrderId orderId )
{
	DebugTwsWrapper::nextValidId(orderId);
	parentTwsDL->nextValidId( orderId );
}




TwsDL::TwsDL( const ConfigTwsdo &c ) :
	state(IDLE),
	error(0),
	lastConnectionTime(0),
	tws_time(0),
	tws_valid_orderId(0),
	connectivity_IB_TWS(false),
	curIdleTime(0),
	cfg(c),
	twsWrapper(NULL),
	twsClient(NULL),
	msgCounter(0),
	currentRequest(  *(new GenericRequest()) ),
	workTodo( new WorkTodo() ),
	packet( NULL ),
	dataFarms( *(new DataFarmStates()) ),
	pacingControl( *(new PacingGod(dataFarms)) )
{
	pacingControl.setPacingTime( cfg.tws_maxRequests,
		cfg.tws_pacingInterval, cfg.tws_minPacingTime );
	pacingControl.setViolationPause( cfg.tws_violationPause );
	initTwsClient();
	initWork();
}


TwsDL::~TwsDL()
{
	delete &currentRequest;
	
	if( twsClient != NULL ) {
		delete twsClient;
	}
	if( twsWrapper != NULL ) {
		delete twsWrapper;
	}
	if( workTodo != NULL ) {
		delete workTodo;
	}
	if( packet != NULL ) {
		delete packet;
	}
	delete &pacingControl;
	delete &dataFarms;
}


int TwsDL::start()
{
	assert( state == IDLE );
	
	curIdleTime = 0;
	eventLoop();
	return error;
}


void TwsDL::eventLoop()
{
	bool run = true;
	while( run ) {
		if( curIdleTime > 0 ) {
			twsClient->selectStuff( curIdleTime );
		}
		switch( state ) {
			case WAIT_TWS_CON:
				waitTwsCon();
				break;
			case IDLE:
				idle();
				break;
			case WAIT_DATA:
				waitData();
				break;
			case QUIT:
				run = false;
				break;
		}
	}
}


void TwsDL::connectTws()
{
	assert( !twsClient->isConnected() && !connectivity_IB_TWS );
	
	int64_t w = nowInMsecs() - lastConnectionTime;
	if( w < cfg.tws_conTimeout ) {
		DEBUG_PRINTF( "Waiting %ldms before connecting again.",
			(long)(cfg.tws_conTimeout - w) );
		curIdleTime = cfg.tws_conTimeout - w;
		return;
	}
	
	lastConnectionTime = nowInMsecs();
	changeState( WAIT_TWS_CON );
	
	/* this may callback (negative) error messages only */
	bool con = twsClient->connectTWS( cfg.tws_host, cfg.tws_port,
		cfg.tws_client_id, cfg.ai_family );
	assert( con == twsClient->isConnected() );
	
	if( !con ) {
		DEBUG_PRINTF("TWS connection failed.");
		changeState(IDLE);
	} else {
		DEBUG_PRINTF("TWS connection established: %d, %s",
			twsClient->serverVersion(), twsClient->TwsConnectionTime().c_str());
		/* this must be set before any possible "Connectivity" callback msg */
		connectivity_IB_TWS = true;
		/* waiting for first messages until tws_time is received */
		tws_time = 0;
		twsClient->reqCurrentTime();
	}
}


void TwsDL::waitTwsCon()
{
	int64_t w = cfg.tws_conTimeout - (nowInMsecs() - lastConnectionTime);
	
	if( twsClient->isConnected() ) {
		if( tws_time != 0 ) {
			DEBUG_PRINTF( "Connection process finished." );
			changeState( IDLE );
		} else if( w > 0 ) {
			DEBUG_PRINTF( "Still waiting for connection finish." );
			curIdleTime = w;
		} else {
			DEBUG_PRINTF( "Timeout connecting TWS." );
			twsClient->disconnectTWS();
		}
	} else {
		// TODO print a specific error
		DEBUG_PRINTF( "Connecting TWS failed." );
		changeState( IDLE );
	}
}


void TwsDL::idle()
{
	assert(currentRequest.reqType() == GenericRequest::NONE);
	
	if( !twsClient->isConnected() ) {
		connectTws();
		return;
	}

	// HACK
	static int fuckme = 0;
	if( fuckme <= 0 ) {
		fuckme = reqMktData();
	}

	GenericRequest::ReqType reqType = workTodo->nextReqType();
	switch( reqType ) {
	case GenericRequest::ACC_STATUS_REQUEST:
		reqAccStatus();
		break;
	case GenericRequest::EXECUTIONS_REQUEST:
		reqExecutions();
		break;
	case GenericRequest::ORDERS_REQUEST:
		reqOrders();
		break;
	case GenericRequest::CONTRACT_DETAILS_REQUEST:
		reqContractDetails();
		break;
	case GenericRequest::HIST_REQUEST:
		reqHistoricalData();
		break;
	case GenericRequest::PLACE_ORDER:
		placeOrder();
		break;
	case GenericRequest::CANCEL_ORDER:
		cancelOrder();
		break;
	case GenericRequest::NONE:
		break;
	}
	
	if( reqType == GenericRequest::NONE && fuckme <= 1 ) {
		_lastError = "No more work to do.";
		changeState( QUIT );
	}
}


void TwsDL::waitData()
{
	if( packet == NULL ) {
		return;
	}

	if( !packet->finished() ) {
		if( currentRequest.age() <= cfg.tws_reqTimeout ) {
			static int64_t last = 0;
			int64_t now = nowInMsecs();
			if( now - last > 2000 ) {
				last = now;
				DEBUG_PRINTF( "Still waiting for data." );
			}
			return;
		} else {
			DEBUG_PRINTF( "Timeout waiting for data." );
			packet->closeError( REQ_ERR_TIMEOUT );
		}
	}
	
	bool ok = false;
	switch( currentRequest.reqType() ) {
	case GenericRequest::CONTRACT_DETAILS_REQUEST:
		ok = finContracts();
		break;
	case GenericRequest::HIST_REQUEST:
		ok = finHist();
		break;
	case GenericRequest::PLACE_ORDER:
		ok = finPlaceOrder();
		break;
	case GenericRequest::CANCEL_ORDER:
		ok = finCancelOrder();
		break;
	case GenericRequest::ACC_STATUS_REQUEST:
	case GenericRequest::EXECUTIONS_REQUEST:
	case GenericRequest::ORDERS_REQUEST:
		packet->dumpXml();
		ok = true;
		break;
	case GenericRequest::NONE:
		assert(false);
		break;
	}
	if( ok ) {
		changeState( IDLE );
	} else {
		error = 1;
		_lastError = "Fatal error.";
		changeState( QUIT );
	}
	delete packet;
	packet = NULL;
	currentRequest.close();
}


bool TwsDL::finContracts()
{
	switch( packet->getError() ) {
	case REQ_ERR_NONE:
		DEBUG_PRINTF( "Contracts received: %zu",
		((PacketContractDetails*)packet)->constList().size() );
		packet->dumpXml();
	case REQ_ERR_NODATA:
	case REQ_ERR_NAV:
	case REQ_ERR_REQUEST:
		break;
	case REQ_ERR_TWSCON:
	case REQ_ERR_TIMEOUT:
		return false;
	}
	return true;
}


bool TwsDL::finHist()
{
	HistTodo *histTodo = workTodo->histTodo();
	
	switch( packet->getError() ) {
	case REQ_ERR_NONE:
		packet->dumpXml();
	case REQ_ERR_NODATA:
	case REQ_ERR_NAV:
		histTodo->tellDone();
		break;
	case REQ_ERR_TWSCON:
		histTodo->cancelForRepeat(0);
		break;
	case REQ_ERR_TIMEOUT:
		histTodo->cancelForRepeat(1);
		break;
	case REQ_ERR_REQUEST:
		histTodo->cancelForRepeat(2);
		break;
	}
	return true;
}


bool TwsDL::finPlaceOrder()
{
	switch( packet->getError() ) {
	case REQ_ERR_NONE:
	case REQ_ERR_REQUEST:
	case REQ_ERR_TIMEOUT:
		packet->dumpXml();
	case REQ_ERR_NODATA:
	case REQ_ERR_NAV:
		break;
	case REQ_ERR_TWSCON:
		return false;
	}
	return true;
}


bool TwsDL::finCancelOrder()
{
	switch( packet->getError() ) {
	case REQ_ERR_NONE:
	case REQ_ERR_REQUEST:
	case REQ_ERR_TIMEOUT:
		packet->dumpXml();
	case REQ_ERR_NODATA:
	case REQ_ERR_NAV:
		break;
	case REQ_ERR_TWSCON:
		return false;
	}
	return true;
}


void TwsDL::initTwsClient()
{
	assert( twsClient == NULL && twsWrapper == NULL );
	
	twsWrapper = new TwsDlWrapper(this);
	twsClient = new TWSClient( twsWrapper );
}


#define ERR_MATCH( _strg_  ) \
	( err.msg.find(_strg_) != std::string::npos )

void TwsDL::twsError( const RowError& err )
{
	msgCounter++;
	
	if( !twsClient->isConnected() ) {
		switch( err.code ) {
		default:
		case 504: /* NOT_CONNECTED */
			DEBUG_PRINTF( "fatal: %d %d %s", err.id, err.code, err.msg.c_str());
			assert(false);
			break;
		case 503: /* UPDATE_TWS */
			DEBUG_PRINTF( "error: %s", err.msg.c_str() );
			break;
		case 502: /* CONNECT_FAIL */
			DEBUG_PRINTF( "connection failed: %s", err.msg.c_str());
			break;
		}
		return;
	}
	
	if( err.id == currentRequest.reqId() ) {
		DEBUG_PRINTF( "TWS message for request %d: %d '%s'",
			err.id, err.code, err.msg.c_str() );
		if( state == WAIT_DATA ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				errorContracts( err );
				break;
			case GenericRequest::HIST_REQUEST:
				errorHistData( err );
				break;
			case GenericRequest::PLACE_ORDER:
				errorPlaceOrder( err );
				break;
			case GenericRequest::CANCEL_ORDER:
				errorCancelOrder( err );
				break;
			case GenericRequest::ACC_STATUS_REQUEST:
			case GenericRequest::EXECUTIONS_REQUEST:
			case GenericRequest::ORDERS_REQUEST:
			case GenericRequest::NONE:
				assert( false );
				break;
			}
		} else {
			// TODO
		}
		return;
	}
	
	if( err.id != -1 ) {
		DEBUG_PRINTF( "TWS message for unexpected request %d: %d '%s'",
			err.id, err.code, err.msg.c_str() );
		return;
	}
	
	DEBUG_PRINTF( "TWS message generic: %d %s", err.code, err.msg.c_str() );
	
	// TODO do better
	switch( err.code ) {
		case 1100:
			assert(ERR_MATCH("Connectivity between IB and T"));
			assert(ERR_MATCH(" has been lost."));
			connectivity_IB_TWS = false;
			curIdleTime = cfg.tws_reqTimeout;
			break;
		case 1101:
			assert(ERR_MATCH("Connectivity between IB and T"));
			assert(ERR_MATCH(" has been restored - data lost."));
			connectivity_IB_TWS = true;
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				packet->closeError( REQ_ERR_TWSCON );
			}
			break;
		case 1102:
			assert(ERR_MATCH("Connectivity between IB and T"));
			assert(ERR_MATCH(" has been restored - data maintained."));
			connectivity_IB_TWS = true;
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				packet->closeError( REQ_ERR_TWSCON );
			}
			break;
		case 1300:
			assert(ERR_MATCH("TWS socket port has been reset and this connection is being dropped."));
			break;
		case 2103:
		case 2104:
		case 2105:
		case 2106:
		case 2107:
		case 2108:
			dataFarms.notify( msgCounter, err.code, err.msg );
			pacingControl.clear();
			break;
		case 2110:
			/* looks like we get that only on fresh connection */
			assert(ERR_MATCH("Connectivity between TWS and server is broken. It will be restored automatically."));
			connectivity_IB_TWS = false;
			break;
	}
}


void TwsDL::errorContracts( const RowError& err )
{
	// TODO
	switch( err.code ) {
		// No security definition has been found for the request"
		case 200:
		if( connectivity_IB_TWS ) {
			packet->closeError( REQ_ERR_REQUEST );
		} else {
			/* using ERR_TIMEOUT instead ERR_TWSCON to push_back this request */
			packet->closeError( REQ_ERR_TIMEOUT );
		}
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	}
}


void TwsDL::errorHistData( const RowError& err )
{
	const IB::Contract &curContract = workTodo->getHistTodo().current().ibContract;
	const HistRequest *cur_hR = &workTodo->getHistTodo().current();
	PacketHistData &p_histData = *((PacketHistData*)packet);
	switch( err.code ) {
	// Historical Market Data Service error message:
	case 162:
		if( ERR_MATCH("Historical data request pacing violation") ) {
			p_histData.closeError( REQ_ERR_TWSCON );
			pacingControl.notifyViolation( curContract );
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			DEBUG_PRINTF( "READY - NO DATA %p %d", cur_hR, err.id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_NODATA );
		} else if( ERR_MATCH("No historical market data for") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE on HMDS server. "
				"%p %d", cur_hR, err.id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_NAV );
		} else if( ERR_MATCH("No data of type EODChart is available") ||
			ERR_MATCH("No data of type DayChart is available") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE (no HMDS route). "
				"%p %d", cur_hR, err.id );
			p_histData.closeError( REQ_ERR_NAV );
		} else if( ERR_MATCH("No market data permissions for") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_REQUEST );
		} else if( ERR_MATCH("Unknown contract") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_REQUEST );
		} else {
			DEBUG_PRINTF( "Warning, unhandled error message." );
			// seen: "TWS exited during processing of HMDS query"
		}
		break;
	// Historical Market Data Service query message:
	case 165:
		if( ERR_MATCH("HMDS server disconnect occurred.  Attempting reconnection") ||
		    ERR_MATCH("HMDS connection attempt failed.  Connection will be re-attempted") ) {
			curIdleTime = cfg.tws_reqTimeout;
		} else if( ERR_MATCH("HMDS server connection was successful") ) {
			dataFarms.learnHmdsLastOk( msgCounter, curContract );
		} else {
			DEBUG_PRINTF( "Warning, unhandled error message." );
		}
		break;
	// No security definition has been found for the request"
	case 200:
		// NOTE we could find out more to throw away similar worktodo
		// TODO "The contract description specified for DESX5 is ambiguous;\nyou must specify the multiplier."
		if( connectivity_IB_TWS ) {
			p_histData.closeError( REQ_ERR_REQUEST );
		} else {
			/* using ERR_TIMEOUT instead ERR_TWSCON to push_back this request */
			p_histData.closeError( REQ_ERR_TIMEOUT );
		}
		break;
	// Order rejected - Reason:
	case 201:
	// Order cancelled - Reason:
	case 202:
		DEBUG_PRINTF( "Warning, unexpected error code." );
		break;
	// The security <security> is not available or allowed for this account
	case 203:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	// Server error when validating an API client request
	case 321:
		// comes directly from TWS whith prefix "Error validating request:-"
		// NOTE we could find out more to throw away similar worktodo
		p_histData.closeError( REQ_ERR_REQUEST );
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	}
}


void TwsDL::errorPlaceOrder( const RowError& err )
{
	PacketPlaceOrder &p_pO = *((PacketPlaceOrder*)packet);
	p_pO.append( err );
	switch( err.code ) {
	default:
		p_pO.closeError( REQ_ERR_REQUEST );
		break;
	}
}


void TwsDL::errorCancelOrder( const RowError& err )
{
	PacketCancelOrder &p_cO = *((PacketCancelOrder*)packet);
	p_cO.append( err );
	switch( err.code ) {
	case 202:
		/* "Order cancelled", this is expected here and we'll wait for some
		   more orderStatus callbacks */
		break;
	default:
		p_cO.closeError( REQ_ERR_REQUEST );
		break;
	}
}


#undef ERR_MATCH


void TwsDL::twsConnectionClosed()
{
	DEBUG_PRINTF( "disconnected in state %d", state );
	
	if( state == WAIT_DATA ) {
		if( !packet->finished() ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
			case GenericRequest::PLACE_ORDER:
			case GenericRequest::CANCEL_ORDER:
			case GenericRequest::ACC_STATUS_REQUEST:
			case GenericRequest::EXECUTIONS_REQUEST:
			case GenericRequest::ORDERS_REQUEST:
				assert(false); // TODO repeat
				break;
			case GenericRequest::HIST_REQUEST:
				packet->closeError( REQ_ERR_TWSCON );
				break;
			case GenericRequest::NONE:
				assert(false);
				break;
			}
		}
	}
	
	connectivity_IB_TWS = false;
	dataFarms.setAllBroken();
	pacingControl.clear();
	curIdleTime = 0;
}


void TwsDL::twsContractDetails( int reqId, const IB::ContractDetails &ibContractDetails )
{
	if( currentRequest.reqType() != GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		assert( false );
	}
	
	((PacketContractDetails*)packet)->append(reqId, ibContractDetails);
}


void TwsDL::twsBondContractDetails( int reqId, const IB::ContractDetails &ibContractDetails )
{
	if( currentRequest.reqType() != GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		assert( false );
	}
	
	((PacketContractDetails*)packet)->append(reqId, ibContractDetails);
}


void TwsDL::twsContractDetailsEnd( int reqId )
{
	if( currentRequest.reqType() != GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		assert( false );
	}
	
	((PacketContractDetails*)packet)->setFinished();
}


void TwsDL::twsHistoricalData( int reqId, const RowHist &row )
{
	if( currentRequest.reqType() != GenericRequest::HIST_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		return;
	}
	
	// TODO we shouldn't do this each row
	dataFarms.learnHmds( workTodo->getHistTodo().current().ibContract );
	
	assert( !packet->finished() );
	((PacketHistData*)packet)->append( reqId, row );
	
	if( packet->finished() ) {
		DEBUG_PRINTF( "READY %p %d",
			&workTodo->getHistTodo().current(), reqId );
	}
}

void TwsDL::twsUpdateAccountValue( const RowAccVal& row )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (updateAccountValue).");
		return;
	}
	((PacketAccStatus*)packet)->append( row );
}

void TwsDL::twsUpdatePortfolio( const RowPrtfl& row )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (updatePortfolio).");
		return;
	}
	((PacketAccStatus*)packet)->append( row );
}

void TwsDL::twsUpdateAccountTime( const std::string& timeStamp )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (updateAccountTime).");
		return;
	}
	((PacketAccStatus*)packet)->appendUpdateAccountTime( timeStamp );
}

void TwsDL::twsAccountDownloadEnd( const std::string& accountName )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (accountDownloadEnd).");
		return;
	}
	((PacketAccStatus*)packet)->appendAccountDownloadEnd( accountName );
}

void TwsDL::twsExecDetails( int reqId, const RowExecution &row )
{
	if( currentRequest.reqType() != GenericRequest::EXECUTIONS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (execDetails).");
		return;
	}
	((PacketExecutions*)packet)->append( reqId, row );
}

void TwsDL::twsExecDetailsEnd( int reqId )
{
	if( currentRequest.reqType() != GenericRequest::EXECUTIONS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (execDetailsEnd).");
		return;
	}
	((PacketExecutions*)packet)->appendExecutionsEnd( reqId );
}

void TwsDL::twsOrderStatus( const RowOrderStatus& row )
{
	if( currentRequest.reqType() == GenericRequest::ORDERS_REQUEST ) {
		((PacketOrders*)packet)->append(row);
		return;
	}
	if( currentRequest.reqType() == GenericRequest::PLACE_ORDER ) {
		((PacketPlaceOrder*)packet)->append(row);
		return;
	}
	if( currentRequest.reqType() == GenericRequest::CANCEL_ORDER ) {
		((PacketCancelOrder*)packet)->append(row);
		return;
	}
	DEBUG_PRINTF( "Warning, unexpected tws callback (orderStatus).");
}

void TwsDL::twsOpenOrder( const RowOpenOrder& row )
{
	if( currentRequest.reqType() == GenericRequest::ORDERS_REQUEST ) {
		((PacketOrders*)packet)->append(row);
		return;
	}
	if( currentRequest.reqType() == GenericRequest::PLACE_ORDER ) {
		((PacketPlaceOrder*)packet)->append(row);
		return;
	}
	DEBUG_PRINTF( "Warning, unexpected tws callback (openOrder).");
}

void TwsDL::twsOpenOrderEnd()
{
	/* this messages usually comes unexpected right after connecting */
	
	if( currentRequest.reqType() != GenericRequest::ORDERS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (openOrderEnd).");
		return;
	}
	
	((PacketOrders*)packet)->appendOpenOrderEnd();
}

void TwsDL::twsCurrentTime( long time )
{
	if( state == WAIT_TWS_CON ) {
		tws_time = time;
	}
	if( state == WAIT_DATA
		&& currentRequest.reqType() == GenericRequest::PLACE_ORDER ) {
		packet->closeError( REQ_ERR_NONE );
	}
}

void TwsDL::nextValidId( long orderId )
{
	if( state == WAIT_TWS_CON ) {
		tws_valid_orderId = orderId;
	}
}


void TwsDL::initWork()
{
	if( cfg.get_account ) {
		workTodo->addSimpleRequest(GenericRequest::ACC_STATUS_REQUEST);
	}
	if( cfg.get_exec ) {
		workTodo->addSimpleRequest(GenericRequest::EXECUTIONS_REQUEST);
	}
	if( cfg.get_order ) {
		workTodo->addSimpleRequest(GenericRequest::ORDERS_REQUEST);
	}
	
	int cnt = workTodo->read_file(cfg.workfile);
	DEBUG_PRINTF( "got %d jobs from workFile %s", cnt, cfg.workfile );
	
	if( workTodo->getContractDetailsTodo().countLeft() > 0 ) {
		DEBUG_PRINTF( "getting contracts from TWS, %d",
			workTodo->getContractDetailsTodo().countLeft() );
// 		state = IDLE;
	}
	if( workTodo->getHistTodo().countLeft() > 0 ) {
		DEBUG_PRINTF( "getting hist data from TWS, %d",
			workTodo->getHistTodo().countLeft());
		dumpWorkTodo();
// 		state = IDLE;;
	}
}


void TwsDL::dumpWorkTodo() const
{
	// HACK just dump it one time
	static bool firstTime = true;
	if( firstTime ) {
		firstTime = false;
		workTodo->getHistTodo().dumpLeft();
	}
}


TwsDL::State TwsDL::currentState() const
{
	return state;
}

std::string TwsDL::lastError() const
{
	return _lastError;
}

void TwsDL::changeState( State s )
{
	assert( state != s );
	state = s;
	
	if( state == WAIT_TWS_CON ) {
		curIdleTime = 50;
	} else if( state == WAIT_DATA ) {
		curIdleTime = 1000;
	} else {
		curIdleTime = 0;
	}
}


void TwsDL::reqContractDetails()
{
	workTodo->contractDetailsTodo()->checkout();
	const ContractDetailsRequest &cdR
		= workTodo->getContractDetailsTodo().current();
	
	PacketContractDetails *p_contractDetails = new PacketContractDetails();
	packet = p_contractDetails;
	currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );
	
	p_contractDetails->record( currentRequest.reqId(), cdR );
	twsClient->reqContractDetails( currentRequest.reqId(), cdR.ibContract() );
	changeState( WAIT_DATA );
}

void TwsDL::reqHistoricalData()
{
	assert( workTodo->getHistTodo().countLeft() > 0 );
	
	int wait = workTodo->histTodo()->checkoutOpt( &pacingControl, &dataFarms );
	
	if( wait > 0 ) {
		curIdleTime = (wait < 1000) ? wait : 1000;
		return;
	}
	if( wait < -1 ) {
		// just debug timer resolution
		DEBUG_PRINTF( "late timeout: %d", wait );
	}
	
	PacketHistData *p_histData = new PacketHistData();
	packet = p_histData;
	currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
	
	
	const HistRequest &hR = workTodo->getHistTodo().current();
	pacingControl.addRequest( hR.ibContract );
	
	const IB::Contract &c = hR.ibContract;
	DEBUG_PRINTF( "REQ_HISTORICAL_DATA %p %d: %ld,%s,%s,%s,%s %s,%s,%s,%s",
		&workTodo->getHistTodo().current(),
		currentRequest.reqId(), c.conId,
		c.symbol.c_str(), c.secType.c_str(),c.exchange.c_str(),c.expiry.c_str(),
		hR.whatToShow.c_str(), hR.endDateTime.c_str(),
		hR.durationStr.c_str(), hR.barSizeSetting.c_str() );
	
	p_histData->record( currentRequest.reqId(), hR );
	twsClient->reqHistoricalData( currentRequest.reqId(),
	                              c,
	                              hR.endDateTime,
	                              hR.durationStr,
	                              hR.barSizeSetting,
	                              hR.whatToShow,
	                              hR.useRTH,
	                              hR.formatDate );
	changeState( WAIT_DATA );
}

void TwsDL::reqAccStatus()
{
	PacketAccStatus *accStatus = new PacketAccStatus();
	packet = accStatus;
	currentRequest.nextRequest( GenericRequest::ACC_STATUS_REQUEST );
	
	AccStatusRequest aR;
	aR.subscribe = true;
	aR.acctCode = cfg.tws_account_name;
	
	accStatus->record( aR );
	twsClient->reqAccountUpdates(aR.subscribe, aR.acctCode);
	changeState( WAIT_DATA );
}

void TwsDL::reqExecutions()
{
	PacketExecutions *executions = new PacketExecutions();
	packet = executions;
	currentRequest.nextRequest( GenericRequest::EXECUTIONS_REQUEST );
	
	ExecutionsRequest eR;
	
	executions->record( currentRequest.reqId(), eR );
	twsClient->reqExecutions( currentRequest.reqId(), eR.executionFilter);
	changeState( WAIT_DATA );
}

void TwsDL::reqOrders()
{
	PacketOrders *orders = new PacketOrders();
	packet = orders;
	currentRequest.nextRequest( GenericRequest::ORDERS_REQUEST );
	
	OrdersRequest oR;
	
	orders->record( oR );
	twsClient->reqAllOpenOrders();
	changeState( WAIT_DATA );
}

void TwsDL::placeOrder()
{
	workTodo->placeOrderTodo()->checkout();
	const PlaceOrder &pO = workTodo->getPlaceOrderTodo().current();

	PacketPlaceOrder *p_placeOrder = new PacketPlaceOrder();
	packet = p_placeOrder;

	long orderId;
	if( pO.orderId == 0 ) {
		orderId = tws_valid_orderId;
		tws_valid_orderId++;
	} else {
		orderId = pO.orderId;
	}
	currentRequest.nextOrderRequest( GenericRequest::PLACE_ORDER, orderId );

	p_placeOrder->record( orderId, pO );
	twsClient->placeOrder( orderId, pO.contract, pO.order );

	if( ! pO.order.transmit ) {
		/* HACK no response is expected on success, in case of error we hope
		   that current time comes after that error */
		twsClient->reqCurrentTime();
	}

	changeState( WAIT_DATA );
}

void TwsDL::cancelOrder()
{
	workTodo->cancelOrderTodo()->checkout();
	const CancelOrder &cO = workTodo->getCancelOrderTodo().current();

	PacketCancelOrder *p_cancelOrder = new PacketCancelOrder();
	packet = p_cancelOrder;

	long orderId = cO.orderId;
	currentRequest.nextOrderRequest( GenericRequest::CANCEL_ORDER, orderId );

	p_cancelOrder->record( orderId, cO );
	twsClient->cancelOrder( orderId );

	changeState( WAIT_DATA );
}

int TwsDL::reqMktData()
{
	const MktDataTodo &mdt = workTodo->getMktDataTodo();
	const std::vector<MktDataRequest> &v =
		workTodo->getMktDataTodo().mktDataRequests;
	std::vector<MktDataRequest>::const_iterator it;
	int reqId = 1;
	for( it = v.begin(); it < v.end(); it++, reqId++ ) {
		twsClient->reqMktData( reqId, it->ibContract, it->genericTicks,
			it->snapshot );
	}
	if( reqId > 1 ) {
		changeState( WAIT_DATA );
	}
	return reqId;
}
