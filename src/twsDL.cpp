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
	confFile(confFile),
	workFile(workFile),
	myProp(NULL),
	twsClient(NULL),
	twsWrapper(NULL),
	currentRequest(  *(new GenericRequest()) ),
	curIndexTodoContractDetails(0),
	curIndexTodoHistData(0),
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
	twsClient->connectTWS(
		myProp->twsHost, myProp->twsPort, myProp->clientId );
	
	state = WAIT_TWS_CON;
	idleTimer->setInterval( myProp->conTimeout );
}


void TwsDL::waitTwsCon()
{
	idleTimer->setInterval( 0 );
	
	if( twsClient->isConnected() ) {
		qDebug() << "We are connected to TWS.";
		state = IDLE;
	} else {
		qDebug() << "Timeout connecting TWS.";
		idleTimer->setInterval( 10000 );
		state = CONNECT;
	}
}


void TwsDL::idle()
{
	Q_ASSERT(currentRequest.reqType == GenericRequest::NONE);
	
	if( curIndexTodoContractDetails < contractDetailsTodo->contractDetailsRequests.size() ) {
		currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );
		getContracts();
		return;
	}
	
	// TODO we want to dump histTodo when contractDetailsTodo is finished but
	// this way it might be dumped twice
	if( curIndexTodoHistData == 0 && curIndexTodoContractDetails > 0) {
		dumpWorkTodo();
	}
	
	if( myProp->downloadData  && curIndexTodoHistData < histTodo->list().size()
	    && ( myProp->reqMaxContracts <= 0 || curIndexTodoHistData < myProp->reqMaxContracts ) ) {
		getData();
	} else {
		state = QUIT_READY;
	}
}


void TwsDL::getContracts()
{
	const ContractDetailsRequest &cdR =
		contractDetailsTodo->contractDetailsRequests.at( curIndexTodoContractDetails );
	reqContractDetails( cdR );
	
	idleTimer->setInterval( myProp->reqTimeout );
	state = WAIT_DATA;
}


void TwsDL::finContracts()
{
	idleTimer->setInterval( 0 );
	
	int inserted;
	inserted = storage2stdout();
	
	for( int i = 0; i<p_contractDetails.constList().size(); i++ ) {
		const IB::ContractDetails &cd = p_contractDetails.constList().at(i);
		
		if( myProp->reqMaxContractsPerSpec > 0 && myProp->reqMaxContractsPerSpec <= i ) {
			break;
		}
		
		foreach( QString wts, myProp->whatToShow ) {
			HistRequest hR;
			hR.initialize( cd.summary, myProp->endDateTime, myProp->durationStr,
			               myProp->barSizeSetting, wts, myProp->useRTH, myProp->formatDate );
			histTodo->add( hR );
		}
	}
	
	p_contractDetails.clear();
	if( inserted == -1 ) {
		state = QUIT_ERROR;
		return;
	}
	
	curIndexTodoContractDetails++;
	currentRequest.close();
	state = IDLE;
}


void TwsDL::getData()
{
	Q_ASSERT( curIndexTodoHistData < histTodo->list().size() );
	
	int wait = pacingControl.goodTime(
		histTodo->list().at(curIndexTodoHistData).ibContract );
	if( wait > 0 ) {
		idleTimer->setInterval( qMin( 1000, wait ) );
		return;
	}
	if( wait < -1 ) {
		// just debug timer resolution
		qDebug() << "late timeout:" << wait;
	}
	
	pacingControl.addRequest(
		histTodo->list().at(curIndexTodoHistData).ibContract );
	
	currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
	const HistRequest &hR = histTodo->list().at( curIndexTodoHistData );
	
	qDebug() << "DOWNLOAD DATA" << curIndexTodoHistData << currentRequest.reqId << ibToString(hR.ibContract);
	
	reqHistoricalData( hR );
	
	idleTimer->setInterval( myProp->reqTimeout );
	state = WAIT_DATA;
}


