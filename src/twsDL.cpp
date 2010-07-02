#include "twsDL.h"
#include "tws_meta.h"

#include "twsapi/twsUtil.h"
#include "twsapi/twsClient.h"
#include "twsapi/twsWrapper.h"
#include "utilities/debug.h"

// from global installed ibtws
#include "Contract.h"

#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QFile>




namespace Test {




TwsDL::TwsDL( const QString& confFile, const QString& workFile ) :
	state(CONNECT),
	lastConnectionTime(0),
	connection_failed( false ),
	confFile(confFile),
	workFile(workFile),
	myProp(NULL),
	twsClient(NULL),
	twsWrapper(NULL),
	msgCounter(0),
	currentRequest(  *(new GenericRequest()) ),
	curIndexTodoContractDetails(0),
	contractDetailsTodo( new ContractDetailsTodo() ),
	histTodo( new HistTodo() ),
	p_contractDetails( *(new PacketContractDetails()) ),
	p_histData( *(new PacketHistData()) ),
	dataFarms( *(new DataFarmStates()) ),
	pacingControl( *(new PacingGod(dataFarms)) ),
	idleTimer(NULL)
{
	initProperties();
	pacingControl.setPacingTime( myProp->maxRequests,
		myProp->pacingInterval, myProp->minPacingTime );
	pacingControl.setViolationPause( myProp->violationPause );
	initTwsClient();
	initIdleTimer();
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
	if( idleTimer != NULL ) {
		delete idleTimer;
	}
}


void TwsDL::start()
{
	Q_ASSERT( !idleTimer->isActive() );
	Q_ASSERT( (state == CONNECT) ||
		(state == QUIT_READY) || (state == QUIT_ERROR) );
	
	state = CONNECT;
	idleTimer->setInterval(0);
	idleTimer->start();
}


void TwsDL::idleTimeout()
{
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
			break;
		case QUIT_ERROR:
				onQuit(-1);
			break;
	}
}


void TwsDL::connectTws()
{
	qint64 w = nowInMsecs() - lastConnectionTime;
	if( w < myProp->conTimeout ) {
		qDebug() << "Waiting" << (myProp->conTimeout - w)
			<< "ms before connecting again.";
		idleTimer->setInterval( myProp->conTimeout - w );
		return;
	}
	
	connection_failed = false;
	lastConnectionTime = nowInMsecs();
	changeState( WAIT_TWS_CON );
	
	twsClient->connectTWS(
		myProp->twsHost, myProp->twsPort, myProp->clientId );
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
				idleTimer->setInterval( 1000 );
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
	
	if( curIndexTodoContractDetails < contractDetailsTodo->contractDetailsRequests.size() ) {
		currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );
		getContracts();
		return;
	}
	
	// TODO we want to dump only one time
	dumpWorkTodo();
	
	if( myProp->downloadData  && histTodo->countLeft() > 0
	    && ( myProp->reqMaxContracts <= 0 || histTodo->countDone() <= myProp->reqMaxContracts ) ) {
		getData();
	} else {
		changeState( QUIT_READY );
	}
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
	int inserted;
	inserted = storage2stdout();
	
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
	if( inserted == -1 ) {
		changeState( QUIT_ERROR );
		return;
	}
	
	curIndexTodoContractDetails++;
	currentRequest.close();
	changeState( IDLE );
}


