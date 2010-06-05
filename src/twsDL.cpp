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













QHash<QString, const char*> init_short_wts()
{
	QHash<QString, const char*> ht;
	ht.insert("TRADES", "T");
	ht.insert("MIDPOINT", "M");
	ht.insert("BID", "B");
	ht.insert("ASK", "A");
	ht.insert("BID_ASK", "BA");
	ht.insert("HISTORICAL_VOLATILITY", "HV");
	ht.insert("OPTION_IMPLIED_VOLATILITY", "OIV");
	ht.insert("OPTION_VOLUME", "OV");
	return ht;
}

static const QHash<QString, const char*> short_wts = init_short_wts();


QHash<QString, const char*> init_short_bar_size()
{
	QHash<QString, const char*> ht;
	ht.insert("1 secs",   "s01");
	ht.insert("5 secs",   "s05");
	ht.insert("15 secs",  "s15");
	ht.insert("30 secs",  "s30");
	ht.insert("1 min",    "m01");
	ht.insert("2 mins",   "m02");
	ht.insert("3 mins",   "m03");
	ht.insert("5 mins",   "m05");
	ht.insert("15 mins",  "m15");
	ht.insert("30 mins",  "m30");
	ht.insert("1 hour",   "h01");
	ht.insert("1 day",    "eod");
	ht.insert("1 week",   "w01");
	ht.insert("1 month",  "x01");
	ht.insert("3 months", "x03");
	ht.insert("1 year",   "y01");
	return ht;
}

static const QHash<QString, const char*> short_bar_size = init_short_bar_size();





TwsDL::TwsDL( const QString& confFile, const QString& workFile ) :
	state(START),
	confFile(confFile),
	workFile(workFile),
	myProp(NULL),
	twsClient(NULL),
	twsWrapper(NULL),
	currentRequest(  *(new GenericRequest()) ),
	contractDetailsTodo( new ContractDetailsTodo() ),
	histTodo( new HistTodo() ),
	idleTimer(NULL)
{
	initProperties();
	initTwsClient();
	initIdleTimer();
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
	if( idleTimer != NULL ) {
		delete idleTimer;
	}
}


void TwsDL::start()
{
	Q_ASSERT( !idleTimer->isActive() );
	Q_ASSERT( (state == START) ||
		(state == QUIT_READY) || (state == QUIT_ERROR) );
	
	state = START;
	idleTimer->setInterval(0);
	idleTimer->start();
}


void TwsDL::idleTimeout()
{
	switch( state ) {
		case START:
			onStart();
			break;
		case WAIT_TWS_CON:
			waitTwsCon();
			break;
		case GET_CONTRACTS:
			getContracts();
			break;
		case WAIT_CONTRACTS:
			waitContracts();
			break;
		case FIN_CONTRACTS:
			finContracts();
			break;
		case GET_DATA:
			getData();
			break;
		case WAIT_DATA:
			waitData();
			break;
		case PAUSE_DATA:
			pauseData();
			break;
		case FIN_DATA:
			finData();
			break;
		case QUIT_READY:
				onQuit(0);
			break;
		case QUIT_ERROR:
				onQuit(-1);
			break;
	}
}


void TwsDL::onStart()
{
	countNewContracts = 0;
	curReqSpecIndex = 0;
	curReqContractIndex = 0;
	
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
		startWork();
	} else {
		qDebug() << "Timeout connecting TWS.";
		state = QUIT_ERROR;
	}
}


void TwsDL::getContracts()
{
	int i = currentRequest.reqId;
	
	IB::Contract ibContract;
	ibContract.symbol = toIBString(myProp->contractSpecs[i][0]);
	ibContract.secType = toIBString(myProp->contractSpecs[i][1]);
	ibContract.exchange= toIBString(myProp->contractSpecs[i][2]);
	// optional filter for a single expiry
	QString e = myProp->contractSpecs[i].size() > 3 ? myProp->contractSpecs[i][3] : "";
	ibContract.expiry = toIBString( e );
	
	ContractDetailsRequest cdR;
	cdR.initialize( ibContract );
	reqContractDetails( cdR );
	
	idleTimer->setInterval( myProp->reqTimeout );
	currentRequest.reqState = GenericRequest::PENDING;
	state = WAIT_CONTRACTS;
}


