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
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>




namespace Test {




struct HistRequest
{
	IB::Contract ibContract;
	QString whatToShow;
};




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
	ht.insert("1 secs",   "01s");
	ht.insert("5 secs",   "05s");
	ht.insert("15 secs",  "15s");
	ht.insert("30 secs",  "30s");
	ht.insert("1 min",    "01m");
	ht.insert("2 mins",   "02m");
	ht.insert("3 mins",   "03m");
	ht.insert("5 mins",   "05m");
	ht.insert("15 mins",  "15m");
	ht.insert("30 mins",  "30m");
	ht.insert("1 hour",   "01h");
	ht.insert("1 day",    "01d");
	ht.insert("1 week",   "01w");
	ht.insert("1 month",  "01x");
	ht.insert("3 months", "03x");
	ht.insert("1 year",   "01y");
	return ht;
}

static const QHash<QString, const char*> short_bar_size = init_short_bar_size();





Worker::Worker( const QString& confFile ) :
	state(START),
	confFile(confFile),
	myProp(NULL),
	twsClient(NULL),
	twsWrapper(NULL),
	db(NULL),
	symbolQuery(NULL),
	warnQuery(NULL),
	idleTimer(NULL)
{
	initProperties();
	initTwsClient();
	initIdleTimer();
}


Worker::~Worker()
{
	removeDb();
	
	if( twsClient != NULL ) {
		delete twsClient;
	}
	if( twsWrapper != NULL ) {
		delete twsWrapper;
	}
	if( myProp  != NULL ) {
		delete myProp;
	}
	if( idleTimer != NULL ) {
		delete idleTimer;
	}
}


void Worker::start()
{
	Q_ASSERT( !idleTimer->isActive() );
	Q_ASSERT( (state == START) ||
		(state == QUIT_READY) || (state == QUIT_ERROR) );
	
	state = START;
	idleTimer->setInterval(0);
	idleTimer->start();
}


void Worker::idleTimeout()
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


void Worker::onStart()
{
	currentReqId = 0;
	countNewContracts = 0;
	curReqContractIndex = 0;
	
	if( myProp->useDB && !initDb() ) {
		state = QUIT_ERROR;
		return;
	}
	
	twsClient->connectTWS(
		myProp->twsHost, myProp->twsPort, myProp->clientId );
	
	state = WAIT_TWS_CON;
	idleTimer->setInterval( myProp->conTimeout );
}


void Worker::waitTwsCon()
{
	idleTimer->setInterval( 0 );
	
	if( twsClient->isConnected() ) {
		qDebug() << "We are connected to TWS.";
		state = GET_CONTRACTS;
	} else {
		qDebug() << "Timeout connecting TWS.";
		state = QUIT_ERROR;
	}
}


void Worker::getContracts()
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


void Worker::waitContracts()
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


void Worker::finContracts()
{
	idleTimer->setInterval( 0 );
	
	int inserted;
	if( myProp->useDB ) {
		inserted = storage2DB();
	} else {
		inserted = storage2stdout();
	}
	
	for( int i = 0; i<contractDetailsStorage.size(); i++ ) {
		const IB::ContractDetails &cd = contractDetailsStorage.at(i);
		
		if( myProp->reqMaxContractsPerSpec > 0 && myProp->reqMaxContractsPerSpec <= i ) {
			break;
		}
		
		foreach( QString wts, myProp->whatToShow ) {
			HistRequest hR = { cd.summary, wts };
			histRequests.append( hR );
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
		if( myProp->downloadData ) {
			state = GET_DATA;;
		} else {
			state = QUIT_READY;
		}
	}
}


void Worker::getData()
{
	Q_ASSERT( curReqContractIndex < histRequests.size() );
	
	// whyever we can't use that contract directly
	const HistRequest &hR = histRequests.at( curReqContractIndex );
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
	                              myProp->endDateTime,
	                              myProp->durationStr,
	                              myProp->barSizeSetting,
	                              hR.whatToShow,
	                              myProp->useRTH,
	                              myProp->formatDate );
	
	idleTimer->setInterval( myProp->reqTimeout );
	finishedReq = false;
	state = WAIT_DATA;
}


void Worker::waitData()
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


