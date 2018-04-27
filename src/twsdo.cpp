/*** twsdo.cpp -- TWS job processing tool
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "twsdo.h"
#include "tws_meta.h"
#include "tws_query.h"
#include "tws_util.h"
#include "tws_client.h"
#include "tws_wrapper.h"
#include "tws_account.h"
#include "debug.h"

#include <twsapi/twsapi_config.h>
#include <twsapi/Contract.h>
#if ! defined TWS_ORIG_CLIENT
# include <twsapi/EPosixClientSocket.h>
#else
# include <twsapi/EClientSocket.h>
#endif

#if defined HAVE_CONFIG_H
# include "config.h"
#endif  /* HAVE_CONFIG_H */
#include "dso_magic.h"

#include <stdio.h>
#include <string.h>

#if defined _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#if TWSAPI_IB_VERSION_NUMBER < 97200
# define lastTradeDateOrContractMonth expiry
#endif

ConfigTwsdo::ConfigTwsdo()
{
	workfile = NULL;
	skipdef = 0;
	tws_host = "localhost";
	tws_port = 7474;
	tws_client_id = 123;
	ai_family = AF_UNSPEC;

	get_account = 0;
	tws_account_name = "";
	get_exec = 0;
	get_order = 0;

	mkt_data_type = 3; /* delayed */
	tws_conTimeout = 30000;
	tws_reqTimeout = 1200000;
	tws_maxRequests = 60;
	tws_pacingInterval = 605000;
	tws_minPacingTime = 1000;
	tws_violationPause = 60000;

	strat_file = NULL;
}

void ConfigTwsdo::init_ai_family( int ipv4, int ipv6 )
{
	if( !ipv4 && !ipv6 ) {
		/* default is ipv4 only */
		ai_family = AF_INET;
		return;
	}

	if( ipv4 && ipv6 ) {
		/* this will be the default one day */
		ai_family = AF_UNSPEC;
	} else if( ipv4 ) {
		ai_family = AF_INET;
	} else if( ipv6 ) {
		ai_family = AF_INET6;
	} else {
		/* no more cases */
		assert(false);
	}
}

void ConfigTwsdo::init_mkt_data_type(const char *str)
{
	if (!strcasecmp(str,"none"))
		mkt_data_type = 0;
	if (!strcasecmp(str,"real-time"))
		mkt_data_type = 1;
	else if (!strcasecmp(str,"frozen"))
		mkt_data_type = 2;
	else if (!strcasecmp(str,"delayed"))
		mkt_data_type = 3;
	else if (!strcasecmp(str,"delayed-frozen"))
		mkt_data_type = 4;
	else {
		char *end;
		long val = strtol(str, &end, 10);
		/* we allow all integers ... for testing */
		if( end == str || *end != '\0' || val < INT_MIN || val > INT_MAX ) {
			fprintf( stderr, "error, invalid market data type '%s'\n", str);
			exit(2);
		}
		mkt_data_type = val;
	}
}

struct RateLimit
{
	bool can_send(int n = 1)
	{
		uint64_t now = nowInMsecs() / m_interval;
		if (m_time < now) {
			m_time = now;
			m_count = n;
			return true;
		} else if ( m_count + n > m_rate ) {
			return false;
		} else {
			m_count += n;;
			return true;
		}
	};

	const int m_interval = 500;
	const int m_rate = 25;
	int m_count = 0;
	uint64_t m_time = 0;
};


class TwsDlWrapper : public DebugTwsWrapper
{
	public:
		TwsDlWrapper( TwsDL* parent );
		virtual ~TwsDlWrapper();

		void connectionClosed();

#if TWSAPI_VERSION_NUMBER >= 17300
		void error( int id, int errorCode,
			const IBString& errorString );
#else
		void error( const int id, const int errorCode,
			const IBString errorString );
#endif
		void contractDetails( int reqId,
			const ContractDetails& contractDetails );
		void bondContractDetails( int reqId,
			const ContractDetails& contractDetails );
		void contractDetailsEnd( int reqId );
#if TWSAPI_IB_VERSION_NUMBER >= 97300
# if TWSAPI_VERSION_NUMBER >= 17300
		void historicalData(TickerId reqId, const Bar& bar);
		void historicalDataEnd(int reqId, const IBString& startDateStr,
			const IBString& endDateStr);
# else
		void historicalData(TickerId reqId, Bar bar);
		void historicalDataEnd(int reqId, IBString startDateStr,
			IBString endDateStr);
# endif
#else
		void historicalData( TickerId reqId, const IBString& date,
			double open, double high, double low, double close, int volume,
			int barCount, double WAP, int hasGaps );
#endif
		void updateAccountValue( const std::string& key,
			const IBString& val, const std::string& currency,
			const IBString& accountName );
		void updatePortfolio( const Contract& contract, int position,
			double marketPrice, double marketValue, double averageCost,
			double unrealizedPNL, double realizedPNL,
			const IBString& accountName);
		void updateAccountTime( const IBString& timeStamp );
		void accountDownloadEnd( const IBString& accountName );
		void execDetails( int reqId, const Contract& contract,
			const Execution& execution );
		void execDetailsEnd( int reqId );
		void orderStatus( OrderId orderId, const IBString &status,
			int filled, int remaining, double avgFillPrice, int permId,
			int parentId, double lastFillPrice, int clientId,
			const IBString& whyHeld
#if TWSAPI_IB_VERSION_NUMBER >= 97300
			, double mktCapPrice
#endif
			);
		void openOrder( OrderId orderId, const Contract&,
			const Order&, const OrderState& );
		void openOrderEnd();
		void currentTime( long time );
		void nextValidId( OrderId orderId );
		void tickPrice( TickerId reqId, TickType field, double price,
#if TWSAPI_IB_VERSION_NUMBER >= 97300
			const TickAttrib& );
#else
			int canAutoExecute );
