/*** twsDL.cpp -- TWS job processing tool
 *
 * Copyright (C) 2010, 2011 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 *
 * This file is part of atem.
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

#include "twsDL.h"
#include "tws_meta.h"

#include "twsUtil.h"
#include "twsClient.h"
#include "twsWrapper.h"
#include "tws_xml.h"
#include "debug.h"
#include "config.h"

// from global installed twsapi
#include "twsapi/Contract.h"

#include <popt.h>
#include <stdio.h>
#include <string.h>


enum { POPT_HELP, POPT_VERSION, POPT_USAGE };

static poptContext opt_ctx;
static const char *workfilep = "";
static int skipdefp = 0;
static const char *tws_hostp = "localhost";
static int tws_portp = 7474;
static int tws_client_idp = 123;

static int get_accountp = 0;
static const char* tws_account_namep = "";
static int get_execp = 0;
static int get_orderp = 0;

static int tws_conTimeoutp = 30000;
static int tws_reqTimeoutp = 1200000;
static int tws_maxRequestsp = 60;
static int tws_pacingIntervalp = 605000;
static int tws_minPacingTimep = 1000;
static int tws_violationPausep = 60000;


#define VERSION_MSG \
PACKAGE_NAME " " PACKAGE_VERSION "\n\
Copyright (C) 2010-2011 Ruediger Meier <sweet_f_a@gmx.de>\n\
License: BSD 3-Clause\n"


static void displayArgs( poptContext con, poptCallbackReason /*foo*/,
	poptOption *key, const char */*arg*/, void */*data*/ )
{
	switch( key->val ) {
	case POPT_HELP:
		poptPrintHelp(con, stdout, 0);
		break;
	case POPT_VERSION:
		fprintf(stdout, VERSION_MSG);
		break;
	case POPT_USAGE:
		poptPrintUsage(con, stdout, 0);
		break;
	default:
		assert(false);
	}
	
	exit(0);
}

static struct poptOption flow_opts[] = {
	{"verbose-xml", 'x', POPT_ARG_NONE, &skipdefp, 0,
		"Never skip xml default values.", NULL},
	{"host", 'h', POPT_ARG_STRING, &tws_hostp, 0,
		"TWS host name or ip (default: localhost).", NULL},
	{"port", 'p', POPT_ARG_INT, &tws_portp, 0,
		"TWS port number (default: 7474).", NULL},
	{"id", 'i', POPT_ARG_INT, &tws_client_idp, 0,
		"TWS client connection id (default: 123).", NULL},
	{"get-account", 'A', POPT_ARG_NONE, &get_accountp, 0,
		"Request account status.", NULL},
	{"accountName", '\0', POPT_ARG_STRING, &tws_account_namep, 0,
		"IB account name (default: \"\").", NULL},
	{"get-exec", 'E', POPT_ARG_NONE, &get_execp, 0,
		"Request executions.", NULL},
	{"get-order", 'O', POPT_ARG_NONE, &get_orderp, 0,
		"Request open orders.", NULL},
	POPT_TABLEEND
};

static struct poptOption tws_tweak_opts[] = {
	{"conTimeout", '\0', POPT_ARG_INT, &tws_conTimeoutp, 0,
	"Connection timeout (default: 30000).", "ms"},
	{"reqTimeout", '\0', POPT_ARG_INT, &tws_reqTimeoutp, 0,
	"Request timeout (default: 1200000).", "ms"},
	{"maxRequests", '\0', POPT_ARG_INT, &tws_maxRequestsp, 0,
	"Max requests per pacing interval (default: 60).", NULL},
	{"pacingInterval", '\0', POPT_ARG_INT, &tws_pacingIntervalp, 0,
	"Pacing interval (default: 605000).", "ms"},
	{"minPacingTime", '\0', POPT_ARG_INT, &tws_minPacingTimep, 0,
	"Minimum time to wait between requests (default: 1000).", "ms"},
	{"violationPause", '\0', POPT_ARG_INT, &tws_violationPausep, 0,
	"Time to wait if pacing violation occurs (default: 60000).", "ms"},
	POPT_TABLEEND
};

