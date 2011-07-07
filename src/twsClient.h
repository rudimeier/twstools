#ifndef TWS_CLIENT_H
#define TWS_CLIENT_H

#include <string>


namespace IB {
	class Contract;
	class ContractDetails;
	class Order;
	class OrderState;
	class Execution;
	class EPosixClientSocket;
	class EWrapper;
};


class TWSClient
{
	public:
		TWSClient( IB::EWrapper *ew );
		~TWSClient();
		
		bool isConnected() const;
		
		std::string getTWSHost() const;
		int getTWSPort() const;
		int getClientId() const;
		
		void setTWSHost( const std::string &host );
		void setTWSPort( const int port );
		void setClientId( int clientId );
		
		void selectStuff( int msec );
		
		/////////////////////////////////////////////////////
		void connectTWS();
		void connectTWS( const std::string &host, int port, int clientId );
		void disconnectTWS();
		
		void reqMktData( int tickerId, const IB::Contract &contract, const std::string &genericTickList, bool snapshot );
		void cancelMktData( int tickerId );
		void placeOrder( int id, const IB::Contract &contract, const IB::Order &order );
		void cancelOrder( int id );
		void reqOpenOrders();
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
		void disconnected();
		
		std::string twsHost;
		int twsPort;
		int clientId;
		
		IB::EWrapper* myEWrapper;
		IB::EPosixClientSocket* ePosixClient;
};


#endif