#endif
		void tickSize( TickerId reqId, TickType field, int size );
		void connectAck();

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


#if TWSAPI_VERSION_NUMBER >= 17300
void TwsDlWrapper::error( int id, int errorCode,
	const IBString& errorString )
#else
void TwsDlWrapper::error( const int id, const int errorCode,
	const IBString errorString )
#endif
{
	RowError row = { id, errorCode, errorString };
	parentTwsDL->twsError( row );
}


void TwsDlWrapper::contractDetails( int reqId,
	const ContractDetails& contractDetails )
{
	parentTwsDL->twsContractDetails(
		reqId, contractDetails );
}


void TwsDlWrapper::bondContractDetails( int reqId,
	const ContractDetails& contractDetails )
{
	parentTwsDL->twsBondContractDetails(
		reqId, contractDetails );
}


void TwsDlWrapper::contractDetailsEnd( int reqId )
{
	parentTwsDL->twsContractDetailsEnd(
		reqId);
}

#if TWSAPI_IB_VERSION_NUMBER >= 97300
# if TWSAPI_VERSION_NUMBER >= 17300
void TwsDlWrapper::historicalData(TickerId reqId, const Bar& bar)
# else
void TwsDlWrapper::historicalData(TickerId reqId, Bar bar)
# endif
{
	/* TODO remove RowHist and use Bar directly */
	RowHist row = { bar.time, bar.open, bar.high, bar.low, bar.close, bar.volume, bar.count, bar.wap, false };
	parentTwsDL->twsHistoricalData( reqId, row );
}
# if TWSAPI_IB_VERSION_NUMBER >= 97300
void TwsDlWrapper::historicalDataEnd(int reqId, const IBString& startDateStr,
	const IBString& endDateStr)
# else
void TwsDlWrapper::historicalDataEnd(int reqId, IBString startDateStr,
	IBString endDateStr)
# endif
{
	RowHist row = dflt_RowHist;
	row.date = IBString("finished-") + startDateStr + "-" + endDateStr;
	parentTwsDL->twsHistoricalData( reqId, row );
}
#else
void TwsDlWrapper::historicalData( TickerId reqId, const IBString& date,
	double open, double high, double low, double close, int volume,
	int count, double WAP, int hasGaps )
{
	RowHist row = { date, open, high, low, close, volume, count, WAP, (bool)hasGaps };
	parentTwsDL->twsHistoricalData( reqId, row );
}
#endif

void TwsDlWrapper::updateAccountValue( const IBString& key,
	const IBString& val, const IBString& currency,
	const IBString& accountName )
{
	RowAccVal row = { key, val, currency, accountName };
	parentTwsDL->twsUpdateAccountValue( row );
}

void TwsDlWrapper::updatePortfolio( const Contract& contract,
	int position, double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const IBString& accountName)
{
	RowPrtfl row = { contract, position, marketPrice, marketValue, averageCost,
		unrealizedPNL, realizedPNL, accountName};
	parentTwsDL->twsUpdatePortfolio( row );
}

void TwsDlWrapper::updateAccountTime( const IBString& timeStamp )
{
	parentTwsDL->twsUpdateAccountTime( timeStamp );
}

void TwsDlWrapper::accountDownloadEnd( const IBString& accountName )
{
	DebugTwsWrapper::accountDownloadEnd( accountName );
	parentTwsDL->twsAccountDownloadEnd( accountName );
}

void TwsDlWrapper::execDetails( int reqId, const Contract& contract,
	const Execution& execution )
{
	DebugTwsWrapper::execDetails(reqId, contract, execution);
	RowExecution row = { contract, execution };
	parentTwsDL->twsExecDetails(  reqId, row );
}

void TwsDlWrapper::execDetailsEnd( int reqId )
{
	DebugTwsWrapper::execDetailsEnd( reqId );
	parentTwsDL->twsExecDetailsEnd( reqId );
}

void TwsDlWrapper::orderStatus( OrderId orderId, const IBString &status,
	int filled, int remaining, double avgFillPrice, int permId,
	int parentId, double lastFillPrice, int clientId,
	const IBString& whyHeld
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	, double mktCapPrice
#endif
	)
{
	DebugTwsWrapper::orderStatus( orderId, status, filled, remaining,
		avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld
#if TWSAPI_IB_VERSION_NUMBER >= 97300
		, mktCapPrice
#endif
		);
	RowOrderStatus row = { orderId, status, filled, remaining, avgFillPrice,
		permId, parentId, lastFillPrice, clientId, whyHeld };
	parentTwsDL->twsOrderStatus(row);
}

void TwsDlWrapper::openOrder( OrderId orderId, const Contract& c,
	const Order& o, const OrderState& os)
{
	DebugTwsWrapper::openOrder(orderId, c, o, os);
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

void TwsDlWrapper::nextValidId( OrderId orderId )
{
	DebugTwsWrapper::nextValidId(orderId);
	parentTwsDL->nextValidId( orderId );
}

void TwsDlWrapper::tickPrice( TickerId reqId, TickType field, double price,
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	const TickAttrib& ta)
#else
	int canAutoExecute)
