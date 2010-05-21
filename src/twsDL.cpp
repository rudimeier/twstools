#include "twsDL.h"

#include "twsapi/twsUtil.h"
#include "twsapi/twsClient.h"
#include "twsapi/twsWrapper.h"
#include "utilities/debug.h"

// from global installed ibtws
#include "Contract.h"

#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>




namespace Test {


#define CONTRACT_COUNT 1
/// static work todo
const QString contractSpecs[CONTRACT_COUNT][3] = {
// 	{ "CAC40"  , "IND", "MONEP"    },
// 	{ "CAC40"  , "OPT", "MONEP"    },
// 	{ "DAX"    , "IND", "DTB"      },
// 	{ "DAX"    , "OPT", "DTB"      },
	{ "DJX"    , "IND", "CBOE"     }
// 	{ "DJX"    , "OPT", "CBOE"     },
// 	{ "ESTX50" , "IND", "DTB"      },
// 	{ "ESTX50" , "OPT", "DTB"      },
// 	{ "K200"   , "IND", "KSE"      },
// 	{ "K200"   , "OPT", "KSE"      },
// 	{ "NDX"    , "IND", "NASDAQ"   },
// 	{ "NDX"    , "OPT", "CBOE"     },
// 	{ "RUT"    , "IND", "CBOE"     },
// 	{ "RUT"    , "OPT", "CBOE"     },
// 	{ "SPX"    , "IND", "CBOE"     },
// 	{ "SPX"    , "OPT", "CBOE"     },
// 	{ "XEO"    , "IND", "CBOE"     },
// 	{ "XEO"    , "OPT", "CBOE"     },
// 	{ "Z"      , "IND", "LIFFE"    },
// 	{ "Z"      , "OPT", "LIFFE"    }
};


Worker::Worker() :
	state(START),
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
	ibContract.symbol = toIBString(contractSpecs[i][0]);
	ibContract.secType = toIBString(contractSpecs[i][1]);
	ibContract.exchange= toIBString(contractSpecs[i][2]);
	
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
	
	foreach( IB::ContractDetails cd,  contractDetailsStorage ) {
		rememberContracts.append( cd.summary );
	}
	
	contractDetailsStorage.clear();
	if( inserted == -1 ) {
		state = QUIT_ERROR;
		return;
	}
	countNewContracts += inserted;
	
	
	currentReqId++;
	if( currentReqId < CONTRACT_COUNT ) {
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
	qDebug() << "DOWNLOAD DATA";
	/*TODO request Data*/
	qDebug() << toQString(rememberContracts.first().symbol);
	
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


void Worker::finData()
{
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
	prop.readConfigFile("twsDL.cfg");
	
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
	
	// connecting some TWS signals to this
	connect ( twsClient, SIGNAL(connected(bool)),
		 this,SLOT(twsConnected(bool)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(contractDetails(int, IB::ContractDetails)),
		 this,SLOT(contractDetails2Storage(int, IB::ContractDetails)), Qt::QueuedConnection );
	connect ( twsClient, SIGNAL(contractDetailsEnd(int)),
		 this,SLOT(contractDetailsEnd(int)), Qt::QueuedConnection );
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
	
	downloadData = false;
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
	
	ok = ok & get("downloadData", downloadData);
	
	return ok;
}
///////////////////////////////////////////////////////////////////////////////


} // namespace Test