void Worker::pauseData()
{
	idleTimer->setInterval( myProp->violationPause );
	qDebug() << "PAUSE" << myProp->violationPause;
	state = GET_DATA;
}


void Worker::finData()
{
	currentReqId++;
	curReqContractIndex ++;
	if( curReqContractIndex < histRequests.size() &&
	    ( myProp->reqMaxContracts <= 0 || curReqContractIndex < myProp->reqMaxContracts ) ) {
		idleTimer->setInterval( myProp->pacingTime );
		state = GET_DATA;
	} else {
		idleTimer->setInterval( 0 );
		state = QUIT_READY;
	}
}


void Worker::onQuit( int /*ret*/ )
{
	qDebug() << "Today we got" << countNewContracts << "new contracts from IB";
	idleTimer->stop();
	emit finished();
}


void Worker::initProperties()
{
	Properties prop;
	prop.readConfigFile(confFile);
	
	myProp = new PropTWSTool(prop);
	myProp->readProperties();
}


void Worker::initIdleTimer()
{
	Q_ASSERT( idleTimer == NULL );
	
	idleTimer = new QTimer();
	idleTimer->setSingleShot(false);
	connect( idleTimer, SIGNAL(timeout()), this, SLOT(idleTimeout()) );
}


void Worker::initTwsClient()
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


bool Worker::initDb()
{
	Q_ASSERT( (db == NULL) && (symbolQuery == NULL) && (warnQuery == NULL) );
	
	db = new QSqlDatabase();
	*db = QSqlDatabase::addDatabase ( "QMYSQL","symbolData" );
	Q_ASSERT( !db->isOpen() );
	
	db->setHostName( myProp->dbHost );
	if( myProp->dbPort>=0) {
		db->setPort( myProp->dbPort );
	}
	db->setDatabaseName( myProp->staticDB );
	db->setUserName( myProp->dbUser );
	db->setPassword( myProp->dbPwd );
	
	if( !db->open() ) {
		qDebug() << db->lastError();
		return false;
	}
	
#if 0
	//TODO "SHOW WARNINGS" does not work anymore!
	warnQuery   = new QSqlQuery( *db );
#endif
	symbolQuery = new QSqlQuery( *db );
	if( !symbolQuery->prepare(myProp->symbolQueryStrg.arg(myProp->ibSymbolTable)) ) {
		qDebug() << symbolQuery->lastError();
		return false;
	}
	return true;
}


void Worker::removeDb()
{
	if( symbolQuery != NULL ) {
		delete symbolQuery;
	}
	if( warnQuery != NULL ) {
		delete warnQuery;
	}
	if( db != NULL ) {
		delete db;
	}
	QSqlDatabase::removeDatabase("symbolData");
	
}


void Worker::error(int id, int errorCode, const QString &errorMsg)
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
			}
		}
		// TODO, handle:
		// 162 "Historical Market Data Service error message:HMDS query returned no data: DJX   100522C00155000@CBOE Bid"
		// 162 "Historical Market Data Service error message:No historical market data for CAC40/IND@MONEP Bid 1800"
		// 162 "Historical Market Data Service error message:No data of type EODChart is available for the exchange 'IDEAL' and the security type 'Forex' and '1 y' and '1 day'"
		// 200 "No security definition has been found for the request"
		// 162 "Historical Market Data Service error message:Historical data request pacing violation"
	} else {
		Q_ASSERT( id == -1 );
	}
}


void Worker::twsConnected( bool /*connected*/ )
{
	idleTimer->setInterval( 0 );
}


void Worker::contractDetails2Storage( int reqId, const IB::ContractDetails &ibContractDetails )
{
	
	if( currentReqId != reqId ) {
		qDebug() << "got reqId" << reqId << "but currentReqId:" << currentReqId;
		Q_ASSERT( false );
	}
	
	contractDetailsStorage.append(ibContractDetails);
}


void Worker::contractDetailsEnd( int reqId )
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