#endif
{
#if TWSAPI_IB_VERSION_NUMBER >= 97300
	int canAutoExecute = ta.canAutoExecute;
#endif
	parentTwsDL->twsTickPrice( reqId, field, price, canAutoExecute );
}

void TwsDlWrapper::tickSize( TickerId reqId, TickType field, int size )
{
	parentTwsDL->twsTickSize( reqId, field, size );
}

void TwsDlWrapper::connectAck()
{
#if TWSAPI_IB_VERSION_NUMBER >= 97200
	DebugTwsWrapper::connectAck();
#endif
	parentTwsDL->twsConnectAck();
}

struct TwsHeartBeat
{
	TwsHeartBeat();
	void reset();

	int cnt_sent;
	int cnt_recv;
	int64_t time_sent;
	int64_t time_recv;
	long tws_time;
};

TwsHeartBeat::TwsHeartBeat() :
	cnt_sent(0),
	cnt_recv(0),
	time_sent(0),
	time_recv(0),
	tws_time(0)
{
}

void TwsHeartBeat::reset()
{
	cnt_sent = 0;
	cnt_recv = 0;
	time_sent = 0;
	time_recv = 0;
	tws_time = 0;
}


TwsDL::TwsDL() :
	state(IDLE),
	quit(false),
	error(0),
	lastConnectionTime(0),
	tws_hb(new TwsHeartBeat()),
	tws_valid_orderId(0),
	connectivity_IB_TWS(false),
	curIdleTime(0),
	twsWrapper( new TwsDlWrapper(this) ),
	twsClient( new TWSClient(twsWrapper) ),
	rate_limit(new RateLimit()),
	msgCounter(0),
	currentRequest(  *(new GenericRequest()) ),
	workTodo( new WorkTodo() ),
	account( new Account ),
	quotes( new Quotes ),
	packet( NULL ),
	dataFarms( *(new DataFarmStates()) ),
	pacingControl( *(new PacingGod(dataFarms)) ),
	strat(NULL)
{
}


TwsDL::~TwsDL()
{
	delete &currentRequest;
	delete tws_hb;
	delete rate_limit;

	if( twsClient != NULL ) {
		delete twsClient;
	}
	if( twsWrapper != NULL ) {
		delete twsWrapper;
	}
	if( workTodo != NULL ) {
		delete workTodo;
	}
	if( account != NULL ) {
		delete account;
	}
	if( quotes != NULL ) {
		delete quotes;
	}
	if( packet != NULL ) {
		delete packet;
	}

	if( strat != NULL ) {
		close_dso( strat, this );
	}

	delete &pacingControl;
	delete &dataFarms;
}


int TwsDL::setup( const ConfigTwsdo &c )
{
	cfg = c;

	pacingControl.setPacingTime( cfg.tws_maxRequests,
		cfg.tws_pacingInterval, cfg.tws_minPacingTime );
	pacingControl.setViolationPause( cfg.tws_violationPause );

	// try loading DSOs before anything else
	if( cfg.strat_file ) {
		// for the moment we assume that the lt's load path is
		// set up correctly or that the user has given an
		// absolute file, if not just do fuckall
		if( (strat = open_dso( cfg.strat_file, this )) == NULL ) {
			return -1;
		}
	}

	if( initWork() < 0 ) {
		return -1;
	}
	return 0;
}

int TwsDL::start()
{
	assert( state == IDLE );

	quit = false;
	curIdleTime = 0;
	eventLoop();
	return error;
}


void TwsDL::eventLoop()
{
	while( !quit ) {
		if( curIdleTime > 0 ) {
			twsClient->selectStuff( curIdleTime );
		}
		/* We want to set the default select timeout if nobody will change it
		   later. TODO do better */
		curIdleTime = -1;
		switch( state ) {
			case WAIT_TWS_CON:
				waitTwsCon();
				break;
			case IDLE:
				idle();
				break;
		}
		if( curIdleTime == -1 ) {
			curIdleTime = 50;
		}
	}
}


void TwsDL::connectTws()
{
	assert( !twsClient->isConnected() && !connectivity_IB_TWS );

	int64_t w = nowInMsecs() - lastConnectionTime;
	if( w < cfg.tws_conTimeout ) {
		DEBUG_PRINTF( "Waiting %ldms before connecting again.",
			(long)(cfg.tws_conTimeout - w) );
		curIdleTime = cfg.tws_conTimeout - w;
		return;
	}

	lastConnectionTime = nowInMsecs();
	changeState( WAIT_TWS_CON );

	/* this may callback (negative) error messages only */
	bool con = twsClient->connectTWS( cfg.tws_host, cfg.tws_port,
		cfg.tws_client_id, cfg.ai_family );
	assert( con == twsClient->isConnected() );

	if( !con ) {
		DEBUG_PRINTF("TWS connection failed.");
		changeState(IDLE);
#if TWSAPI_IB_VERSION_NUMBER >= 97200
	} else if (!twsClient->ePosixClient->asyncEConnect()) {
#else
	} else {
#endif
		DEBUG_PRINTF("TWS connection established: %d, %s",
			twsClient->serverVersion(), twsClient->TwsConnectionTime().c_str());
		/* this must be set before any possible "Connectivity" callback msg */
		connectivity_IB_TWS = true;
		/* waiting for first messages until tws_time is received */
		tws_hb->reset();
		tws_hb->cnt_sent = 1;
		tws_hb->time_sent = nowInMsecs();
		rate_limit->can_send(4);
		if (cfg.mkt_data_type)
			twsClient->reqMarketDataType(cfg.mkt_data_type);
		twsClient->reqCurrentTime();
	}
}