void TwsDL::waitData()
{
	idleTimer->setInterval( 0 );
	
	switch( currentRequest.reqType ) {
	case GenericRequest::CONTRACT_DETAILS_REQUEST:
		if( p_contractDetails.isFinished() ) {
			finContracts();
		}
		return;
	case GenericRequest::HIST_REQUEST:
		if( p_histData.isFinished() ) {
			finData();
		}
		return;
	case GenericRequest::NONE:
		Q_ASSERT( false );
		break;
	}
	
	qDebug() << "Timeout waiting for data.";
	state = QUIT_ERROR;
}


void TwsDL::finData()
{
	idleTimer->setInterval( 0 );
	
	Q_ASSERT( p_histData.isFinished() );
	if( !p_histData.needRepeat() ) {
		p_histData.dump( histTodo->list().at(curIndexTodoHistData),
		                 myProp->printFormatDates );
		curIndexTodoHistData++;
	}
	
	p_histData.clear();
	currentRequest.close();
	state = IDLE;
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
	if( id == currentRequest.reqId ) {
		qDebug() << "ERROR for request" << id << errorCode <<errorMsg;
		if( state == WAIT_DATA ) {
			switch( currentRequest.reqType ) {
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
	
	Q_ASSERT( id == -1 );
	
	// TODO do better
	switch( errorCode ) {
		case 1100:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been lost."));
			idleTimer->setInterval( myProp->reqTimeout );
			break;
		case 1101:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been restored - data lost."));
			if( currentRequest.reqType == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( true );
				idleTimer->setInterval( 0 );
			}
			break;
		case 1102:
			Q_ASSERT(ERR_MATCH("Connectivity between IB and TWS has been restored - data maintained."));
			if( currentRequest.reqType == GenericRequest::HIST_REQUEST ) {
				p_histData.closeError( true );
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
			dataFarms.notify( errorCode, errorMsg );
			pacingControl.clear();
			break;
	}
}


void TwsDL::errorContracts(int id, int errorCode, const QString &errorMsg)
{
	// TODO
}


void TwsDL::errorHistData(int id, int errorCode, const QString &errorMsg)
{
	switch( errorCode ) {
	//Historical Market Data Service error message:
	case 162:
		if( ERR_MATCH("Historical data request pacing violation") ) {
			p_histData.closeError( true );
			pacingControl.notifyViolation(
				histTodo->list().at(curIndexTodoHistData).ibContract );
			idleTimer->setInterval( 0 );
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			qDebug() << "READY - NO DATA" << curIndexTodoHistData << id;
			dataFarms.learnHmds( histTodo->list().at(curIndexTodoHistData).ibContract );
			p_histData.closeError( false );
			idleTimer->setInterval( 0 );
		} else if( ERR_MATCH("No historical market data for") ||
		           ERR_MATCH("No data of type EODChart is available") ) {
			/*TODO we should skip all similar work intelligently*/
			qDebug() << "WARNING - THIS KIND OF DATA IS NOT AVAILABLE" << curIndexTodoHistData << id;
			dataFarms.learnHmds( histTodo->list().at(curIndexTodoHistData).ibContract );
			p_histData.closeError( false );
			idleTimer->setInterval( 0 );
		} else {
			// nothing
		}
		break;
	//Historical Market Data Service query message:
	case 165:
		if( ERR_MATCH("HMDS server disconnect occurred.  Attempting reconnection") ||
		    ERR_MATCH("HMDS connection attempt failed.  Connection will be re-attempted") ||
			ERR_MATCH("HMDS server connection was successful") ) {
			idleTimer->setInterval( myProp->reqTimeout );
		} else {
			// nothing
		}
		break;
	default:
		// nothing
		break;
	}
	// TODO, handle:
	// 200 "No security definition has been found for the request"
	//
	// -1 1100 "Connectivity between IB and TWS has been lost."
	// -1 1102 "Connectivity between IB and TWS has been restored - data maintained."
}

#undef ERR_MATCH


void TwsDL::twsConnected( bool connected )
{
	if( connected ) {
		Q_ASSERT( state == WAIT_TWS_CON );
		idleTimer->setInterval( 1000 ); //TODO wait for first tws messages
	} else {
		// TODO should we check current state here?
		switch( currentRequest.reqType ) {
		case GenericRequest::CONTRACT_DETAILS_REQUEST:
			if( !p_contractDetails.isFinished() ) {
				Q_ASSERT(false); // TODO repeat
			}
			finContracts();
			break;
		case GenericRequest::HIST_REQUEST:
			if( !p_histData.isFinished() ) {
				p_histData.closeError( true );
			}
			finData();
			break;
		case GenericRequest::NONE:
			qDebug() << "NONE";
			break;
		}
// 		Q_ASSERT( state == IDLE || state == CONNECT );
		Q_ASSERT( currentRequest.reqType == GenericRequest::NONE );
		state = CONNECT;
		idleTimer->setInterval( 10000 );
	}
}


void TwsDL::twsContractDetails( int reqId, const IB::ContractDetails &ibContractDetails )
{
	
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	p_contractDetails.append(reqId, ibContractDetails);
}


void TwsDL::twsContractDetailsEnd( int reqId )
{
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	idleTimer->setInterval( 0 );
	p_contractDetails.setFinished();
}


void TwsDL::twsHistoricalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps )
{
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	// TODO we shouldn't do this each row
	dataFarms.learnHmds( histTodo->list().at(curIndexTodoHistData).ibContract );
	
	Q_ASSERT( !p_histData.isFinished() );
	p_histData.append( reqId, date, open, high, low,
		close, volume, count, WAP, hasGaps );
	
	if( p_histData.isFinished() ) {
		idleTimer->setInterval( 0 );
		qDebug() << "READY" << curIndexTodoHistData << reqId;
	}
}


int TwsDL::storage2stdout()
{
	QTime  stopWatch;
	stopWatch.start();
	
	int countReceived = p_contractDetails.constList().size();
	
	for( int i=0; i<countReceived; i++ ) {
		
		const IB::ContractDetails *ibContractDetails = &p_contractDetails.constList()[i];
		
		qDebug() << toQString(ibContractDetails->summary.symbol)
		         << toQString(ibContractDetails->summary.secType)
		         << toQString(ibContractDetails->summary.expiry)
		         << ibContractDetails->summary.strike
		         << toQString(ibContractDetails->summary.right)
		         << toQString(ibContractDetails->summary.exchange)
		         << toQString(ibContractDetails->summary.currency)
		         << toQString(ibContractDetails->summary.localSymbol)
		         << toQString(ibContractDetails->marketName)
		         << toQString(ibContractDetails->tradingClass)
		         << (int)ibContractDetails->summary.conId
		         << ibContractDetails->minTick
		         << toQString(ibContractDetails->summary.multiplier)
		         << (int)ibContractDetails->priceMagnifier;
// 		         << toQString(ibContractDetails->orderTypes)
// 		         << toQString(ibContractDetails->validExchanges);
		
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
		int i = contractDetailsTodo->fromConfig( myProp->contractSpecs );
		Q_ASSERT( i>=0 );
// 		state = IDLE;
	} else {
		if( myProp->downloadData ) {
			qDebug() << "read work from file";
			int i = histTodo->fromFile(workFile);
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
	histTodo->dump( stderr );
}


TwsDL::State TwsDL::currentState() const
{
	return state;
}


void TwsDL::reqContractDetails( const ContractDetailsRequest& cdR )
{
	twsClient->reqContractDetails( currentRequest.reqId, cdR.ibContract );
}


void TwsDL::reqHistoricalData( const HistRequest& hR )
{
	p_histData.record( currentRequest.reqId );
	twsClient->reqHistoricalData( currentRequest.reqId,
	                              hR.ibContract,
	                              hR.endDateTime,
	                              hR.durationStr,
	                              hR.barSizeSetting,
	                              hR.whatToShow,
	                              hR.useRTH,
	                              hR.formatDate );
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
