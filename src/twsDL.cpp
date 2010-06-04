#include "twsDL.h"

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




class HistRequest
{
	public:
		bool fromString( const QString& );
		QString toString() const;
		void clear();
		
		IB::Contract ibContract;
		QString endDateTime;
		QString whatToShow;
};


bool HistRequest::fromString( const QString& s )
{
	bool ok = false;
	QStringList sl = s.split('\t');
	
	if( sl.size() < 9 ) {
		return ok;
	}
	
	int i = 0;
	endDateTime = sl.at(i++);
	whatToShow = sl.at(i++);
	
	ibContract.symbol = toIBString(sl.at(i++));
	ibContract.secType = toIBString(sl.at(i++));
	ibContract.exchange = toIBString(sl.at(i++));
	ibContract.currency = toIBString(sl.at(i++));
	ibContract.expiry = toIBString(sl.at(i++));
	ibContract.strike = sl.at(i++).toDouble( &ok );
	ibContract.right = toIBString(sl.at(i++));
	
	return ok;
}


QString HistRequest::toString() const
{
	QString c_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
		.arg(toQString(ibContract.symbol))
		.arg(toQString(ibContract.secType))
		.arg(toQString(ibContract.exchange))
		.arg(toQString(ibContract.currency))
		.arg(toQString(ibContract.expiry))
		.arg(ibContract.strike)
		.arg(toQString(ibContract.right));
	
	QString retVal = QString("%1\t%2\t%3")
		.arg(endDateTime)
		.arg(whatToShow)
		.arg(c_str);
	
	return retVal;
}


void HistRequest::clear()
{
	ibContract = IB::Contract();
	whatToShow.clear();
}








class WorkTodo
{
	public:
		int fromFile( const QString & fileName );
		void dump( FILE *stream ) const;
		
		QList<HistRequest> histRequests;
		
	private:
		int read_file( const QString & fileName, QList<QByteArray> *list ) const;
};


int WorkTodo::fromFile( const QString & fileName )
{
	histRequests.clear();
	
	QList<QByteArray> rows;
	int retVal = read_file( fileName, &rows );
	if( retVal == -1) {
		return retVal;
	}
	foreach( QByteArray row, rows ) {
		if( row.startsWith('[') ) {
			int firstTab = row.indexOf('\t');
			Q_ASSERT( row.size() > firstTab );
			Q_ASSERT( firstTab >= 0 ); //TODO
			row.remove(0, firstTab+1 );
		}
		HistRequest hR;
		bool ok = hR.fromString( row );
		Q_ASSERT(ok); //TODO
		histRequests.append( hR );
	}
	return retVal;
}


int WorkTodo::read_file( const QString & fileName, QList<QByteArray> *list ) const
{
	int retVal = -1;
	QFile f( fileName );
	if (f.open(QFile::ReadOnly)) {
		retVal = 0;
		while (!f.atEnd()) {
			QByteArray line = f.readLine();
			line.chop(1); //remove line feed
			if( line.startsWith('#') || line.isEmpty() ) {
				continue;
			}
			list->append(line);
			retVal++;
		}
	} else {
// 		_lastError = QString("can't read file '%1'").arg(fileName);
	}
	return retVal;
}


void WorkTodo::dump( FILE *stream ) const
{
	for(int i=0; i < histRequests.size(); i++ ) {
		fprintf( stream, "[%d]\t%s\n",
		         i,
		         histRequests.at(i).toString().toUtf8().constData() );
	}
}








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
	workTodo( new WorkTodo() ),
	idleTimer(NULL)
{
	initProperties();
	initTwsClient();
	initIdleTimer();
}


TwsDL::~TwsDL()
{
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
	currentReqId = 0;
	countNewContracts = 0;
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
	int i = currentReqId;
	
	IB::Contract ibContract;
	ibContract.symbol = toIBString(myProp->contractSpecs[i][0]);
	ibContract.secType = toIBString(myProp->contractSpecs[i][1]);
	ibContract.exchange= toIBString(myProp->contractSpecs[i][2]);
	// optional filter for a single expiry
	QString e = myProp->contractSpecs[i].size() > 3 ? myProp->contractSpecs[i][3] : "";
	ibContract.expiry = toIBString( e );
	
	twsClient->reqContractDetails( currentReqId, ibContract );
	
	idleTimer->setInterval( myProp->reqTimeout );
	finishedReq = false;
	state = WAIT_CONTRACTS;
}


void TwsDL::waitContracts()
{
	idleTimer->setInterval( 0 );
	
	if( finishedReq ) {
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
			HistRequest hR = { cd.summary, myProp->endDateTime, wts };
			workTodo->histRequests.append( hR );
		}
	}
	
	contractDetailsStorage.clear();
	if( inserted == -1 ) {
		state = QUIT_ERROR;
		return;
	}
	countNewContracts += inserted;
	
	
	currentReqId++;
	if( currentReqId < myProp->contractSpecs.size() ) {
		state = GET_CONTRACTS;
	} else {
		dumpWorkTodo();
		if( myProp->downloadData ) {
			state = GET_DATA;;
		} else {
			state = QUIT_READY;
		}
	}
}