void TwsDL::twsConnectAck()
{
#if TWSAPI_IB_VERSION_NUMBER >= 97200
	if (twsClient->ePosixClient->asyncEConnect()) {
		DEBUG_PRINTF("TWS connection established (async): %d, %s",
			twsClient->serverVersion(), twsClient->TwsConnectionTime().c_str());
		/* this must be set before any possible "Connectivity" callback msg */
		connectivity_IB_TWS = true;
		/* waiting for first messages until tws_time is received */
		tws_hb->reset();
		tws_hb->cnt_sent = 1;
		tws_hb->time_sent = nowInMsecs();
		rate_limit->can_send(4);
		twsClient->ePosixClient->startApi();
		if (cfg.mkt_data_type)
			twsClient->reqMarketDataType(cfg.mkt_data_type);
		twsClient->reqCurrentTime();
	}
#endif
}

void TwsDL::waitTwsCon()
{
	int64_t w = cfg.tws_conTimeout - (nowInMsecs() - lastConnectionTime);

	if( twsClient->isConnected() ) {
		if( tws_hb->tws_time != 0 ) {
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
	waitData();
	if( quit || currentRequest.reqType() != GenericRequest::NONE ) {
		return;
	}

	if( !twsClient->isConnected() ) {
		connectTws();
		return;
	}

	// HACK
	static int fuckme = 0;
	if( fuckme <= 0 ) {
		fuckme = reqMktData();
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
		/* TODO for now we place all orders when nothing else todo */
		if( strat != NULL ) {
			work_dso( strat, this );
		}
		placeAllOrders();
		break;
	}

	if( reqType == GenericRequest::NONE && fuckme <= 1
		&& workTodo->placeOrderTodo()->countLeft() <= 0 && p_orders.empty() ) {
		_lastError = "No more work to do.";
		quit = true;
	}
}


void TwsDL::waitData()
{
	finPlaceOrder();
	if( packet == NULL || currentRequest.reqType() == GenericRequest::NONE ) {
		return;
	}

	if( !packet->finished() ) {
		if( currentRequest.age() <= cfg.tws_reqTimeout ) {
			static int64_t last = 0;
			int64_t now = nowInMsecs();
			if( now - last > 2000 ) {
				last = now;
				DEBUG_PRINTF( "Still waiting for data." );
			}
			return;
		} else {
			DEBUG_PRINTF( "Timeout waiting for data." );
			packet->closeError( REQ_ERR_TIMEOUT );
		}
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
	if( !ok ) {
		error = 1;
		_lastError = "Fatal error.";
		quit = true;
	}
	delete packet;
	packet = NULL;
	currentRequest.close();
}


bool TwsDL::finContracts()
{
	PacketContractDetails* p = (PacketContractDetails*)packet;
	switch( packet->getError() ) {
	case REQ_ERR_NONE:
		DEBUG_PRINTF( "Contracts received: %lu",
			(unsigned long) p->constList().size() );
		packet->dumpXml();
	case REQ_ERR_NODATA:
	case REQ_ERR_NAV:
	case REQ_ERR_REQUEST:
		break;
	case REQ_ERR_TWSCON:
	case REQ_ERR_TIMEOUT:
		return false;
	}

	if( strat != NULL ) {
		// HACK we want exactly one ContractDetails for each mkt data contract
		assert( p->constList().size() == 1 );
		const ContractDetails &cd = p->constList().at(0);
		con_details[cd.summary.conId] =  new ContractDetails(cd);

		// HACK add conId to mkt data contracts
		int mi = con_details.size() - 1;
		const MktDataTodo &mtodo = workTodo->getMktDataTodo();
		Contract &contract = mtodo.mktDataRequests[mi].ibContract;
		assert( contract.conId == 0 || contract.conId == cd.summary.conId );
		contract.conId = cd.summary.conId;
	}

	return true;
}


bool TwsDL::finHist()
{
	HistTodo *histTodo = workTodo->histTodo();

	switch( packet->getError() ) {
	case REQ_ERR_NONE:
		packet->dumpXml();
	case REQ_ERR_NODATA:
	case REQ_ERR_NAV:
		histTodo->tellDone();
		break;
	case REQ_ERR_TWSCON:
		histTodo->cancelForRepeat(0);
		break;
	case REQ_ERR_TIMEOUT:
		histTodo->cancelForRepeat(1);
		break;
	case REQ_ERR_REQUEST:
		histTodo->cancelForRepeat(2);
		break;
	}
	return true;
}


bool TwsDL::finPlaceOrder()
{
	bool ok = true;
	std::map<long, PacketPlaceOrder*>::iterator it = p_orders.begin();
	while( it != p_orders.end() ) {
		/* increment it already here and use it_tmp because erase would
		   invalidate it */
		std::map<long, PacketPlaceOrder*>::iterator it_tmp = it++;
		long orderId = it_tmp->first;
		PacketPlaceOrder* p = it_tmp->second;
		const PlaceOrder &r = p->getRequest();

		assert( orderId == r.orderId );
		if( ! p->finished() ) {
			/* close non transmit orders where we haven't received errors */
			if( !r.order.transmit && (nowInMsecs() - r.time_sent) > 5000 ) {
				p->closeError( REQ_ERR_NONE );
			} else {
				continue;
			}
		}

		switch( p->getError() ) {
		case REQ_ERR_NONE:
		case REQ_ERR_REQUEST:
		case REQ_ERR_TIMEOUT:
			p->dumpXml();
			DEBUG_PRINTF("fin order, %ld %s, %ld", orderId,
				r.contract.symbol.c_str(), r.contract.conId);
			assert( p_orders_old.find(orderId) == p_orders_old.end() );
			p_orders_old[orderId] = p;
			p_orders.erase( it_tmp );
		case REQ_ERR_NODATA:
		case REQ_ERR_NAV:
			break;
		case REQ_ERR_TWSCON:
			ok = false;
		}
	}
	return ok;
}


#define ERR_MATCH( _strg_  ) \
	( err.msg.find(_strg_) != std::string::npos )

void TwsDL::twsError( const RowError& err )
{
	msgCounter++;

	if( !twsClient->isConnected() ) {
		switch( err.code ) {
		default:
		case 504: /* NOT_CONNECTED */
			DEBUG_PRINTF( "fatal: %d %d %s", err.id, err.code, err.msg.c_str());
			assert(false);
			break;
		case 503: /* UPDATE_TWS */
			DEBUG_PRINTF( "error: %s", err.msg.c_str() );
			break;
		case 502: /* CONNECT_FAIL */
			DEBUG_PRINTF( "connection failed: %s", err.msg.c_str());
			break;
		}
		return;
	}

	if( err.id == currentRequest.reqId() ) {
		DEBUG_PRINTF( "TWS message for request %d: %d '%s'",
			err.id, err.code, err.msg.c_str() );
		switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
				errorContracts( err );
				break;
			case GenericRequest::HIST_REQUEST:
				errorHistData( err );
				break;
			case GenericRequest::ACC_STATUS_REQUEST:
			case GenericRequest::EXECUTIONS_REQUEST:
			case GenericRequest::ORDERS_REQUEST:
				assert( false );
				break;
			case GenericRequest::NONE:
				DEBUG_PRINTF( "Warning, got message for closed request %d.",
					err.id );
				break;
		}
		return;
	} else {
		errorPlaceOrder( err );
	}

	if( err.id != -1 ) {
		DEBUG_PRINTF( "TWS message for unexpected request %d: %d '%s'",
			err.id, err.code, err.msg.c_str() );
		return;
	}

	DEBUG_PRINTF( "TWS message generic: %d %s", err.code, err.msg.c_str() );

	// TODO do better
	switch( err.code ) {
		case 1100:
			assert(ERR_MATCH("Connectivity between IB and T"));
			assert(ERR_MATCH(" has been lost."));
			connectivity_IB_TWS = false;
			curIdleTime = cfg.tws_reqTimeout;
			break;
		case 1101:
			assert(ERR_MATCH("Connectivity between IB and T"));
			assert(ERR_MATCH(" has been restored - data lost."));
			connectivity_IB_TWS = true;
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				packet->closeError( REQ_ERR_TWSCON );
			}
			break;
		case 1102:
			assert(ERR_MATCH("Connectivity between IB and T"));
			assert(ERR_MATCH(" has been restored - data maintained."));
			connectivity_IB_TWS = true;
			if( currentRequest.reqType() == GenericRequest::HIST_REQUEST ) {
				packet->closeError( REQ_ERR_TWSCON );
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
			dataFarms.notify( msgCounter, err.code, err.msg );
			pacingControl.clear();
			break;
		case 2110:
			/* looks like we get that only on fresh connection */
			assert(ERR_MATCH("Connectivity between T"));
			assert(ERR_MATCH(" and server is broken. It will be restored automatically."));
			connectivity_IB_TWS = false;
			break;
	}
}


void TwsDL::errorContracts( const RowError& err )
{
	// TODO
	switch( err.code ) {
	case 200:
		/* "No security definition has been found for the request" */
		if( connectivity_IB_TWS ) {
			packet->closeError( REQ_ERR_REQUEST );
		} else {
			/* using ERR_TIMEOUT instead ERR_TWSCON to push_back this request */
			packet->closeError( REQ_ERR_TIMEOUT );
		}
		break;
	case 321:
		/* comes directly from TWS with prefix "Error validating request:-" */
		packet->closeError( REQ_ERR_REQUEST );
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	}
}


void TwsDL::errorHistData( const RowError& err )
{
	const Contract &curContract = workTodo->getHistTodo().current().ibContract;
	const HistRequest *cur_hR = &workTodo->getHistTodo().current();
	PacketHistData &p_histData = *((PacketHistData*)packet);
	switch( err.code ) {
	// Historical Market Data Service error message:
	case 162:
		if( ERR_MATCH("Historical data request pacing violation") ) {
			p_histData.closeError( REQ_ERR_TWSCON );
			pacingControl.notifyViolation( curContract );
		} else if( ERR_MATCH("HMDS query returned no data:") ) {
			DEBUG_PRINTF( "READY - NO DATA %p %d", cur_hR, err.id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_NODATA );
		} else if( ERR_MATCH("No historical market data for") ) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE on HMDS server. "
				"%p %d", cur_hR, err.id );
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_NAV );
		} else if( ERR_MATCH("No data of type EODChart is available") ||
			ERR_MATCH("No data of type DayChart is available") ||
			ERR_MATCH("BEST queries are not supported for this contract")) {
			// NOTE we should skip all similar work intelligently
			DEBUG_PRINTF( "WARNING - DATA IS NOT AVAILABLE (no HMDS route). "
				"%p %d", cur_hR, err.id );
			p_histData.closeError( REQ_ERR_NAV );
			workTodo->histTodo()->skip_by_nodata(*cur_hR);
			pacingControl.remove_last_request( curContract );
		} else if( ERR_MATCH("No market data permissions for") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_REQUEST );
			workTodo->histTodo()->skip_by_perm(curContract);
			pacingControl.remove_last_request( curContract );
		} else if( ERR_MATCH("Unknown contract") ) {
			// NOTE we should skip all similar work intelligently
			dataFarms.learnHmds( curContract );
			p_histData.closeError( REQ_ERR_REQUEST );
		} else {
			DEBUG_PRINTF( "Warning, unhandled error message." );
			// seen: "TWS exited during processing of HMDS query"
		}
		break;
	// Historical Market Data Service query message:
	case 165:
		if( ERR_MATCH("HMDS server disconnect occurred.  Attempting reconnection") ||
		    ERR_MATCH("HMDS connection attempt failed.  Connection will be re-attempted") ) {
			curIdleTime = cfg.tws_reqTimeout;
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
			p_histData.closeError( REQ_ERR_REQUEST );
		} else {
			/* using ERR_TIMEOUT instead ERR_TWSCON to push_back this request */
			p_histData.closeError( REQ_ERR_TIMEOUT );
		}
		break;
	// Order rejected - Reason:
	case 201:
	// Order cancelled - Reason:
	case 202:
		DEBUG_PRINTF( "Warning, unexpected error code." );
		/* TODO "Order cancelled", this is expected here and we'll wait for some
		   more orderStatus callbacks */
		break;
	// The security <security> is not available or allowed for this account
	case 203:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	// Server error when validating an API client request
	case 321:
		// comes directly from TWS with prefix "Error validating request:-"
		// NOTE we could find out more to throw away similar worktodo
		p_histData.closeError( REQ_ERR_REQUEST );
		break;
	default:
		DEBUG_PRINTF( "Warning, unhandled error code." );
		break;
	}
}


