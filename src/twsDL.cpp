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

#include <QtCore/QVariant>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtCore/QCoreApplication>
#include <popt.h>
#include <stdio.h>


static poptContext opt_ctx;
static const char *workfilep = "";
static const char *configfilep = "twsDL.cfg";
static int skipdefp = 0;

#define VERSION_MSG \
PACKAGE_NAME " " PACKAGE_VERSION "\n\
Copyright (C) 2010-2011 Ruediger Meier <sweet_f_a@gmx.de>\n\
License: BSD 3-Clause\n"


static void displayArgs( poptContext con, poptCallbackReason /*foo*/,
	poptOption *key, const char */*arg*/, void */*data*/ )
{
	if (key->shortName == 'h') {
		poptPrintHelp(con, stdout, 0);
	} else if (key->shortName == 'V') {
		fprintf(stdout, VERSION_MSG);
	} else {
		poptPrintUsage(con, stdout, 0);
	}
	
	exit(0);
}

static struct poptOption flow_opts[] = {
	{"verbose-xml", 'x', POPT_ARG_NONE, &skipdefp, 0,
		"Never skip xml default values.", NULL},
	{"config", 'c', POPT_ARG_STRING, &configfilep, 0,
		"Config file (default: twsDL.cfg).", "FILE"},
	POPT_TABLEEND
};

static struct poptOption help_opts[] = {
	{NULL, '\0', POPT_ARG_CALLBACK, (void*)displayArgs, 0, NULL, NULL},
	{"help", 'h', POPT_ARG_NONE, NULL, 0, "Show this help message.", NULL},
	{"version", 'V', POPT_ARG_NONE, NULL, 0, "Print version string and exit.",
		NULL},
	{"usage", '\0', POPT_ARG_NONE, NULL, 0, "Display brief usage message."
		, NULL},
	POPT_TABLEEND
};

static const struct poptOption twsDL_opts[] = {
	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, flow_opts, 0,
	 "Program advice:", NULL},
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
		Q_ASSERT(false);
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
	parentTwsDL->twsError(
		id, errorCode, toQString(errorString) );
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
		reqId, toQString(date), open, high, low, close, volume,
		barCount, WAP, hasGaps);
}




TwsDL::TwsDL( const QString& confFile, const QString& workFile ) :
	state(CONNECT),
	lastConnectionTime(0),
	connection_failed( false ),
	curIdleTime(0),
	confFile(confFile),
	workFile(workFile),
	myProp(NULL),
	twsWrapper(NULL),
	twsClient(NULL),
	msgCounter(0),
	currentRequest(  *(new GenericRequest()) ),
	curIndexTodoContractDetails(0),
	workTodo( new WorkTodo() ),
	contractDetailsTodo( new ContractDetailsTodo() ),
	histTodo( new HistTodo() ),
	p_contractDetails( *(new PacketContractDetails()) ),
	p_histData( *(new PacketHistData()) ),
	dataFarms( *(new DataFarmStates()) ),
	pacingControl( *(new PacingGod(dataFarms)) )
{
	initProperties();
	pacingControl.setPacingTime( myProp->maxRequests,
		myProp->pacingInterval, myProp->minPacingTime );
	pacingControl.setViolationPause( myProp->violationPause );
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
	if( myProp  != NULL ) {
		delete myProp;
	}
	if( workTodo != NULL ) {
		delete workTodo;
	}
	if( histTodo != NULL ) {
		delete histTodo;
	}
	if( contractDetailsTodo != NULL ) {
		delete contractDetailsTodo;
	}
	delete &p_contractDetails;
	delete &p_histData;
	delete &pacingControl;
	delete &dataFarms;
}