void TwsDL::getData()
{
	Q_ASSERT( histTodo->countLeft() > 0 );
	
	int wait = histTodo->checkoutOpt( &pacingControl, &dataFarms );
	
	if( wait > 0 ) {
		idleTimer->setInterval( qMin( 1000, wait ) );
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
	idleTimer->stop();
	emit finished();
}


void TwsDL::initProperties()
{
	Properties prop;
	prop.readConfigFile(confFile);
	
	myProp = new PropTwsDL(prop);
	myProp->readProperties();
}


void TwsDL::initIdleTimer()
{
	Q_ASSERT( idleTimer == NULL );
	
	idleTimer = new QTimer();
	idleTimer->setSingleShot(false);
	connect( idleTimer, SIGNAL(timeout()), this, SLOT(idleTimeout()) );
}


void TwsDL::initTwsClient()
{
	Q_ASSERT( (twsClient == NULL) && (twsWrapper == NULL) );
	
	twsClient = new TWSClient();
	twsWrapper = new TWSWrapper();
	
	// connecting all TWS signals to twsWrapper
	TWSWrapper::connectAllSignals(twsClient, twsWrapper);
	TWSWrapper::disconnectContractDetails(twsClient, twsWrapper);
	TWSWrapper::disconnectContractDetailsEnd(twsClient, twsWrapper);
	TWSWrapper::disconnectHistoricalData(twsClient, twsWrapper);
	
	// connecting some TWS signals to this
	connect(twsClient, SIGNAL(error(int, int, const QString &)),
		this, SLOT(twsError(int, int, const QString &)), Qt::QueuedConnection );
	
	connect ( twsClient, SIGNAL(connected(bool)),
		 this,SLOT(twsConnected(bool)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(contractDetails(int, IB::ContractDetails)),
		 this,SLOT(twsContractDetails(int, IB::ContractDetails)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(contractDetailsEnd(int)),
		 this,SLOT(twsContractDetailsEnd(int)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(historicalData(int, const QString&, double, double, double,
			double, int, int, double, bool )),
		 this,SLOT(twsHistoricalData(int, const QString&, double, double, double,
			double, int, int, double, bool )), Qt::QueuedConnection );
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
			idleTimer->setInterval( myProp->reqTimeout );
			break;
		case 1101:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been restored - data lost."));
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( PacketHistData::ERR_TWSCON );
				idleTimer->setInterval( 0 );
			}
			break;
		case 1102:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been restored - data maintained."));
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( PacketHistData::ERR_TWSCON );
				idleTimer->setInterval( 0 );
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
			idleTimer->setInterval( 0 );
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			qDebug() << "READY - NO DATA" << histTodo->currentIndex() << id;
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_NODATA );
			idleTimer->setInterval( 0 );
		} else if( ERR_MATCH("No historical market data for") ) {
			// NOTE we should skip all similar work intelligently
			qDebug() << "WARNING - DATA IS NOT AVAILABLE on HMDS server."
				<< histTodo->currentIndex() << id;
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_NAV );
			idleTimer->setInterval( 0 );
		} else if( ERR_MATCH("No data of type EODChart is available") ||
			ERR_MATCH("No data of type DayChart is available") ) {
			// NOTE we should skip all similar work intelligently
			qDebug() << "WARNING - DATA IS NOT AVAILABLE (no HMDS route)."
				<< histTodo->currentIndex() << id;
			p_histData.closeError( PacketHistData::ERR_NAV );
			idleTimer->setInterval( 0 );
		} else if( ERR_MATCH("No market data permissions for") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( histTodo->current().ibContract() );
			p_histData.closeError( PacketHistData::ERR_REQUEST );
			idleTimer->setInterval( 0 );
		} else {
			qDebug() << "Warning, unhandled error message.";
			// seen: "TWS exited during processing of HMDS query"
		}
		break;
	// Historical Market Data Service query message:
	case 165:
		if( ERR_MATCH("HMDS server disconnect occurred.  Attempting reconnection") ||
		    ERR_MATCH("HMDS connection attempt failed.  Connection will be re-attempted") ) {
			idleTimer->setInterval( myProp->reqTimeout );
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
		idleTimer->setInterval( 0 );
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
		idleTimer->setInterval( 0 );
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
		idleTimer->setInterval( 1000 ); //TODO wait for first tws messages
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
		idleTimer->setInterval( 0 );
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


void TwsDL::twsContractDetailsEnd( int reqId )
{
	if( currentRequest.reqId() != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:"
			<< currentRequest.reqId();
		Q_ASSERT( false );
	}
	
	idleTimer->setInterval( 0 );
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
		idleTimer->setInterval( 0 );
		qDebug() << "READY" << histTodo->currentIndex() << reqId;
	}
}


int TwsDL::storage2stdout()
{
	QTime  stopWatch;
	stopWatch.start();
	
	int countReceived = p_contractDetails.constList().size();
	
	for( int i=0; i<countReceived; i++ ) {
		
		const IB::ContractDetails &cD = p_contractDetails.constList()[i];
		const IB::Contract &c = cD.summary;


		QString c1_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
			.arg(toQString(c.symbol))
			.arg(toQString(c.secType))
			.arg(toQString(c.exchange))
			.arg(toQString(c.currency))
			.arg(toQString(c.expiry))
			.arg(c.strike)
			.arg(toQString(c.right));
		
		QString c2_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
			.arg( c.conId )
			.arg( toQString(c.multiplier) )
			.arg( toQString(c.primaryExchange) )
			.arg( toQString(c.localSymbol) )
			.arg( c.includeExpired )
			.arg( toQString(c.secIdType) )
			.arg( toQString(c.secId) );
		
		// IBString c.comboLegsDescrip;
		// ComboLegList* comboLegs;
		// UnderComp* underComp;
		
		
		QString cD1_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9"
				"\t%10\t%11\t%12\t%13\t%14\t%15")
			.arg( toQString(cD.marketName) )
			.arg( toQString(cD.tradingClass) )
			.arg( cD.minTick )
			.arg( toQString(cD.orderTypes) )
			.arg( toQString(cD.validExchanges) )
			.arg( cD.priceMagnifier )
			.arg( cD.underConId )
			.arg( toQString(cD.longName) )
			.arg( toQString(cD.contractMonth) )
			.arg( toQString(cD.industry) )
			.arg( toQString(cD.category) )
			.arg( toQString(cD.subcategory) )
			.arg( toQString(cD.timeZoneId) )
			.arg( toQString(cD.tradingHours) )
			.arg( toQString(cD.liquidHours) );
		
		// BOND values
			QString cD2_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9"
			"\t%10\t%11\t%12\t%13\t%14\t%15")
			.arg( toQString(cD.cusip) )
			.arg( toQString(cD.ratings) )
			.arg( toQString(cD.descAppend) )
			.arg( toQString(cD.bondType) )
			.arg( toQString(cD.couponType) )
			.arg( cD.callable )
			.arg( cD.putable )
			.arg( cD.coupon )
			.arg( cD.convertible )
			.arg( toQString(cD.maturity) )
			.arg( toQString(cD.issueDate) )
			.arg( toQString(cD.nextOptionDate) )
			.arg( toQString(cD.nextOptionType) )
			.arg( cD.nextOptionPartial )
			.arg( toQString(cD.notes) );
		
		
		fprintf( stderr, "_CD_C1___\t%s\n",
		         c1_str.toUtf8().constData() );
		fprintf( stderr, "_CD_C2___\t%s\n",
		         c2_str.toUtf8().constData() );
		
		fprintf( stderr, "_CD_CD___\t%s\n",
		         cD1_str.toUtf8().constData() );
		fprintf( stderr, "_CD_BOND_\t%s\n",
		         cD2_str.toUtf8().constData() );


	}
	qDebug() << QString(
		"Contracts received: %1 (%2ms)")
		.arg(countReceived).arg(stopWatch.elapsed());
	
	return countReceived;
}