void TwsDL::errorPlaceOrder( const RowError& err )
{
	if( p_orders.find(err.id) != p_orders.end() ) {
		assert( p_orders_old.find(err.id) == p_orders_old.end() );
		PacketPlaceOrder *p_pO = p_orders[err.id];
		if( p_pO->finished() ) {
			DEBUG_PRINTF("Warning, got openOrder callback for closed order.");
		}
		p_pO->append(err);

		switch( err.code ) {
		// Unable to modify this order as its still being processed.
		case 2102:
			break;
		default:
			p_pO->closeError( REQ_ERR_REQUEST );
		}
		return;
	} else if( p_orders_old.find(err.id) != p_orders_old.end() ) {
		assert( p_orders.find(err.id) == p_orders.end() );
		DEBUG_PRINTF("Warning, got openOrder callback for finished order.");
		PacketPlaceOrder *p_pO = p_orders_old[err.id];
		p_pO->append(err);
		return;
	}
}


#undef ERR_MATCH


void TwsDL::twsConnectionClosed()
{
	DEBUG_PRINTF( "disconnected in state %d", state );

	if( currentRequest.reqType() != GenericRequest::NONE ) {
		if( !packet->finished() ) {
			switch( currentRequest.reqType() ) {
			case GenericRequest::CONTRACT_DETAILS_REQUEST:
			case GenericRequest::ACC_STATUS_REQUEST:
			case GenericRequest::EXECUTIONS_REQUEST:
			case GenericRequest::ORDERS_REQUEST:
				assert(false); // TODO repeat
				break;
			case GenericRequest::HIST_REQUEST:
				packet->closeError( REQ_ERR_TWSCON );
				break;
			case GenericRequest::NONE:
				assert(false);
				break;
			}
		}
	}
	assert( p_orders.empty() ); // TODO repeat

	connectivity_IB_TWS = false;
	dataFarms.setAllBroken();
	pacingControl.clear();
	/* avoid re-connect right now */
	lastConnectionTime = nowInMsecs();
}