void TwsDL::start()
{
	Q_ASSERT( (state == CONNECT) ||
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
	qint64 w = nowInMsecs() - lastConnectionTime;
	if( w < myProp->conTimeout ) {
		qDebug() << "Waiting" << (myProp->conTimeout - w)
			<< "ms before connecting again.";
		curIdleTime = myProp->conTimeout - w;
		return;
	}
	
	connection_failed = false;
	lastConnectionTime = nowInMsecs();
	changeState( WAIT_TWS_CON );
	
	twsClient->connectTWS(
		myProp->twsHost, myProp->twsPort, myProp->clientId );
	
	if( !twsClient->isConnected() ) {
		qDebug() << "Connection to TWS failed:"; //TODO print a specific error
		twsConnected( false );
	} else {
		//TODO print client/server version and m_TwsTime
		twsConnected( true );
	}
}


void TwsDL::waitTwsCon()
{
	if( twsClient->isConnected() ) {
		qDebug() << "We are connected to TWS.";
		changeState( IDLE );
	} else {
		if( connection_failed ) {
			qDebug() << "Connecting TWS failed.";
			changeState( CONNECT );
		} else if( (nowInMsecs() - lastConnectionTime) > myProp->conTimeout ) {
				qDebug() << "Timeout connecting TWS.";
				twsClient->disconnectTWS();
				curIdleTime = 1000;
		} else {
			qDebug() << "Still waiting for tws connection.";
		}
	}
}


void TwsDL::idle()
{
	Q_ASSERT(currentRequest.reqType() == GenericRequest::NONE);
	
	if( !twsClient->isConnected() ) {
		changeState( CONNECT );
		return;
	}
	
	if( workTodo->getType() == GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		if( curIndexTodoContractDetails < contractDetailsTodo->contractDetailsRequests.size() ) {
			currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );
			getContracts();
			return;
		}
	}
	
	// TODO we want to dump only one time
	dumpWorkTodo();
	
	if( workTodo->getType() == GenericRequest::HIST_REQUEST ) {
		if( histTodo->countLeft() > 0
	    	&& ( myProp->reqMaxContracts <= 0 || histTodo->countDone() <= myProp->reqMaxContracts ) ) {
			getData();
			return;
		}
	}
	
	changeState( QUIT_READY );
}


void TwsDL::getContracts()
{
	const ContractDetailsRequest &cdR =
		contractDetailsTodo->contractDetailsRequests.at( curIndexTodoContractDetails );
	reqContractDetails( cdR );
	
	changeState( WAIT_DATA );
}


void TwsDL::finContracts()
{
	qDebug() << QString( "Contracts received: %1")
		.arg(p_contractDetails.constList().size() );
	
	p_contractDetails.dumpXml();
	
	for( int i = 0; i<p_contractDetails.constList().size(); i++ ) {
		const IB::ContractDetails &cd = p_contractDetails.constList().at(i);
		IB::Contract c = cd.summary;
		c.includeExpired = myProp->includeExpired;
		
		if( myProp->reqMaxContractsPerSpec > 0 && myProp->reqMaxContractsPerSpec <= i ) {
			break;
		}
		
		foreach( QString wts, myProp->whatToShow ) {
			HistRequest hR;
			hR.initialize( c, myProp->endDateTime, myProp->durationStr,
			               myProp->barSizeSetting, wts, myProp->useRTH, myProp->formatDate );
			histTodo->add( hR );
		}
	}
	
	p_contractDetails.clear();
	
	curIndexTodoContractDetails++;
	currentRequest.close();
	changeState( IDLE );
}


void TwsDL::getData()
{
	Q_ASSERT( histTodo->countLeft() > 0 );
	
	int wait = histTodo->checkoutOpt( &pacingControl, &dataFarms );
	
	if( wait > 0 ) {
		curIdleTime = qMin( 1000, wait );
		return;
	}
	if( wait < -1 ) {
		// just debug timer resolution
		qDebug() << "late timeout:" << wait;
	}
	
	const HistRequest &hR = histTodo->current();
	
	pacingControl.addRequest( hR.ibContract() );
	
	currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
	
	qDebug() << "DOWNLOAD DATA" << histTodo->currentIndex()
		<< currentRequest.reqId() << ibToString(hR.ibContract());
	
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
		Q_ASSERT( false );
		break;
	}
}


void TwsDL::waitContracts()
{
	if( p_contractDetails.isFinished() ) {
		finContracts();
	} else if( currentRequest.age() > myProp->reqTimeout ) {
		qDebug() << "Timeout waiting for data.";
		// TODO repeat
		changeState( QUIT_ERROR );
	} else {
		qDebug() << "still waiting for data.";
	}
}


