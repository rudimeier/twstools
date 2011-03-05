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
};

class QTimer;


class TWSClient : public QObject
{
	Q_OBJECT
	
	public:
		TWSClient();
		~TWSClient();
		
		bool isConnected() const;
		
		QString getTWSHost() const;
		quint16 getTWSPort() const;
		int     getClientId() const;
		
		void setTWSHost( const QString &host );
		void setTWSPort( const quint16 &port );
		void setClientId( int clientId );
		
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
		
	signals:
		void connected(bool connected);
		
		void error( const QString &msg );
		void error(int id, int errorCode, const QString &errorMsg);
		
		void tickPrice( int tickerId, int field, double price, int canAutoExecute);
		void tickSize( int tickerId, int field, int size );
		void orderStatus( int orderId, const QString &status, int filled, int remaining, double avgFillPrice,
			int permId, int parentId, double lastFillPrice, int clientId, const QString &whyHeld );
		void openOrder( int orderId, const IB::Contract &contract, const IB::Order &order, const IB::OrderState &orderState );
		void updateAccountValue( const QString &key, const QString &value, const QString &currency, const QString &accountName);
		void updatePortfolio( const IB::Contract &contract, int position, double marketPrice, double marketValue,
			double averageCost, double unrealizedPNL, double realizedPNL, const QString &accountName );
		void updateAccountTime( const QString &timeStamp );
		void nextValidId( int orderId );
		void bondContractDetails ( int reqId, const IB::ContractDetails &ibContractDetails );
		void contractDetails ( int reqId, const IB::ContractDetails &ibContractDetails );
		void contractDetailsEnd ( int reqId );
		void execDetails( int reqId, const IB::Contract &contract, const IB::Execution &exec );
		void managedAccounts( const QString &accountsList );
    	void historicalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps );
		void tickOptionComputation ( int tickerId, int tickType, double impliedVol, double delta, double modelPrice, double pvDividend );
		void tickGeneric( int tickerId, int tickType, double value );
		void tickString( int tickerId, int tickType, const QString &value );
		void currentTime( long time );
		
		
	private:
		static void registerMetaTypes();
		
		void startSelect();
		void stopSelect();
		
		void disconnected();
		
		QString twsHost;
		quint16 twsPort;
		int     clientId;
		
		
		IB::EPosixClientSocket* ePosixClient;
		
		class MyEWrapper;
		MyEWrapper* myEWrapper;
		
		QTimer *selectTimer;
		
	private slots:
		
		void selectStuff();
};


#endif