void TwsDL::getData()
{
	Q_ASSERT( curReqContractIndex < workTodo->histRequests.size() );
	
	// whyever we can't use that contract directly
	const HistRequest &hR = workTodo->histRequests.at( curReqContractIndex );
	IB::Contract cF = hR.ibContract;
	IB::Contract c;
	
	qDebug() << "DOWNLOAD DATA" << curReqContractIndex << currentReqId << ibToString(cF);
	
	c.symbol = cF.symbol/*"DJX"*/;
	c.secType = cF.secType /*"OPT"*/;
	c.exchange = cF.exchange /*"CBOE"*/;
	c.currency = cF.currency /*"USD"*/;
	c.right = cF.right /*"P"*/;
	c.strike = cF.strike /*75.0*/;
	c.expiry = cF.expiry /*"20100520"*/;
	
	twsClient->reqHistoricalData( currentReqId,
	                              c,
	                              hR.endDateTime,
	                              myProp->durationStr,
	                              myProp->barSizeSetting,
	                              hR.whatToShow,
	                              myProp->useRTH,
	                              myProp->formatDate );
	
	idleTimer->setInterval( myProp->reqTimeout );
	finishedReq = false;
	state = WAIT_DATA;
}


void TwsDL::waitData()
{
	idleTimer->setInterval( 0 );
	
	if( finishedReq ) {
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
	currentReqId++;
	curReqContractIndex ++;
	if( curReqContractIndex < workTodo->histRequests.size() &&
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
	
	myProp = new PropTWSTool(prop);
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
	if( id == currentReqId ) {
		qDebug() << "ERROR for request" << id << errorCode <<errorMsg;
		if( state == WAIT_DATA ) {
			if( errorCode == 162 && errorMsg.contains("pacing violation", Qt::CaseInsensitive) ) {
				idleTimer->setInterval( 0 );
				currentReqId++;
				state = PAUSE_DATA;
			} else if( errorCode == 162 && errorMsg.contains("HMDS query returned no data", Qt::CaseInsensitive) ) {
				idleTimer->setInterval( 0 );
				finishedReq = true;
				qDebug() << "READY - NO DATA" << curReqContractIndex << id;;
			} else if( errorCode == 162 &&
			           (errorMsg.contains("No historical market data for", Qt::CaseInsensitive) ||
			            errorMsg.contains("No data of type EODChart is available", Qt::CaseInsensitive) ) ) {
				idleTimer->setInterval( 0 );
				if( myProp->ignoreNotAvailable /*TODO we should skip all similar work intelligently*/) {
					finishedReq = true;
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
	
	if( currentReqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentReqId;
		Q_ASSERT( false );
	}
	
	contractDetailsStorage.append(ibContractDetails);
}


void TwsDL::contractDetailsEnd( int reqId )
{
	if( currentReqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentReqId;
		Q_ASSERT( false );
	}
	
	idleTimer->setInterval( 0 );
	finishedReq = true;
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
	if( currentReqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentReqId;
		Q_ASSERT( false );
	}
	
	if( date.startsWith("finished") ) {
		idleTimer->setInterval( 0 );
		finishedReq = true;
		qDebug() << "READY" << curReqContractIndex << reqId;;
	} else {
		const IB::Contract &c = workTodo->histRequests.at(curReqContractIndex).ibContract;
		const QString &wts = workTodo->histRequests.at(curReqContractIndex).whatToShow;
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
			int i = workTodo->fromFile(workFile);
			Q_ASSERT( i>=0 );
			workTodo->dump( stderr );
			state = GET_DATA;;
		} else {
			state = QUIT_READY;
		}
	}
}


void TwsDL::dumpWorkTodo() const
{
	workTodo->dump( stderr );
}


TwsDL::State TwsDL::currentState() const
{
	return state;
}




///////////////////////////////////////////////////////////////////////////////
// PropTWSTool
PropTWSTool::PropTWSTool( const Properties& prop, const QString& cName ) :
	PropGeneral(prop,cName)
{
	PROP_DEBUG( 2, "INITIALIZING" );
	//initializing fields
	initDefaults();
}


void PropTWSTool::initDefaults()
{
	PropGeneral::initDefaults();
	
	twsHost  = "localhost";
	twsPort  = 6666;
	clientId = 66;
	
	conTimeout = 1000;
	reqTimeout = 20000;
	pacingTime = 10300;
	violationPause = 60000;
	
	reqExpiry = "";
	
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


bool PropTWSTool::readProperties()
{
	PROP_DEBUG( 2, "READ CONFIG" );
	bool ok = true;
	
	ok = ok & PropGeneral::readProperties();
	
	ok = ok & get("twsHost",       twsHost);
	ok = ok & get("twsPort",       twsPort);
	ok = ok & get("clientId",      clientId);
	
	ok &= get("conTimeout", conTimeout);
	ok &= get("reqTimeout", reqTimeout);
	ok &= get("pacingTime", pacingTime);
	ok &= get("violationPause", violationPause);
	
	ok &= get("reqExpiry", reqExpiry);
	
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
