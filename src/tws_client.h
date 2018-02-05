/*** tws_client.h -- IB/API TWSClient class
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_CLIENT_H
#define TWS_CLIENT_H

#include <string>


namespace IB {
	class Contract;
	class ContractDetails;
	class Order;
	class OrderState;
	class Execution;
	class ExecutionFilter;
	class EPosixClientSocket;
	class EWrapper;
};


class TWSClient
{
	public:
		TWSClient( IB::EWrapper *ew );
		~TWSClient();

		bool isConnected() const;

		void selectStuff( int msec );

		/////////////////////////////////////////////////////
		bool connectTWS( const std::string &host, int port, int clientId,
			int ai_family );
		void disconnectTWS();

		int serverVersion();
		std::string TwsConnectionTime();

		void reqMktData( int tickerId, const IB::Contract &contract, const std::string &genericTickList, bool snapshot );
		void cancelMktData( int tickerId );
		void placeOrder( int id, const IB::Contract &contract, const IB::Order &order );
		void cancelOrder( int id );
		void reqOpenOrders();
		void reqAllOpenOrders();
		void reqAutoOpenOrders( bool bAutoBind );
		void reqExecutions(int reqId, const IB::ExecutionFilter& filter);
		void reqAccountUpdates( bool subscribe, const std::string &acctCode );
		void reqIds( int numIds );
		void reqContractDetails( int reqId, const IB::Contract &contract );
		void setServerLogLevel( int logLevel );
		void reqHistoricalData ( int tickerId, const IB::Contract &contract,
			const std::string &endDateTime, const std::string &durationStr,
			const std::string &barSizeSetting, const std::string &whatToShow,
			int useRTH, int formatDate );
		void reqCurrentTime();

	private:
		IB::EWrapper* myEWrapper;
		IB::EPosixClientSocket* ePosixClient;
};


#endif
