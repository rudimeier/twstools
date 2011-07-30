#include "twsDL.h"
#include "tws_meta.h"

#include "twsUtil.h"
#include "twsClient.h"
#include "twsWrapper.h"
#include "tws_xml.h"
#include "debug.h"
#include "config.h"

// from global installed ibtws
#include "ibtws/Contract.h"

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
		
// 		void twsConnected( bool connected );
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
	parentTwsDL->twsConnected( false );
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




TwsDL::TwsDL( const std::string& workFile ) :
	state(CONNECT),
	lastConnectionTime(0),
	connection_failed( false ),
	curIdleTime(0),
	workFile(workFile),
	twsWrapper(NULL),
	twsClient(NULL),
	msgCounter(0),
	currentRequest(  *(new GenericRequest()) ),
	curIndexTodoContractDetails(0),
	workTodo( new WorkTodo() ),
	p_contractDetails( *(new PacketContractDetails()) ),
	p_histData( *(new PacketHistData()) ),
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
	delete &p_contractDetails;
	delete &p_histData;
	delete &pacingControl;
	delete &dataFarms;
}


void TwsDL::start()
{
	assert( (state == CONNECT) ||
		(state == QUIT_READY) || (state == QUIT_ERROR) );
	
	state = CONNECT;
	curIdleTime = 0;
	eventLoop();
}


void TwsDL::eventLoop()
{
	bool run = true;
	while( run ) {
		if( curIdleTime > 0 ) {
			twsClient->selectStuff( curIdleTime );
		}
		switch( state ) {
			case CONNECT:
				connectTws();
				break;
			case WAIT_TWS_CON:
				waitTwsCon();
				break;
			case IDLE:
				idle();
				break;
			case WAIT_DATA:
				waitData();
				break;
			case QUIT_READY:
				onQuit(0);
				run = false;
				break;
			case QUIT_ERROR:
				onQuit(-1);
				run = false;
				break;
		}
	}
}


void TwsDL::connectTws()
{
	int64_t w = nowInMsecs() - lastConnectionTime;
	if( w < tws_conTimeoutp ) {
		DEBUG_PRINTF( "Waiting %ldms before connecting again.",
			(tws_conTimeoutp - w) );
		curIdleTime = tws_conTimeoutp - w;
		return;
	}
	
	connection_failed = false;
	lastConnectionTime = nowInMsecs();
	changeState( WAIT_TWS_CON );
	
	twsClient->connectTWS( tws_hostp, tws_portp, tws_client_idp );
	
	if( !twsClient->isConnected() ) {
		DEBUG_PRINTF("Connection to TWS failed:"); //TODO print a specific error
		twsConnected( false );
	} else {
		//TODO print client/server version and m_TwsTime
		twsConnected( true );
	}
}


void TwsDL::waitTwsCon()
{
	if( twsClient->isConnected() ) {
		DEBUG_PRINTF( "We are connected to TWS." );
		changeState( IDLE );
	} else {
		if( connection_failed ) {
			DEBUG_PRINTF( "Connecting TWS failed." );
			changeState( CONNECT );
		} else if( (nowInMsecs() - lastConnectionTime) > tws_conTimeoutp ) {
				DEBUG_PRINTF( "Timeout connecting TWS." );
				twsClient->disconnectTWS();
				curIdleTime = 1000;
		} else {
			DEBUG_PRINTF( "Still waiting for tws connection." );
		}
	}
}


void TwsDL::idle()
{
	assert(currentRequest.reqType() == GenericRequest::NONE);
	
	if( !twsClient->isConnected() ) {
		changeState( CONNECT );
		return;
	}
	
	if( workTodo->getType() == GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		if( curIndexTodoContractDetails < (int)workTodo->getContractDetailsTodo().contractDetailsRequests.size() ) {
			currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );
			getContracts();
			return;
		}
	}
	
	// TODO we want to dump only one time
	dumpWorkTodo();
	
	if( workTodo->getType() == GenericRequest::HIST_REQUEST ) {
		if(  workTodo->getHistTodo().countLeft() > 0 ) {
			getData();
			return;
		}
	}
	
	changeState( QUIT_READY );
}


void TwsDL::getContracts()
{
	const ContractDetailsRequest &cdR =
		workTodo->getContractDetailsTodo().contractDetailsRequests.at( curIndexTodoContractDetails );
	reqContractDetails( cdR );
	
	changeState( WAIT_DATA );
}