static struct poptOption help_opts[] = {
	{NULL, '\0', POPT_ARG_CALLBACK, (void*)displayArgs, 0, NULL, NULL},
	{"help", '\0', POPT_ARG_NONE, NULL, POPT_HELP,
		"Show this help message.", NULL},
	{"version", '\0', POPT_ARG_NONE, NULL, POPT_VERSION,
		"Print version string and exit.", NULL},
	{"usage", '\0', POPT_ARG_NONE, NULL, POPT_USAGE,
		"Display brief usage message." , NULL},
	POPT_TABLEEND
};

static const struct poptOption twsDL_opts[] = {
	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, flow_opts, 0,
	 "Program advice:", NULL},
	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, tws_tweak_opts, 0,
	 "TWS tweaks:", NULL},
	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, help_opts, 0,
	 "Help options:", NULL},
	POPT_TABLEEND
};

void clear_popt()
{
	poptFreeContext(opt_ctx);
}

void twsDL_parse_cl(size_t argc, const char *argv[])
{
	opt_ctx = poptGetContext(NULL, argc, argv, twsDL_opts, 0);
	atexit(clear_popt);
	
	poptSetOtherOptionHelp( opt_ctx, "[OPTION]... [WORK_FILE]");
	
	int rc;
	while( (rc = poptGetNextOpt(opt_ctx)) > 0 ) {
		// handle options when we have returning ones
		assert(false);
	}
	
	if( rc != -1 ) {
		fprintf( stderr, "error: %s '%s'\n",
			poptStrerror(rc), poptBadOption(opt_ctx, 0) );
		exit(2);
	}
	
	const char** rest = poptGetArgs(opt_ctx);
	if( rest != NULL ) {
		if( *rest != NULL ) {
			workfilep = *rest;
			rest++;
		}
		if( *rest != NULL ) {
			fprintf( stderr, "error: bad usage\n" );
			exit(2);
		}
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
	parentTwsDL->twsError( id, errorCode, errorString );
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
	int barCount, double WAP, int hasGaps )
{
	parentTwsDL->twsHistoricalData(
		reqId, date, open, high, low, close, volume,
		barCount, WAP, hasGaps);
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
	parentTwsDL->twsExecDetails(  reqId, contract, execution  );
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




TwsDL::TwsDL( const std::string& workFile ) :
	state(IDLE),
	error(0),
	lastConnectionTime(0),
	tws_time(0),
	connectivity_IB_TWS(false),
	curIdleTime(0),
	workFile(workFile),
	twsWrapper(NULL),
	twsClient(NULL),
	msgCounter(0),
	currentRequest(  *(new GenericRequest()) ),
	workTodo( new WorkTodo() ),
	packet( NULL ),
	dataFarms( *(new DataFarmStates()) ),
	pacingControl( *(new PacingGod(dataFarms)) )
{
	pacingControl.setPacingTime( tws_maxRequestsp, tws_pacingIntervalp,
		tws_minPacingTimep );
	pacingControl.setViolationPause( tws_violationPausep );
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
	if( w < tws_conTimeoutp ) {
		DEBUG_PRINTF( "Waiting %ldms before connecting again.",
			(tws_conTimeoutp - w) );
		curIdleTime = tws_conTimeoutp - w;
		return;
	}
	
	lastConnectionTime = nowInMsecs();
	changeState( WAIT_TWS_CON );
	
	/* this may callback (negative) error messages only */
	bool con = twsClient->connectTWS( tws_hostp, tws_portp, tws_client_idp );
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
	int64_t w = tws_conTimeoutp - (nowInMsecs() - lastConnectionTime);
	
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
	case GenericRequest::NONE:
		break;
	}
	
	if( reqType == GenericRequest::NONE ) {
		_lastError = "No more work to do.";
		changeState( QUIT );
	}
}


bool TwsDL::finContracts()
{
	if( !packet->finished() ) {
		DEBUG_PRINTF( "Timeout waiting for data." );
		// TODO repeat
		return false;
	}
	
	DEBUG_PRINTF( "Contracts received: %zu",
		((PacketContractDetails*)packet)->constList().size() );
	
	packet->dumpXml();
	
	return true;
}


void TwsDL::waitData()
{
	if( !packet->finished() && (currentRequest.age() <= tws_reqTimeoutp) ) {
		DEBUG_PRINTF( "still waiting for data." );
		return;
	}
	
	bool ok = false;
	switch( currentRequest.reqType() ) {
	case GenericRequest::CONTRACT_DETAILS_REQUEST:
		ok = finContracts();
		break;
	case GenericRequest::HIST_REQUEST:
		ok = finHist();
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


bool TwsDL::finHist()
{
	if( !packet->finished() ) {
		DEBUG_PRINTF( "Timeout waiting for data." );
		((PacketHistData*)packet)->closeError( PacketHistData::ERR_TIMEOUT );
	}
	
	HistTodo *histTodo = workTodo->histTodo();
	
	switch( ((PacketHistData*)packet)->getError() ) {
	case PacketHistData::ERR_NONE:
		packet->dumpXml();
	case PacketHistData::ERR_NODATA:
	case PacketHistData::ERR_NAV:
		histTodo->tellDone();
		break;
	case PacketHistData::ERR_TWSCON:
		histTodo->cancelForRepeat(0);
		break;
	case PacketHistData::ERR_TIMEOUT:
		histTodo->cancelForRepeat(1);
		break;
	case PacketHistData::ERR_REQUEST:
		histTodo->cancelForRepeat(2);
		break;
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
	( errorMsg.find(_strg_) != std::string::npos )

void TwsDL::twsError(int id, int errorCode, const std::string &errorMsg)
{
	msgCounter++;
	
	if( !twsClient->isConnected() ) {
		switch( errorCode ) {
		default:
		case 504: /* NOT_CONNECTED */
			DEBUG_PRINTF( "fatal: %d %d %s", id, errorCode, errorMsg.c_str() );
			assert(false);
			break;
		case 503: /* UPDATE_TWS */
			DEBUG_PRINTF( "error: %s", errorMsg.c_str() );
			break;
		case 502: /* CONNECT_FAIL */
			DEBUG_PRINTF( "connection failed: %s", errorMsg.c_str());
			break;
		}
		return;
	}
	
	if( id == currentRequest.reqId() ) {
		DEBUG_PRINTF( "TWS message for request %d: %d '%s'",
			id, errorCode, errorMsg.c_str() );
		if( state == WAIT_DATA ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				errorContracts( id, errorCode, errorMsg );
				break;
			case GenericRequest::HIST_REQUEST:
				errorHistData( id, errorCode, errorMsg );
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
	
	if( id != -1 ) {
		DEBUG_PRINTF( "TWS message for unexpected request %d: %d '%s'",
			id, errorCode, errorMsg.c_str() );
		return;
	}
	
	DEBUG_PRINTF( "TWS message generic: %d %s", errorCode, errorMsg.c_str() );
	
	// TODO do better
	switch( errorCode ) {
		case 1100:
			assert(ERR_MATCH("Connectivity between IB and TWS has been lost."));
			connectivity_IB_TWS = false;
			curIdleTime = tws_reqTimeoutp;
			break;
		case 1101:
			assert(ERR_MATCH("Connectivity between IB and TWS has been restored - data lost."));
			connectivity_IB_TWS = true;
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				((PacketHistData*)packet)->closeError(
					PacketHistData::ERR_TWSCON );
			}
			break;
		case 1102:
			assert(ERR_MATCH("Connectivity between IB and TWS has been restored - data maintained."));
			connectivity_IB_TWS = true;
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				((PacketHistData*)packet)->closeError(
					PacketHistData::ERR_TWSCON );
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
			dataFarms.notify( msgCounter, errorCode, errorMsg );
			pacingControl.clear();
			break;
		case 2110:
			/* looks like we get that only on fresh connection */
			assert(ERR_MATCH("Connectivity between TWS and server is broken. It will be restored automatically."));
			connectivity_IB_TWS = false;
			break;
	}
}


void TwsDL::errorContracts(int id, int errorCode, const std::string &errorMsg)
{
	// TODO
	switch( errorCode ) {
		// No security definition has been found for the request"
		case 200:
		twsContractDetailsEnd( id );
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	}
}


void TwsDL::errorHistData(int id, int errorCode, const std::string &errorMsg)
{
	const IB::Contract &curContract = workTodo->getHistTodo().current().ibContract();
	const HistRequest *cur_hR = &workTodo->getHistTodo().current();
	PacketHistData &p_histData = *((PacketHistData*)packet);
	switch( errorCode ) {
	// Historical Market Data Service error message:
	case 162:
		if( ERR_MATCH("Historical data request pacing violation") ) {
			p_histData.closeError( PacketHistData::ERR_TWSCON );
			pacingControl.notifyViolation( curContract );
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			DEBUG_PRINTF( "READY - NO DATA %p %d", cur_hR, id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_NODATA );
		} else if( ERR_MATCH("No historical market data for") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE on HMDS server. "
				"%p %d", cur_hR, id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_NAV );
		} else if( ERR_MATCH("No data of type EODChart is available") ||
			ERR_MATCH("No data of type DayChart is available") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE (no HMDS route). "
				"%p %d", cur_hR, id );
			p_histData.closeError( PacketHistData::ERR_NAV );
		} else if( ERR_MATCH("No market data permissions for") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
		} else if( ERR_MATCH("Unknown contract") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
		} else {
			DEBUG_PRINTF( "Warning, unhandled error message." );
			// seen: "TWS exited during processing of HMDS query"
		}
		break;
	// Historical Market Data Service query message:
	case 165:
		if( ERR_MATCH("HMDS server disconnect occurred.  Attempting reconnection") ||
		    ERR_MATCH("HMDS connection attempt failed.  Connection will be re-attempted") ) {
			curIdleTime = tws_reqTimeoutp;
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
			p_histData.closeError( PacketHistData::ERR_REQUEST );
		} else {
			/* using ERR_TIMEOUT instead ERR_TWSCON to push_back this request */
			p_histData.closeError( PacketHistData::ERR_TIMEOUT );
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
		p_histData.closeError( PacketHistData::ERR_REQUEST );
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
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
			case GenericRequest::ACC_STATUS_REQUEST:
			case GenericRequest::EXECUTIONS_REQUEST:
			case GenericRequest::ORDERS_REQUEST:
				assert(false); // TODO repeat
				break;
			case GenericRequest::HIST_REQUEST:
				((PacketHistData*)packet)->closeError(
					PacketHistData::ERR_TWSCON );
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


void TwsDL::twsHistoricalData( int reqId, const std::string &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps )
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
	dataFarms.learnHmds( workTodo->getHistTodo().current().ibContract() );
	
	assert( !packet->finished() );
	((PacketHistData*)packet)->append( reqId, date, open, high, low,
		close, volume, count, WAP, hasGaps );
	
	if( packet->finished() ) {
		DEBUG_PRINTF( "READY %p %d",
			&workTodo->getHistTodo().current(), reqId );
	}
}

void TwsDL::twsUpdateAccountValue( const RowAccVal& row )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketAccStatus*)packet)->append( row );
}

void TwsDL::twsUpdatePortfolio( const RowPrtfl& row )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketAccStatus*)packet)->append( row );
}

