#ifndef TWS_CLIENT_H
#define TWS_CLIENT_H

#include <QtCore/QObject>


namespace IB {
	class Contract;
	class ContractDetails;
	class Order;
	class OrderState;
	class Execution;
	class EPosixClientSocket;
	class EWrapper;
};


class TWSClient : public QObject
{
	Q_OBJECT
	
	public:
		TWSClient( IB::EWrapper *ew );
		~TWSClient();
		
		bool isConnected() const;
		
		QString getTWSHost() const;
		quint16 getTWSPort() const;
		int     getClientId() const;
		
		void setTWSHost( const QString &host );
		void setTWSPort( const quint16 &port );
		void setClientId( int clientId );
		
		void selectStuff( int msec );
		
		/////////////////////////////////////////////////////
		void connectTWS();
		void connectTWS( const QString &host, quint16 port, int clientId );
		void disconnectTWS();
		
		void reqMktData( int tickerId, const IB::Contract &contract, const QString &genericTickList, bool snapshot );
		void cancelMktData( int tickerId );
		void placeOrder( int id, const IB::Contract &contract, const IB::Order &order );
		void cancelOrder( int id );
		void reqOpenOrders();
		void reqAccountUpdates( bool subscribe, const QString &acctCode );
		void reqIds( int numIds );
		void reqContractDetails( int reqId, const IB::Contract &contract );
		void setServerLogLevel( int logLevel );
		void reqHistoricalData ( int tickerId, const IB::Contract &contract, const QString &endDateTime, const QString &durationStr,
			const QString &barSizeSetting, const QString &whatToShow, int useRTH, int formatDate );
		void reqCurrentTime();
		
	private:
		static void registerMetaTypes();
		
		void disconnected();
		
		QString twsHost;
		quint16 twsPort;
		int     clientId;
		
		IB::EWrapper* myEWrapper;
		IB::EPosixClientSocket* ePosixClient;
};


#endif
