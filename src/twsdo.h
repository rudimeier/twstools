/*** twsdo.h -- TWS job processing tool
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_DL_H
#define TWS_DL_H

#include <string>
#include <stdint.h>
#include <map>

#include "tws_quote.h"

#include <twsapi/twsapi_config.h>
#include <twsapi/EWrapper.h>



class TWSClient;
struct RateLimit;

#ifndef TWSAPI_NO_NAMESPACE
namespace IB {
#endif
	class ContractDetails;
	class Contract;
	class Execution;
#ifndef TWSAPI_NO_NAMESPACE
}
using namespace IB;
#endif

struct ConfigTwsdo
{
	ConfigTwsdo();

	void init_ai_family( int ipv4, int ipv6 );
	void init_mkt_data_type(const char *str);

	const char *workfile;
	int skipdef;
	const char *tws_host;
	int tws_port;
	int tws_client_id;
	int ai_family;

	int get_account;
	const char* tws_account_name;
	int get_exec;
	int get_order;

	int mkt_data_type;
	int tws_conTimeout;
	int tws_reqTimeout;
	int tws_maxRequests;
	int tws_pacingInterval;
	int tws_minPacingTime;
	int tws_violationPause;

	const char* strat_file;
};




class ContractDetailsRequest;
class HistRequest;
class GenericRequest;
class WorkTodo;
class ContractDetailsTodo;
class HistTodo;
class Packet;
class PacketPlaceOrder;
class RowError;
class RowHist;
class RowAccVal;
class RowPrtfl;
class RowExecution;
class RowOrderStatus;
class RowOpenOrder;
class PacingGod;
class DataFarmStates;
class Account;

class TwsDlWrapper;
class TwsHeartBeat;

typedef struct tws_dso_s *tws_dso_t;



class TwsDL
{
	public:
		enum State {
			WAIT_TWS_CON,
			IDLE
		};

		TwsDL();
		~TwsDL();

		int setup( const ConfigTwsdo& );
		int start();

		State currentState() const;
		std::string lastError() const;

// 	private:
		void eventLoop();

		void dumpWorkTodo() const;

		void connectTws();
		void waitTwsCon();
		void idle();
		bool finContracts();
		bool finHist();
		bool finPlaceOrder();
		void waitData();

		void changeState( State );

		int initWork();

		long fetch_inc_order_id();

		void reqContractDetails();
		void reqHistoricalData();
		void reqAccStatus();
		void reqExecutions();
		void reqOrders();
		void placeOrder();
		void placeAllOrders();
		int reqMktData();

		void errorContracts( const RowError& );
		void errorHistData( const RowError& );
		void errorPlaceOrder( const RowError& );

		// callbacks from our twsWrapper
		void twsError( const RowError& );

		void twsConnectionClosed();
		void twsContractDetails( int reqId,
			const ContractDetails &ibContractDetails );
		void twsBondContractDetails( int reqId,
			const ContractDetails &ibContractDetails );
		void twsContractDetailsEnd( int reqId );
		void twsHistoricalData( int reqId, const RowHist& );
		void twsUpdateAccountValue( const RowAccVal& );
		void twsUpdatePortfolio( const RowPrtfl& );
		void twsUpdateAccountTime( const std::string& timeStamp );
		void twsAccountDownloadEnd( const std::string& accountName );
		void twsExecDetails( int reqId, const RowExecution& );
		void twsExecDetailsEnd( int reqId );
		void twsOrderStatus( const RowOrderStatus& );
		void twsOpenOrder( const RowOpenOrder& );
		void twsOpenOrderEnd();
		void twsCurrentTime( long time );
		void nextValidId( long orderId );
		void twsTickPrice( int reqId, TickType field, double price,
			int canAutoExecute );
		void twsTickSize( int reqId, TickType field, int size );
		void twsTickOptionComputation( TickerId tickerId,
			TickType tickType, double impliedVol, double delta,
			double optPrice, double pvDividend, double gamma, double vega,
			double theta, double undPrice );
		void twsTickGeneric( TickerId tickerId, TickType tickType,
			double value );
		void twsTickString(TickerId tickerId, TickType tickType,
			const IBString& value );
		void twsConnectAck();

		State state;
		bool quit;
		int error;
		std::string _lastError;
		int64_t lastConnectionTime;
		TwsHeartBeat *tws_hb;
		long tws_valid_orderId;
		bool connectivity_IB_TWS;
		int curIdleTime;

		ConfigTwsdo cfg;

		TwsDlWrapper *twsWrapper;
		TWSClient  *twsClient;

		RateLimit *rate_limit;

		int msgCounter;
		GenericRequest &currentRequest;

		WorkTodo *workTodo;
		Account *account;
		Quotes *quotes;

		Packet *packet;
		std::map<long, PacketPlaceOrder*> p_orders;
		std::map<long, PacketPlaceOrder*> p_orders_old;

		std::map<long, ContractDetails*> con_details;

		DataFarmStates &dataFarms;
		PacingGod &pacingControl;

		tws_dso_t strat;

	friend class TwsDlWrapper;
};




#endif