void TwsDL::finContracts()
{
	DEBUG_PRINTF( "Contracts received: %zu",
		p_contractDetails.constList().size() );
	
	p_contractDetails.dumpXml();
	p_contractDetails.clear();
	
	curIndexTodoContractDetails++;
	currentRequest.close();
	changeState( IDLE );
}


void TwsDL::getData()
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
	
	const HistRequest &hR = workTodo->getHistTodo().current();
	
	pacingControl.addRequest( hR.ibContract() );
	
	currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
	
	DEBUG_PRINTF( "DOWNLOAD DATA %p %d %s",
		&workTodo->getHistTodo().current(),
		currentRequest.reqId(),
		ibToString(hR.ibContract()).c_str() );
	
	reqHistoricalData( hR );
	
	changeState( WAIT_DATA );
}


void TwsDL::waitData()
{
	switch( currentRequest.reqType() ) {
	case GenericRequest::CONTRACT_DETAILS_REQUEST:
		waitContracts();
		break;
	case GenericRequest::HIST_REQUEST:
		waitHist();
		break;
	case GenericRequest::NONE:
		assert( false );
		break;
	}
}


void TwsDL::waitContracts()
{
	if( p_contractDetails.isFinished() ) {
		finContracts();
	} else if( currentRequest.age() > tws_reqTimeoutp ) {
		DEBUG_PRINTF( "Timeout waiting for data." );
		// TODO repeat
		changeState( QUIT_ERROR );
	} else {
		DEBUG_PRINTF( "still waiting for data." );
	}
}


void TwsDL::waitHist()
{
	if( p_histData.finished() ) {
		finData();
	} else if( currentRequest.age() > tws_reqTimeoutp ) {
		DEBUG_PRINTF( "Timeout waiting for data." );
		p_histData.closeError( PacketHistData::ERR_TIMEOUT );
		finData();
	} else {
		DEBUG_PRINTF( "still waiting for data." );
	}
}


void TwsDL::finData()
{
	assert( p_histData.finished() );
	HistTodo *histTodo = workTodo->histTodo();
	
	switch( p_histData.getError() ) {
	case PacketHistData::ERR_NONE:
		p_histData.dumpXml();
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
	
	p_histData.clear();
	currentRequest.close();
	changeState( IDLE );
}


void TwsDL::onQuit( int /*ret*/ )
{
	curIdleTime = 0;
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
	
	if( id == currentRequest.reqId() ) {
		DEBUG_PRINTF( "ERROR for request %d %d %s",
			id, errorCode, errorMsg.c_str() );
		if( state == WAIT_DATA ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				errorContracts( id, errorCode, errorMsg );
				break;
			case GenericRequest::HIST_REQUEST:
				errorHistData( id, errorCode, errorMsg );
				break;
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
		DEBUG_PRINTF( "Warning, unexpected request Id %d", id );
		return;
	}
	
	// TODO do better
	switch( errorCode ) {
		case 1100:
			assert(ERR_MATCH("Connectivity between IB and TWS has been lost."));
			curIdleTime = tws_reqTimeoutp;
			break;
		case 1101:
			assert(ERR_MATCH("Connectivity between IB and TWS has been restored - data lost."));
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( PacketHistData::ERR_TWSCON );
				curIdleTime = 0;
			}
			break;
		case 1102:
			assert(ERR_MATCH("Connectivity between IB and TWS has been restored - data maintained."));
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( PacketHistData::ERR_TWSCON );
				curIdleTime = 0;
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
	switch( errorCode ) {
	// Historical Market Data Service error message:
	case 162:
		if( ERR_MATCH("Historical data request pacing violation") ) {
			p_histData.closeError( PacketHistData::ERR_TWSCON );
			pacingControl.notifyViolation( curContract );
			curIdleTime = 0;
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			DEBUG_PRINTF( "READY - NO DATA %p %d", cur_hR, id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_NODATA );
			curIdleTime = 0;
		} else if( ERR_MATCH("No historical market data for") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE on HMDS server. "
				"%p %d", cur_hR, id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_NAV );
			curIdleTime = 0;
		} else if( ERR_MATCH("No data of type EODChart is available") ||
			ERR_MATCH("No data of type DayChart is available") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE (no HMDS route). "
				"%p %d", cur_hR, id );
			p_histData.closeError( PacketHistData::ERR_NAV );
			curIdleTime = 0;
		} else if( ERR_MATCH("No market data permissions for") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
			curIdleTime = 0;
		} else if( ERR_MATCH("Unknown contract") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
			curIdleTime = 0;
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
		p_histData.closeError( PacketHistData::ERR_REQUEST );
		curIdleTime = 0;
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
		curIdleTime = 0;
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	}
}

#undef ERR_MATCH


void TwsDL::twsConnected( bool connected )
{
	if( connected ) {
		assert( state == WAIT_TWS_CON );
		curIdleTime = 1000; //TODO wait for first tws messages
	} else {
		DEBUG_PRINTF( "disconnected in state %d", state );
		assert( state != CONNECT );
		
		if( state == WAIT_TWS_CON ) {
			connection_failed = true;
		} else if( state == WAIT_DATA ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				if( !p_contractDetails.isFinished() ) {
					assert(false); // TODO repeat
				}
				break;
			case GenericRequest::HIST_REQUEST:
				if( !p_histData.finished() ) {
					p_histData.closeError( PacketHistData::ERR_TWSCON );
				}
				break;
			case GenericRequest::NONE:
				assert(false);
				break;
			}
		}
		dataFarms.setAllBroken();
		pacingControl.clear();
		curIdleTime = 0;
	}
}