void TwsDL::waitHist()
{
	if( p_histData.isFinished() ) {
		finData();
	} else if( currentRequest.age() > myProp->reqTimeout ) {
		qDebug() << "Timeout waiting for data.";
		p_histData.closeError( PacketHistData::ERR_TIMEOUT );
		finData();
	} else {
		qDebug() << "still waiting for data.";
	}
}


void TwsDL::finData()
{
	Q_ASSERT( p_histData.isFinished() );
	
	switch( p_histData.getError() ) {
	case PacketHistData::ERR_NONE:
		p_histData.dump( histTodo->current(), myProp->printFormatDates );
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


void TwsDL::initProperties()
{
	Properties prop;
	prop.readConfigFile(confFile);
	
	myProp = new PropTwsDL(prop);
	myProp->readProperties();
}


void TwsDL::initTwsClient()
{
	Q_ASSERT( twsClient == NULL && twsWrapper == NULL );
	
	twsWrapper = new TwsDlWrapper(this);
	twsClient = new TWSClient( twsWrapper );
}


#define ERR_MATCH( _strg_  ) \
	errorMsg.contains( QString(_strg_), Qt::CaseInsensitive )

void TwsDL::twsError(int id, int errorCode, const QString &errorMsg)
{
	msgCounter++;
	
	if( id == currentRequest.reqId() ) {
		qDebug() << "ERROR for request" << id << errorCode <<errorMsg;
		if( state == WAIT_DATA ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				errorContracts( id, errorCode, errorMsg );
				break;
			case GenericRequest::HIST_REQUEST:
				errorHistData( id, errorCode, errorMsg );
				break;
			case GenericRequest::NONE:
				Q_ASSERT( false );
				break;
			}
		} else {
			// TODO
		}
		return;
	}
	
	if( id != -1 ) {
		qDebug() << "Warning, unexpected request Id";
		return;
	}
	
	// TODO do better
	switch( errorCode ) {
		case 1100:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been lost."));
			curIdleTime = myProp->reqTimeout;
			break;
		case 1101:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been restored - data lost."));
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( PacketHistData::ERR_TWSCON );
				curIdleTime = 0;
			}
			break;
		case 1102:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been restored - data maintained."));
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( PacketHistData::ERR_TWSCON );
				curIdleTime = 0;
			}
			break;
		case 1300:
			Q_ASSERT(ERR_MATCH("TWS socket port has been reset and this connection is being dropped."));
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


void TwsDL::errorContracts(int id, int errorCode, const QString &errorMsg)
{
	// TODO
	switch( errorCode ) {
		// No security definition has been found for the request"
		case 200:
		twsContractDetailsEnd( id );
		break;
	default:
		qDebug() << "Warning, unhandled error code.";
		break;
	}
}


void TwsDL::errorHistData(int id, int errorCode, const QString &errorMsg)
{
	switch( errorCode ) {
	// Historical Market Data Service error message:
	case 162:
		if( ERR_MATCH("Historical data request pacing violation") ) {
			p_histData.closeError( PacketHistData::ERR_TWSCON );
			pacingControl.notifyViolation(
				histTodo->current().ibContract() );
			curIdleTime = 0;
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			qDebug() << "READY - NO DATA" << histTodo->currentIndex() << id;
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_NODATA );
			curIdleTime = 0;
		} else if( ERR_MATCH("No historical market data for") ) {
			// NOTE we should skip all similar work intelligently
			qDebug() << "WARNING - DATA IS NOT AVAILABLE on HMDS server."
				<< histTodo->currentIndex() << id;
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_NAV );
			curIdleTime = 0;
		} else if( ERR_MATCH("No data of type EODChart is available") ||
			ERR_MATCH("No data of type DayChart is available") ) {
			// NOTE we should skip all similar work intelligently
			qDebug() << "WARNING - DATA IS NOT AVAILABLE (no HMDS route)."
				<< histTodo->currentIndex() << id;
			p_histData.closeError( PacketHistData::ERR_NAV );
			curIdleTime = 0;
		} else if( ERR_MATCH("No market data permissions for") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
			curIdleTime = 0;
		} else if( ERR_MATCH("Unknown contract") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
			curIdleTime = 0;
		} else {
			qDebug() << "Warning, unhandled error message.";
			// seen: "TWS exited during processing of HMDS query"
		}
		break;
	// Historical Market Data Service query message:
	case 165:
		if( ERR_MATCH("HMDS server disconnect occurred.  Attempting reconnection") ||
		    ERR_MATCH("HMDS connection attempt failed.  Connection will be re-attempted") ) {
			curIdleTime = myProp->reqTimeout;
		} else if( ERR_MATCH("HMDS server connection was successful") ) {
			dataFarms.learnHmdsLastOk( msgCounter, histTodo->current().ibContract() );
		} else {
			qDebug() << "Warning, unhandled error message.";
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
		qDebug() << "Warning, unexpected error code.";
		break;
	// The security <security> is not available or allowed for this account
	case 203:
		qDebug() << "Warning, unhandled error code.";
		break;
	// Server error when validating an API client request
	case 321:
		// comes directly from TWS whith prefix "Error validating request:-"
		// NOTE we could find out more to throw away similar worktodo
		p_histData.closeError( PacketHistData::ERR_REQUEST );
		curIdleTime = 0;
		break;
	default:
		qDebug() << "Warning, unhandled error code.";
		break;
	}
}