void TwsDL::twsContractDetails( int reqId, const ContractDetails &ibContractDetails )
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


void TwsDL::twsBondContractDetails( int reqId, const ContractDetails &ibContractDetails )
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


void TwsDL::twsHistoricalData( int reqId, const RowHist &row )
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
	dataFarms.learnHmds( workTodo->getHistTodo().current().ibContract );

	assert( !packet->finished() );
	((PacketHistData*)packet)->append( reqId, row );

	if( packet->finished() ) {
		DEBUG_PRINTF( "READY %p %d",
			&workTodo->getHistTodo().current(), reqId );
	}
}

void TwsDL::twsUpdateAccountValue( const RowAccVal& row )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (updateAccountValue).");
		return;
	}
	((PacketAccStatus*)packet)->append( row );
}

void TwsDL::twsUpdatePortfolio( const RowPrtfl& row )
{
	account->updatePortfolio(row);

	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (updatePortfolio).");
		return;
	}
	((PacketAccStatus*)packet)->append( row );
}

void TwsDL::twsUpdateAccountTime( const std::string& timeStamp )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (updateAccountTime).");
		return;
	}
	((PacketAccStatus*)packet)->appendUpdateAccountTime( timeStamp );
}

void TwsDL::twsAccountDownloadEnd( const std::string& accountName )
{
	if( currentRequest.reqType() != GenericRequest::ACC_STATUS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (accountDownloadEnd).");
		return;
	}
	((PacketAccStatus*)packet)->appendAccountDownloadEnd( accountName );
}