void Worker::historicalData( int reqId, const QString &date, double open, double high, double low,
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
		const IB::Contract &c = histRequests.at(curReqContractIndex).ibContract;
		const QString &wts = histRequests.at(curReqContractIndex).whatToShow;
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


int Worker::storage2DB()
{
	QTime  stopWatch;
	stopWatch.start();
	
	int countInserted = 0;
	int countUpdated  = 0;
	int countReceived = contractDetailsStorage.size();
	
	for( int i=0; i<countReceived; i++ ) {
		
		IB::ContractDetails *ibContractDetails = &contractDetailsStorage[i];
		
		symbolQuery->bindValue(":symbol"         , toQString(ibContractDetails->summary.symbol));
		symbolQuery->bindValue(":secType"        , toQString(ibContractDetails->summary.secType));
		symbolQuery->bindValue(":expiry"         , toQString(ibContractDetails->summary.expiry));
		symbolQuery->bindValue(":strike"         , ibContractDetails->summary.strike);
		symbolQuery->bindValue(":pc"             , toQString(ibContractDetails->summary.right));
		symbolQuery->bindValue(":exchange"       , toQString(ibContractDetails->summary.exchange));
		symbolQuery->bindValue(":currency"       , toQString(ibContractDetails->summary.currency));
		symbolQuery->bindValue(":localSymbol"    , toQString(ibContractDetails->summary.localSymbol));
		symbolQuery->bindValue(":marketName"     , toQString(ibContractDetails->marketName));
		symbolQuery->bindValue(":tradingClass"   , toQString(ibContractDetails->tradingClass));
		symbolQuery->bindValue(":conid"          , (int)ibContractDetails->summary.conId);
		symbolQuery->bindValue(":minTick"        , ibContractDetails->minTick);
		symbolQuery->bindValue(":multiplier"     , toQString(ibContractDetails->summary.multiplier));
		symbolQuery->bindValue(":priceMagnifier" , (int)ibContractDetails->priceMagnifier);
		symbolQuery->bindValue(":orderTypes"     , toQString(ibContractDetails->orderTypes));
		symbolQuery->bindValue(":validExchanges" , toQString(ibContractDetails->validExchanges));
		
		if( !symbolQuery->exec() ) {
			qDebug() << symbolQuery->lastError();
			return -1;
		} else {
			if( symbolQuery->numRowsAffected() == 1 ) {
				countInserted++;
			}
			if( symbolQuery->numRowsAffected() == 2 ) {
				countUpdated++;
			}
		}
		
//TODO "SHOW WARNINGS" does not work anymore!
#if 0
		if( !warnQuery->exec() ) {
			qDebug() << "warnQuery false" << warnQuery->lastError();
		} else {
			if( warnQuery->size() > 0 ) {
				qDebug() << toQString(ibContractDetails->validExchanges);
				qDebug() << toQString(ibContractDetails->orderTypes);
				qDebug() << "SQL warnings:" <<  warnQuery-> size();
				while (warnQuery->next()) {
					qDebug() << warnQuery->value(0).toString() << warnQuery->value(1).toString() << warnQuery->value(2).toString();
				}
			}
		}
#endif
	}
	qDebug() << QString(
		"Contracts received: %1, updated: %2, inserted: %3 (%4ms)")
		.arg(countReceived).arg(countUpdated).arg(countInserted)
		.arg(stopWatch.elapsed());
	
	return countInserted;
}


int Worker::storage2stdout()
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


Worker::State Worker::currentState() const
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
	useDB = false;
	ibSymbolTable   = "someTable";
	symbolQueryStrg = QString()
		+"INSERT INTO "
		+"`%1`(symbol, secType, expiry, strike, pc, exchange, currency, localSymbol, marketName, "
		+"tradingClass, conid, minTick, multiplier, priceMagnifier, orderTypes, validExchanges, "
		+"firstSeen, lastSeen) "
		+"VALUES(:symbol, :secType, :expiry, :strike, :pc, :exchange, :currency, :localSymbol, :marketName, "
		+":tradingClass, :conid, :minTick, :multiplier, :priceMagnifier, :orderTypes, :validExchanges, "
		+"CURRENT_DATE(), CURRENT_DATE()) "
		+"ON DUPLICATE KEY UPDATE lastSeen=CURRENT_DATE()";
	
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
	ok = ok & get("useDB",         useDB);
	ok = ok & get("ibSymbolTable", ibSymbolTable);
	
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