#undef ERR_MATCH


void TwsDL::twsConnected( bool connected )
{
	if( connected ) {
		Q_ASSERT( state == WAIT_TWS_CON );
		curIdleTime = 1000; //TODO wait for first tws messages
	} else {
		qDebug() << "disconnected in state" << state;
		Q_ASSERT( state != CONNECT );
		
		if( state == WAIT_TWS_CON ) {
			connection_failed = true;
		} else if( state == WAIT_DATA ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				if( !p_contractDetails.isFinished() ) {
					Q_ASSERT(false); // TODO repeat
				}
				break;
			case GenericRequest::HIST_REQUEST:
				if( !p_histData.isFinished() ) {
					p_histData.closeError( PacketHistData::ERR_TWSCON );
				}
				break;
			case GenericRequest::NONE:
				Q_ASSERT(false);
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
		qDebug() << "got reqId" << reqId << "but currentReqId:"
			<< currentRequest.reqId();
		Q_ASSERT( false );
	}
	
	p_contractDetails.append(reqId, ibContractDetails);
}


void TwsDL::twsBondContractDetails( int reqId, const IB::ContractDetails &ibContractDetails )
{
	if( currentRequest.reqId() != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:"
			<< currentRequest.reqId();
		Q_ASSERT( false );
	}
	
	p_contractDetails.append(reqId, ibContractDetails);
}


void TwsDL::twsContractDetailsEnd( int reqId )
{
	if( currentRequest.reqId() != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:"
			<< currentRequest.reqId();
		Q_ASSERT( false );
	}
	
	curIdleTime = 0;
	p_contractDetails.setFinished();
}


void TwsDL::twsHistoricalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps )
{
	if( currentRequest.reqId() != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:"
			<< currentRequest.reqId();
		return;
	}
	
	// TODO we shouldn't do this each row
	dataFarms.learnHmds( histTodo->current().ibContract() );
	
	Q_ASSERT( !p_histData.isFinished() );
	p_histData.append( reqId, date, open, high, low,
		close, volume, count, WAP, hasGaps );
	
	if( p_histData.isFinished() ) {
		curIdleTime = 0;
		qDebug() << "READY" << histTodo->currentIndex() << reqId;
	}
}


void TwsDL::initWork()
{
	int cnt = workTodo->read_file(workFile);
	const QList<QByteArray> &rows = workTodo->getRows();
	qDebug() << QString("got %1, %2 rows from workFile %3")
		.arg(cnt).arg(rows.size()).arg(workFile);
	
	if( workTodo->getType() == GenericRequest::CONTRACT_DETAILS_REQUEST ) {
		qDebug() << "getting contracts from TWS";
		int i = contractDetailsTodo->fromFile( rows, myProp->includeExpired );
		Q_ASSERT( i>=0 );
// 		state = IDLE;
	} else if( workTodo->getType() == GenericRequest::HIST_REQUEST ) {
		qDebug() << "read work from file";
		int i = histTodo->fromFile(rows, myProp->includeExpired);
		Q_ASSERT( i>=0 );
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
		histTodo->dumpLeft( stderr );
	}
}


TwsDL::State TwsDL::currentState() const
{
	return state;
}