void TwsDL::initWork()
{
	if( workFile.isEmpty() ) {
		qDebug() << "getting contracts from TWS";
		int i = contractDetailsTodo->fromConfig(
			myProp->contractSpecs, myProp->includeExpired );
		Q_ASSERT( i>=0 );
// 		state = IDLE;
	} else {
		if( myProp->downloadData ) {
			qDebug() << "read work from file";
			int i = histTodo->fromFile(workFile, myProp->includeExpired);
			Q_ASSERT( i>=0 );
			dumpWorkTodo();
// 			state = IDLE;;
		} else {
// 			state = QUIT_READY;
		}
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
		idleTimer->setInterval( 50 );
	} else if( state == WAIT_DATA ) {
		idleTimer->setInterval( 1000 );
		qDebug() << "TTTTTTTTTTT" << 1000;
	} else {
		idleTimer->setInterval( 0 );
		qDebug() << "TTTTTTTTTTT" << 0;
	}
}


void TwsDL::reqContractDetails( const ContractDetailsRequest& cdR )
{
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
	
	downloadData = false;
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
	
	ok = ok & get("downloadData", downloadData);
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
	
	contractSpecs.clear();
	QVector<QString> tmp;
	ok = ok & get("contractSpecs", tmp);
	foreach( QString s, tmp ) {
		QList<QString> l = s.trimmed().split(QRegExp("[ \t\r\n]*,[ \t\r\n]*"));
		Q_ASSERT( l.size() == 3 || l.size() == 4 ); // TODO handle that
		contractSpecs.append( l );
	}
	
	return ok;
}
///////////////////////////////////////////////////////////////////////////////


} // namespace Test