void TwsDL::waitContracts()
{
	idleTimer->setInterval( 0 );
	
	if( currentRequest.reqState == GenericRequest::FINISHED ) {
		state = FIN_CONTRACTS;
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
	
	for( int i = 0; i<contractDetailsStorage.size(); i++ ) {
		const IB::ContractDetails &cd = contractDetailsStorage.at(i);
		
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
	
	contractDetailsStorage.clear();
	if( inserted == -1 ) {
		state = QUIT_ERROR;
		return;
	}
	countNewContracts += inserted;
	
	
	if( (currentRequest.reqId + 1) < myProp->contractSpecs.size() ) {
		curReqSpecIndex++;
		currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );
		state = GET_CONTRACTS;
	} else {
		dumpWorkTodo();
		if( myProp->downloadData ) {
			currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
			state = GET_DATA;;
		} else {
			state = QUIT_READY;
		}
	}
}


void TwsDL::getData()
{
	Q_ASSERT( curReqContractIndex < histTodo->histRequests.size() );
	
	const HistRequest &hR = histTodo->histRequests.at( curReqContractIndex );
	
	qDebug() << "DOWNLOAD DATA" << curReqContractIndex << currentRequest.reqId << ibToString(hR.ibContract);
	
	reqHistoricalData( hR );
	
	idleTimer->setInterval( myProp->reqTimeout );
	currentRequest.reqState = GenericRequest::PENDING;
	state = WAIT_DATA;
}


void TwsDL::waitData()
{
	idleTimer->setInterval( 0 );
	
	if( currentRequest.reqState == GenericRequest::FINISHED ) {
		state = FIN_DATA;
		return;
	} else {
		qDebug() << "Timeout waiting for data.";
		state = QUIT_ERROR;
	}
}


void TwsDL::pauseData()
{
	idleTimer->setInterval( myProp->violationPause );
	qDebug() << "PAUSE" << myProp->violationPause;
	state = GET_DATA;
}


void TwsDL::finData()
{
	curReqContractIndex ++;
	currentRequest.nextRequest( GenericRequest::HIST_REQUEST );
	if( curReqContractIndex < histTodo->histRequests.size() &&
	    ( myProp->reqMaxContracts <= 0 || curReqContractIndex < myProp->reqMaxContracts ) ) {
		idleTimer->setInterval( myProp->pacingTime );
		state = GET_DATA;
	} else {
		idleTimer->setInterval( 0 );
		state = QUIT_READY;
	}
}


void TwsDL::onQuit( int /*ret*/ )
{
	qDebug() << "Today we got" << countNewContracts << "new contracts from IB";
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
		this, SLOT(error(int, int, const QString &)), Qt::QueuedConnection );
	
	connect ( twsClient, SIGNAL(connected(bool)),
		 this,SLOT(twsConnected(bool)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(contractDetails(int, IB::ContractDetails)),
		 this,SLOT(contractDetails2Storage(int, IB::ContractDetails)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(contractDetailsEnd(int)),
		 this,SLOT(contractDetailsEnd(int)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(historicalData(int, const QString&, double, double, double,
			double, int, int, double, bool )),
		 this,SLOT(historicalData(int, const QString&, double, double, double,
			double, int, int, double, bool )), Qt::QueuedConnection );
}


void TwsDL::error(int id, int errorCode, const QString &errorMsg)
{
	if( id == currentRequest.reqId ) {
		qDebug() << "ERROR for request" << id << errorCode <<errorMsg;
		if( state == WAIT_DATA ) {
			if( errorCode == 162 && errorMsg.contains("pacing violation", Qt::CaseInsensitive) ) {
				idleTimer->setInterval( 0 );
				currentRequest.nextRequest(GenericRequest::HIST_REQUEST );
				state = PAUSE_DATA;
			} else if( errorCode == 162 && errorMsg.contains("HMDS query returned no data", Qt::CaseInsensitive) ) {
				idleTimer->setInterval( 0 );
				currentRequest.reqState = GenericRequest::FINISHED;
				qDebug() << "READY - NO DATA" << curReqContractIndex << id;;
			} else if( errorCode == 162 &&
			           (errorMsg.contains("No historical market data for", Qt::CaseInsensitive) ||
			            errorMsg.contains("No data of type EODChart is available", Qt::CaseInsensitive) ) ) {
				idleTimer->setInterval( 0 );
				if( myProp->ignoreNotAvailable /*TODO we should skip all similar work intelligently*/) {
					currentRequest.reqState = GenericRequest::FINISHED;
				} // else will cause error
				qDebug() << "WARNING - THIS KIND OF DATA IS NOT AVAILABLE" << curReqContractIndex << id;;
			} else if( errorCode == 165 &&
			           errorMsg.contains("HMDS server disconnect occurred.  Attempting reconnection") ) {
				idleTimer->setInterval( myProp->reqTimeout );
			} else if( errorCode == 165 &&
			           errorMsg.contains("HMDS connection attempt failed.  Connection will be re-attempted") ) {
				idleTimer->setInterval( myProp->reqTimeout );
			}
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
	} else {
		Q_ASSERT( id == -1 );
	}
}


void TwsDL::twsConnected( bool /*connected*/ )
{
	idleTimer->setInterval( 0 );
}


void TwsDL::contractDetails2Storage( int reqId, const IB::ContractDetails &ibContractDetails )
{
	
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	contractDetailsStorage.append(ibContractDetails);
}


void TwsDL::contractDetailsEnd( int reqId )
{
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	idleTimer->setInterval( 0 );
	currentRequest.reqState = GenericRequest::FINISHED;
}


/// stupid static helper
QString ibDate2ISO( const QString &ibDate )
{
	QDateTime dt;
	
	dt = QDateTime::fromString( ibDate, "yyyyMMdd  hh:mm:ss");
	if( dt.isValid() ) {
		return dt.toString("yyyy-MM-dd hh:mm:ss");
	}
	
	dt.setDate( QDate::fromString( ibDate, "yyyyMMdd") );
	if( dt.isValid() ) {
		return dt.toString("yyyy-MM-dd");
	}
	
	bool ok = false;
	uint t = ibDate.toUInt( &ok );
	if( ok ) {
		dt.setTime_t( t );
		return dt.toString("yyyy-MM-dd hh:mm:ss");
	}
	
	return QString();
}



void TwsDL::historicalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps )
{
	if( currentRequest.reqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentRequest.reqId;
		Q_ASSERT( false );
	}
	
	if( date.startsWith("finished") ) {
		idleTimer->setInterval( 0 );
		currentRequest.reqState = GenericRequest::FINISHED;
		qDebug() << "READY" << curReqContractIndex << reqId;;
	} else {
		const IB::Contract &c = histTodo->histRequests.at(curReqContractIndex).ibContract;
		const QString &wts = histTodo->histRequests.at(curReqContractIndex).whatToShow;
		QString expiry = toQString(c.expiry);
		QString dateTime = date;
		if( myProp->printFormatDates ) {
			if( expiry.isEmpty() ) {
				expiry = "0000-00-00";
			} else {
				expiry = ibDate2ISO( toQString(c.expiry) );
			}
			dateTime = ibDate2ISO(date);
			Q_ASSERT( !expiry.isEmpty() && !dateTime.isEmpty() ); //TODO
		}
		QString c_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
			.arg(toQString(c.symbol))
			.arg(toQString(c.secType))
			.arg(toQString(c.exchange))
			.arg(toQString(c.currency))
			.arg(expiry)
			.arg(c.strike)
			.arg(toQString(c.right));
		printf("%s\t%s\t%s\t%s\t%f\t%f\t%f\t%f\t%d\t%d\t%f\t%d\n",
		       short_wts.value( wts, "NNN" ),
		       short_bar_size.value( myProp->barSizeSetting, "00N" ),
		       c_str.toUtf8().constData(),
		       dateTime.toUtf8().constData(), open, high, low, close, volume, count, WAP, hasGaps);
		fflush(stdout);
	}
}


int TwsDL::storage2stdout()
{
	QTime  stopWatch;
	stopWatch.start();
	
	int countReceived = contractDetailsStorage.size();
	
	for( int i=0; i<countReceived; i++ ) {
		
		IB::ContractDetails *ibContractDetails = &contractDetailsStorage[i];
		
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


void TwsDL::startWork()
{
	if( workFile.isEmpty() ) {
		qDebug() << "getting contracts from TWS";
		state = GET_CONTRACTS;
	} else {
		if( myProp->downloadData ) {
			qDebug() << "read work from file";
			int i = histTodo->fromFile(workFile);
			Q_ASSERT( i>=0 );
			histTodo->dump( stderr );
			state = GET_DATA;;
		} else {
			state = QUIT_READY;
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