void TwsDL::changeState( State s )
{
	Q_ASSERT( state != s );
	state = s;
	
	if( state == WAIT_TWS_CON ) {
		qDebug() << "TTTTTTTTTTT" << 50;
		curIdleTime = 50;
	} else if( state == WAIT_DATA ) {
		curIdleTime = 1000;
		qDebug() << "TTTTTTTTTTT" << 1000;
	} else {
		curIdleTime = 0;
		qDebug() << "TTTTTTTTTTT" << 0;
	}
}


void TwsDL::reqContractDetails( const ContractDetailsRequest& cdR )
{
	p_contractDetails.record( currentRequest.reqId(), cdR );
	twsClient->reqContractDetails( currentRequest.reqId(), cdR.ibContract() );
}


void TwsDL::reqHistoricalData( const HistRequest& hR )
{
	p_histData.record( currentRequest.reqId() );
	twsClient->reqHistoricalData( currentRequest.reqId(),
	                              hR.ibContract(),
	                              hR.endDateTime(),
	                              hR.durationStr(),
	                              hR.barSizeSetting(),
	                              hR.whatToShow(),
	                              hR.useRTH(),
	                              hR.formatDate() );
}


///////////////////////////////////////////////////////////////////////////////
// PropTwsDL
PropTwsDL::PropTwsDL( const Properties& prop, const QString& cName ) :
	PropSub(prop,cName)
{
	PROP_DEBUG( 2, "INITIALIZING" );
	//initializing fields
	initDefaults();
}


void PropTwsDL::initDefaults()
{
	twsHost  = "localhost";
	twsPort  = 6666;
	clientId = 66;
	
	conTimeout = 1000;
	reqTimeout = 20000;
	maxRequests = 60;
	pacingInterval = 601000;
	minPacingTime = 1500;
	violationPause = 60000;
	
	reqMaxContracts = -1;
	reqMaxContractsPerSpec = -1;
	
	printFormatDates = true;
	
	includeExpired = false;
	
	endDateTime = "20100514 22:15:00 GMT";
	durationStr = "1 W";
	barSizeSetting = "1 hour";
	whatToShow = QList<QString>() << "TRADES";
	useRTH = 1;
	formatDate = 1;
}


bool PropTwsDL::readProperties()
{
	PROP_DEBUG( 2, "READ CONFIG" );
	bool ok = true;
	
	ok = ok & get("twsHost",       twsHost);
	ok = ok & get("twsPort",       twsPort);
	ok = ok & get("clientId",      clientId);
	
	ok &= get("conTimeout", conTimeout);
	ok &= get("reqTimeout", reqTimeout);
	ok &= get("maxRequests", maxRequests );
	ok &= get("pacingInterval", pacingInterval );
	ok &= get("minPacingTime", minPacingTime);
	ok &= get("violationPause", violationPause);
	
	ok = ok & get("reqMaxContracts", reqMaxContracts);
	ok = ok & get("reqMaxContractsPerSpec", reqMaxContractsPerSpec);
	
	ok = ok & get("printFormatDates", printFormatDates);
	
	ok = ok & get("includeExpired", includeExpired);
	
	ok = ok & get("endDateTime", endDateTime);
	ok = ok & get("durationStr", durationStr);
	ok = ok & get("barSizeSetting", barSizeSetting);
	
	QString wtsStr;
	ok = ok & get("whatToShow", wtsStr);
	whatToShow = wtsStr.trimmed().split(QRegExp("[ \t\r\n]*,[ \t\r\n]*"));
	
	ok = ok & get("useRTH", useRTH);
	ok = ok & get("formatDate", formatDate);
	
	return ok;
}
///////////////////////////////////////////////////////////////////////////////




int main(int argc, char *argv[])
{
	QCoreApplication app( argc, argv );
	
	twsDL_parse_cl(argc, (const char **) argv);
	
	IbXml::setSkipDefaults( !skipdefp );
	
	TwsDL twsDL( configfilep, workfilep );
	twsDL.start();
	
	TwsDL::State state = twsDL.currentState();
	Q_ASSERT( (state == TwsDL::QUIT_READY) ||
		(state == TwsDL::QUIT_ERROR) );
	if( state == TwsDL::QUIT_READY ) {
		return 0;
	} else {
		qDebug() << "Finished with errors.";
		return 1;
	}
}