void TwsDL::twsExecDetails( int reqId, const RowExecution &row )
{
	if( currentRequest.reqType() != GenericRequest::EXECUTIONS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (execDetails).");
		return;
	}
	((PacketExecutions*)packet)->append( reqId, row );
}

void TwsDL::twsExecDetailsEnd( int reqId )
{
	if( currentRequest.reqType() != GenericRequest::EXECUTIONS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (execDetailsEnd).");
		return;
	}
	((PacketExecutions*)packet)->appendExecutionsEnd( reqId );
}

void TwsDL::twsOrderStatus( const RowOrderStatus& row )
{
	account->update_os(row);

	if( currentRequest.reqType() == GenericRequest::ORDERS_REQUEST ) {
		((PacketOrders*)packet)->append(row);
		return;
	}
	if( p_orders.find(row.id) != p_orders.end() ) {
		assert( p_orders_old.find(row.id) == p_orders_old.end() );
		PacketPlaceOrder *p_pO = p_orders[row.id];
		if( p_pO->finished() ) {
			DEBUG_PRINTF("Warning, got orderStatus callback for closed order.");
		}
		p_pO->append(row);
		return;
	} else if( p_orders_old.find(row.id) != p_orders_old.end() ) {
		assert( p_orders.find(row.id) == p_orders.end() );
		DEBUG_PRINTF("Warning, got orderStatus callback for finished order.");
		PacketPlaceOrder *p_pO = p_orders_old[row.id];
		p_pO->append(row);
		return;
	}
	DEBUG_PRINTF( "Warning, unexpected tws callback (orderStatus).");
}

void TwsDL::twsOpenOrder( const RowOpenOrder& row )
{
	account->update_oo(row);

	if( currentRequest.reqType() == GenericRequest::ORDERS_REQUEST ) {
		((PacketOrders*)packet)->append(row);
		return;
	}
	if( p_orders.find(row.orderId) != p_orders.end() ) {
		assert( p_orders_old.find(row.orderId) == p_orders_old.end() );
		PacketPlaceOrder *p_pO = p_orders[row.orderId];
		if( p_pO->finished() ) {
			DEBUG_PRINTF("Warning, got openOrder callback for closed order.");
		}
		p_pO->append(row);
		return;
	} else if( p_orders_old.find(row.orderId) != p_orders_old.end() ) {
		assert( p_orders.find(row.orderId) == p_orders.end() );
		DEBUG_PRINTF("Warning, got openOrder callback for finished order.");
		PacketPlaceOrder *p_pO = p_orders_old[row.orderId];
		p_pO->append(row);
		return;
	}
	DEBUG_PRINTF( "Warning, unexpected tws callback (openOrder).");
}

void TwsDL::twsOpenOrderEnd()
{
	/* this messages usually comes unexpected right after connecting */

	if( currentRequest.reqType() != GenericRequest::ORDERS_REQUEST ) {
		DEBUG_PRINTF( "Warning, unexpected tws callback (openOrderEnd).");
		return;
	}

	((PacketOrders*)packet)->appendOpenOrderEnd();
}

void TwsDL::twsCurrentTime( long time )
{
	tws_hb->cnt_recv++;
	tws_hb->time_recv = nowInMsecs();
	tws_hb->tws_time = time;
}

void TwsDL::nextValidId( long orderId )
{
	if( state == WAIT_TWS_CON ) {
		tws_valid_orderId = orderId;
	}
}

void TwsDL::twsTickPrice( int reqId, TickType field, double price,
	int canAutoExecute )
{
	Quote &q = quotes->at(reqId-1);
	q.val[field] = price;
	q.stamp[field] = nowInMsecs();

	const std::vector<MktDataRequest> &mdlist
		= workTodo->getMktDataTodo().mktDataRequests;
	assert( reqId > 0 && reqId <= (int)mdlist.size() );

	const Contract &c
		= workTodo->getMktDataTodo().mktDataRequests[reqId - 1].ibContract;
	DEBUG_PRINTF( "TICK_PRICE: %d %s %ld %s %g", reqId,
		c.symbol.c_str(), c.conId, ibToString(field).c_str(), price );
}

void TwsDL::twsTickSize( int reqId, TickType field, int size )
{
	Quote &q = quotes->at(reqId-1);
	q.val[field] = size;
	q.stamp[field] = nowInMsecs();

	const std::vector<MktDataRequest> &mdlist
		= workTodo->getMktDataTodo().mktDataRequests;
	assert( reqId > 0 && reqId <= (int)mdlist.size() );

	const Contract &c
		= workTodo->getMktDataTodo().mktDataRequests[reqId - 1].ibContract;
	DEBUG_PRINTF( "TICK_SIZE: %d %s %ld %s %d", reqId,
		c.symbol.c_str(), c.conId, ibToString(field).c_str(), size );
}


