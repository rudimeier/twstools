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
	idleTimer(NULL)
{
	initProperties();
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
	if( myProp->downloadData  && curIndexTodoHistData < histTodo->histRequests.size()
	    && ( myProp->reqMaxContracts <= 0 || curIndexTodoHistData < myProp->reqMaxContracts ) ) {
		currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
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
	currentRequest.reqState = GenericRequest::PENDING;
	state = WAIT_DATA;
}


void TwsDL::waitContracts()
{
	idleTimer->setInterval( 0 );
	
	if( currentRequest.reqState == GenericRequest::FINISHED ) {
		finContracts();
		return;
	} else {
		qDebug() << "Timeout waiting for data.";
		state = QUIT_ERROR;
	}
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
			histTodo->histRequests.append( hR );
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
	Q_ASSERT( curIndexTodoHistData < histTodo->histRequests.size() );
	
	const HistRequest &hR = histTodo->histRequests.at( curIndexTodoHistData );
	
	qDebug() << "DOWNLOAD DATA" << curIndexTodoHistData << currentRequest.reqId << ibToString(hR.ibContract);
	
	reqHistoricalData( hR );
	
	idleTimer->setInterval( myProp->reqTimeout );
	currentRequest.reqState = GenericRequest::PENDING;
	state = WAIT_DATA;
}


void TwsDL::waitData()
{
	idleTimer->setInterval( 0 );
	
	if( currentRequest.reqState == GenericRequest::FINISHED ) {
		switch( currentRequest.reqType ) {
		case GenericRequest::CONTRACT_DETAILS_REQUEST:
			finContracts();
			break;
		case GenericRequest::HIST_REQUEST:
			finData();
			break;
		case GenericRequest::NONE:
			Q_ASSERT( false );
			break;
		}
	} else {
		qDebug() << "Timeout waiting for data.";
		state = QUIT_ERROR;
	}
}


void TwsDL::pauseData()
{
	idleTimer->setInterval( myProp->violationPause );
	qDebug() << "PAUSE" << myProp->violationPause;
	state = IDLE;
}


void TwsDL::finData()
{
	idleTimer->setInterval( myProp->pacingTime );
	
	p_histData.dump( histTodo->histRequests.at(curIndexTodoHistData),
	                 myProp->printFormatDates );
	p_histData.clear();
	
	curIndexTodoHistData++;
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
	} else {
		Q_ASSERT( id == -1 );
	}
}


void TwsDL::errorContracts(int id, int errorCode, const QString &errorMsg)
{
	// TODO
}


void TwsDL::errorHistData(int id, int errorCode, const QString &errorMsg)
{
	if( errorCode == 162 && errorMsg.contains("pacing violation", Qt::CaseInsensitive) ) {
		idleTimer->setInterval( 0 );
		currentRequest.reqState = GenericRequest::FINISHED;
		pauseData();
	} else if( errorCode == 162 && errorMsg.contains("HMDS query returned no data", Qt::CaseInsensitive) ) {
		idleTimer->setInterval( 0 );
		currentRequest.reqState = GenericRequest::FINISHED;
		qDebug() << "READY - NO DATA" << curIndexTodoHistData << id;;
	} else if( errorCode == 162 &&
	           (errorMsg.contains("No historical market data for", Qt::CaseInsensitive) ||
	            errorMsg.contains("No data of type EODChart is available", Qt::CaseInsensitive) ) ) {
		idleTimer->setInterval( 0 );
		if( myProp->ignoreNotAvailable /*TODO we should skip all similar work intelligently*/) {
			currentRequest.reqState = GenericRequest::FINISHED;
		} // else will cause error
		qDebug() << "WARNING - THIS KIND OF DATA IS NOT AVAILABLE" << curIndexTodoHistData << id;;
	} else if( errorCode == 165 &&
	           errorMsg.contains("HMDS server disconnect occurred.  Attempting reconnection") ) {
		idleTimer->setInterval( myProp->reqTimeout );
	} else if( errorCode == 165 &&
	           errorMsg.contains("HMDS connection attempt failed.  Connection will be re-attempted") ) {
		idleTimer->setInterval( myProp->reqTimeout );
	}
	// TODO, handle:
	// 162 "Historical Market Data Service error message:HMDS query returned no data: DJX   100522C00155000@CBOE Bid"
	// 162 "Historical Market Data Service error message:No historical market data for CAC40/IND@MONEP Bid 1800"
	// 162 "Historical Market Data Service error message:No data of type EODChart is available for the exchange 'IDEAL' and the security type 'Forex' and '1 y' and '1 day'"
	// 200 "No security definition has been found for the request"
	// 162 "Historical Market Data Service error message:Historical data request pacing violation"
	// Maybe this comes only once?:
	// 165 "Historical Market Data Service query message:HMDS server disconnect occurred.  Attempting reconnection..."
	// This might come several times:
	// 165 "Historical Market Data Service query message:HMDS connection attempt failed.  Connection will be re-attempted..."
}


void TwsDL::twsConnected( bool connected )
{
	if( connected ) {
		Q_ASSERT( state == WAIT_TWS_CON );
		idleTimer->setInterval( 0 );
	} else {
		// TODO check current state
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
	currentRequest.reqState = GenericRequest::FINISHED;
}


void TwsDL::twsHistoricalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps )
{
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	Q_ASSERT( !p_histData.isFinished() );
	p_histData.append( reqId, date, open, high, low,
		close, volume, count, WAP, hasGaps );
	
	if( p_histData.isFinished() ) {
		idleTimer->setInterval( 0 );
		currentRequest.reqState = GenericRequest::FINISHED;
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
			histTodo->dump( stderr );
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
	pacingTime = 10300;
	violationPause = 60000;
	
	downloadData = false;
	reqMaxContracts = -1;
	reqMaxContractsPerSpec = -1;
	ignoreNotAvailable = true;
	
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
	ok &= get("pacingTime", pacingTime);
	ok &= get("violationPause", violationPause);
	
	ok = ok & get("downloadData", downloadData);
	ok = ok & get("reqMaxContracts", reqMaxContracts);
	ok = ok & get("reqMaxContractsPerSpec", reqMaxContractsPerSpec);
	ok = ok & get("ignoreNotAvailable", ignoreNotAvailable);
	
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