void TwsDL::twsUpdateAccountTime( const std::string& timeStamp )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketAccStatus*)packet)->appendUpdateAccountTime( timeStamp );
}

void TwsDL::twsAccountDownloadEnd( const std::string& accountName )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketAccStatus*)packet)->appendAccountDownloadEnd( accountName );
}

void TwsDL::twsExecDetails( int reqId, const IB::Contract& contract,
	const IB::Execution& execution )
{
	if( currentRequest.reqType() != GenericRequest::EXECUTIONS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketExecutions*)packet)->append( reqId, contract, execution );
}

void TwsDL::twsExecDetailsEnd( int reqId )
{
	if( currentRequest.reqType() != GenericRequest::EXECUTIONS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketExecutions*)packet)->appendExecutionsEnd( reqId );
}

void TwsDL::twsOrderStatus( const RowOrderStatus& row )
{
	if( currentRequest.reqType() != GenericRequest::ORDERS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	((PacketOrders*)packet)->append(row);
}

void TwsDL::twsOpenOrder( const RowOpenOrder& row )
{
	if( currentRequest.reqType() != GenericRequest::ORDERS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback..");
		return;
	}
	((PacketOrders*)packet)->append(row);
}

void TwsDL::twsOpenOrderEnd()
{
	/* this messages usually comes unexpected right after connecting */
	
	if( currentRequest.reqType() != GenericRequest::ORDERS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback.");
		return;
	}
	
	((PacketOrders*)packet)->appendOpenOrderEnd();
}

void TwsDL::twsCurrentTime( long time )
{
	if( state == WAIT_TWS_CON ) {
		tws_time = time;
	}
}


void TwsDL::initWork()
{
	if( get_accountp ) {
		workTodo->addSimpleRequest(GenericRequest::ACC_STATUS_REQUEST);
	}
	if( get_execp ) {
		workTodo->addSimpleRequest(GenericRequest::EXECUTIONS_REQUEST);
	}
	if( get_orderp ) {
		workTodo->addSimpleRequest(GenericRequest::ORDERS_REQUEST);
	}
	
	int cnt = workTodo->read_file(workFile);
	DEBUG_PRINTF( "got %d jobs from workFile %s", cnt, workFile.c_str() );
	
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
		workTodo->getHistTodo().dumpLeft( stderr );
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
		DEBUG_PRINTF( "TTTTTTTTTTT %d", 50 );
		curIdleTime = 50;
	} else if( state == WAIT_DATA ) {
		curIdleTime = 1000;
		DEBUG_PRINTF( "TTTTTTTTTTT %d", 1000 );
	} else {
		curIdleTime = 0;
		DEBUG_PRINTF( "TTTTTTTTTTT %d", 0 );
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
	pacingControl.addRequest( hR.ibContract() );
	
	DEBUG_PRINTF( "DOWNLOAD DATA %p %d %s",
		&workTodo->getHistTodo().current(),
		currentRequest.reqId(),
		ibToString(hR.ibContract()).c_str() );
	
	p_histData->record( currentRequest.reqId(), hR );
	twsClient->reqHistoricalData( currentRequest.reqId(),
	                              hR.ibContract(),
	                              hR.endDateTime(),
	                              hR.durationStr(),
	                              hR.barSizeSetting(),
	                              hR.whatToShow(),
	                              hR.useRTH(),
	                              hR.formatDate() );
	changeState( WAIT_DATA );
}

void TwsDL::reqAccStatus()
{
	PacketAccStatus *accStatus = new PacketAccStatus();
	packet = accStatus;
	currentRequest.nextRequest( GenericRequest::ACC_STATUS_REQUEST );
	
	accStatus->record( tws_account_namep );
	twsClient->reqAccountUpdates(true, tws_account_namep);
	changeState( WAIT_DATA );
}

void TwsDL::reqExecutions()
{
	PacketExecutions *executions = new PacketExecutions();
	packet = executions;
	currentRequest.nextRequest( GenericRequest::EXECUTIONS_REQUEST );
	
	IB::ExecutionFilter eF;
	
	executions->record( currentRequest.reqId(), eF );
	twsClient->reqExecutions( currentRequest.reqId(), eF);
	changeState( WAIT_DATA );
}

void TwsDL::reqOrders()
{
	PacketOrders *orders = new PacketOrders();
	packet = orders;
	currentRequest.nextRequest( GenericRequest::ORDERS_REQUEST );
	
	orders->record();
	twsClient->reqAllOpenOrders();
	changeState( WAIT_DATA );
}




int main(int argc, const char *argv[])
{
	twsDL_parse_cl(argc, argv);
	
	TwsXml::setSkipDefaults( !skipdefp );
	
	TwsDL twsDL( workfilep );
	int ret = twsDL.start();
	
	assert( twsDL.currentState() == TwsDL::QUIT );
	if( ret != 0 ) {
		DEBUG_PRINTF( "error: %s", twsDL.lastError().c_str() );
	} else {
		DEBUG_PRINTF( "%s", twsDL.lastError().c_str() );
	}
	return ret;
}