void TwsDL::twsContractDetails( int reqId, const IB::ContractDetails &ibContractDetails )
{
	
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		assert( false );
	}
	
	p_contractDetails.append(reqId, ibContractDetails);
}


void TwsDL::twsBondContractDetails( int reqId, const IB::ContractDetails &ibContractDetails )
{
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		assert( false );
	}
	
	p_contractDetails.append(reqId, ibContractDetails);
}


void TwsDL::twsContractDetailsEnd( int reqId )
{
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		assert( false );
	}
	
	curIdleTime = 0;
	p_contractDetails.setFinished();
}


void TwsDL::twsHistoricalData( int reqId, const std::string &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps )
{
	if( currentRequest.reqId() != reqId ) {
		DEBUG_PRINTF( "got reqId %d but currentReqId: %d",
			reqId, currentRequest.reqId() );
		return;
	}
	
	// TODO we shouldn't do this each row
	dataFarms.learnHmds( workTodo->getHistTodo().current().ibContract() );
	
	assert( !p_histData.finished() );
	p_histData.append( reqId, date, open, high, low,
		close, volume, count, WAP, hasGaps );
	
	if( p_histData.finished() ) {
		curIdleTime = 0;
		DEBUG_PRINTF( "READY %p %d",
			&workTodo->getHistTodo().current(), reqId );
	}
}


void TwsDL::initWork()
{
	int cnt = workTodo->read_file(workFile);
	DEBUG_PRINTF( "got %d jobs from workFile %s", cnt, workFile.c_str() );
	
	if( workTodo->getType() == GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		DEBUG_PRINTF( "getting contracts from TWS, %zu",
			workTodo->getContractDetailsTodo().contractDetailsRequests.size() );
// 		state = IDLE;
	} else if( workTodo->getType() == GenericRequest::HIST_REQUEST ) {
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


void TwsDL::reqContractDetails( const ContractDetailsRequest& cdR )
{
	p_contractDetails.record( currentRequest.reqId(), cdR );
	twsClient->reqContractDetails( currentRequest.reqId(), cdR.ibContract() );
}


void TwsDL::reqHistoricalData( const HistRequest& hR )
{
	p_histData.record( currentRequest.reqId(), hR );
	twsClient->reqHistoricalData( currentRequest.reqId(),
	                              hR.ibContract(),
	                              hR.endDateTime(),
	                              hR.durationStr(),
	                              hR.barSizeSetting(),
	                              hR.whatToShow(),
	                              hR.useRTH(),
	                              hR.formatDate() );
}




int main(int argc, const char *argv[])
{
	twsDL_parse_cl(argc, argv);
	
	TwsXml::setSkipDefaults( !skipdefp );
	
	TwsDL twsDL( workfilep );
	twsDL.start();
	
	TwsDL::State state = twsDL.currentState();
	assert( (state == TwsDL::QUIT_READY) ||
		(state == TwsDL::QUIT_ERROR) );
	if( state == TwsDL::QUIT_READY ) {
		return 0;
	} else {
		DEBUG_PRINTF( "Finished with errors." );
		return 1;
	}
}