int TwsDL::initWork()
{
	if( cfg.get_account ) {
		workTodo->addSimpleRequest(GenericRequest::ACC_STATUS_REQUEST);
	}
	if( cfg.get_exec ) {
		workTodo->addSimpleRequest(GenericRequest::EXECUTIONS_REQUEST);
	}
	if( cfg.get_order ) {
		workTodo->addSimpleRequest(GenericRequest::ORDERS_REQUEST);
	}

	int cnt = workTodo->read_file(cfg.workfile);
	if( cnt < 0 ) {
		/* it's not an error if no workfile is given and nothing on stdin */
		cnt = cfg.workfile == NULL ? 0 : -1;
		goto end;
	}
	DEBUG_PRINTF( "got %d jobs from workFile %s", cnt, cfg.workfile );

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
end:
	return cnt;
}


void TwsDL::dumpWorkTodo() const
{
	// HACK just dump it one time
	static bool firstTime = true;
	if( firstTime ) {
		firstTime = false;
		workTodo->getHistTodo().dumpLeft();
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
}


long TwsDL::fetch_inc_order_id()
{
	long orderId = tws_valid_orderId;
	tws_valid_orderId++;
	return orderId;
}


void TwsDL::reqContractDetails()
{
	if (! rate_limit->can_send())
		return;
	workTodo->contractDetailsTodo()->checkout();
	const ContractDetailsRequest &cdR
		= workTodo->getContractDetailsTodo().current();

	PacketContractDetails *p_contractDetails = new PacketContractDetails();
	packet = p_contractDetails;
	currentRequest.nextRequest( GenericRequest::CONTRACT_DETAILS_REQUEST );

	p_contractDetails->record( currentRequest.reqId(), cdR );
	twsClient->reqContractDetails( currentRequest.reqId(), cdR.ibContract() );
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
	pacingControl.addRequest( hR.ibContract );

	const Contract &c = hR.ibContract;
	DEBUG_PRINTF( "REQ_HISTORICAL_DATA %p %d: %ld,%s,%s,%s,%s %s,%s,%s,%s",
		&workTodo->getHistTodo().current(),
		currentRequest.reqId(), c.conId,
		c.symbol.c_str(), c.secType.c_str(),c.exchange.c_str(),c.lastTradeDateOrContractMonth.c_str(),
		hR.whatToShow.c_str(), hR.endDateTime.c_str(),
		hR.durationStr.c_str(), hR.barSizeSetting.c_str() );

	p_histData->record( currentRequest.reqId(), hR );
	twsClient->reqHistoricalData( currentRequest.reqId(),
	                              c,
	                              hR.endDateTime,
	                              hR.durationStr,
	                              hR.barSizeSetting,
	                              hR.whatToShow,
	                              hR.useRTH,
	                              hR.formatDate );
}

void TwsDL::reqAccStatus()
{
	PacketAccStatus *accStatus = new PacketAccStatus();
	packet = accStatus;
	currentRequest.nextRequest( GenericRequest::ACC_STATUS_REQUEST );

	AccStatusRequest aR;
	aR.subscribe = true;
	aR.acctCode = cfg.tws_account_name;

	accStatus->record( aR );
	twsClient->reqAccountUpdates(aR.subscribe, aR.acctCode);
}

void TwsDL::reqExecutions()
{
	PacketExecutions *executions = new PacketExecutions();
	packet = executions;
	currentRequest.nextRequest( GenericRequest::EXECUTIONS_REQUEST );

	ExecutionsRequest eR;

	executions->record( currentRequest.reqId(), eR );
	twsClient->reqExecutions( currentRequest.reqId(), eR.executionFilter);
}

void TwsDL::reqOrders()
{
	PacketOrders *orders = new PacketOrders();
	packet = orders;
	currentRequest.nextRequest( GenericRequest::ORDERS_REQUEST );

	OrdersRequest oR;

	orders->record( oR );
	twsClient->reqAllOpenOrders();
}

void TwsDL::placeOrder()
{
	workTodo->placeOrderTodo()->checkout();
	const PlaceOrder &pO = workTodo->getPlaceOrderTodo().current();

	long orderId;
	if( pO.orderId == 0 ) {
		orderId = tws_valid_orderId;
		tws_valid_orderId++;
	} else {
		orderId = pO.orderId;
	}
	PacketPlaceOrder *p_placeOrder;
	if( p_orders.find(orderId) == p_orders.end() ) {
		p_placeOrder = new PacketPlaceOrder();
		p_orders[orderId] = p_placeOrder;
		p_placeOrder->record( orderId, pO );
	} else {
		// TODO order modify
		p_placeOrder = p_orders[orderId];
		p_placeOrder->modify( pO );
	}

	if( pO.order.totalQuantity != 0
	    || strcasecmp(pO.order.action.c_str(), "CANCEL") != 0 ) {
		twsClient->placeOrder( orderId, pO.contract, pO.order );
	} else {
		twsClient->cancelOrder( orderId );
	}
}

void TwsDL::placeAllOrders()
{
	PlaceOrderTodo* todo = workTodo->placeOrderTodo();
	while( todo->countLeft() > 0 && rate_limit->can_send()) {
		placeOrder();
	}
}

int TwsDL::reqMktData()
{
	const std::vector<MktDataRequest> &v =
		workTodo->getMktDataTodo().mktDataRequests;
	std::vector<MktDataRequest>::const_iterator it;
	int reqId = 1;

	/* initialize quote snapshot */
	assert(quotes->empty());
	quotes->resize(v.size());

	for( it = v.begin(); it < v.end(); it++, reqId++ ) {
		twsClient->reqMktData( reqId, it->ibContract, it->genericTicks,
			it->snapshot );
	}
	if( reqId > 1 ) {
// 		changeState( WAIT_DATA );
	}
	return reqId;
}
