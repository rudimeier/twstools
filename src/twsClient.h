#ifndef TWS_CLIENT_H
#define TWS_CLIENT_H

// from global installed ibapi
#include "ibtws/Contract.h"
#include "ibtws/Order.h"

#include <QtCore/QThread>
#include <QtCore/QMutex>


namespace IB {
	class OrderState;
	class Execution;
	class EPosixClientSocket;
};

class QTimer;


class TWSClient : public QThread
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
		void connectTWS( const QString &host, const quint16 &port, int clientId );
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
		
		void run();
		void startSelect();
		void stopSelect();
		
		/// Writes something to pipefd to wake up selectStuff().
		void infoPipe();
		
		QString twsHost;
		quint16 twsPort;
		int     clientId;
		
		
		IB::EPosixClientSocket* ePosixClient;
		
		class MyEWrapper;
		MyEWrapper* myEWrapper;
		
		QTimer *selectTimer;
		
		/// A pipe file descriptor used to wake up selectStuff() on pending
		/// outgoing messages.
		int pipefd[2];
		/// Needed to make infoPipe() thread safe.
		QMutex pipeMutex;
		
	private slots:
		//// wrapper slots to do requests from within the right thread /////
		void _connectTWS();
		void _connectTWS( QString host, quint16 port, int clientId );
		void _disconnectTWS();

		void _reqMktData( int tickerId, IB::Contract contract, QString genericTickList, bool snapshot );
		void _cancelMktData( int tickerId );
		void _placeOrder( int id, IB::Contract contract, IB::Order order );
		void _cancelOrder( int id );
		void _reqOpenOrders();
		void _reqAccountUpdates( bool subscribe, QString acctCode );
		void _reqIds( int numIds );
		void _reqContractDetails( int reqId, IB::Contract contract );
		void _setServerLogLevel( int logLevel );
		void _reqHistoricalData ( int tickerId, IB::Contract, QString endDateTime, QString durationStr,
			QString barSizeSetting, QString whatToShow, int useRTH, int formatDate );
		void _reqCurrentTime();
		//// end wrapper slots /////////////////////////////////////////////
		
		//// internal slots ////////////////////////////////////////////////
		void disconnected(); //should be "really" private 
		void tcpError(/*QAbstractSocket::SocketError socketError*/); //should be "really" private 
		
		void selectStuff();
		//// internal slots ////////////////////////////////////////////////
};


inline void TWSClient::infoPipe()
{
	QMutexLocker locker( &pipeMutex );
	char bla = 'A';
	int n = write( pipefd[1], &bla, 1);
	Q_ASSERT( n > 0 );
}


inline void TWSClient::connectTWS()
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_connectTWS", Qt::QueuedConnection) );
	infoPipe();
}


inline void TWSClient::connectTWS( const QString &host, const quint16 &port, int clientId)
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_connectTWS", Qt::QueuedConnection,
		Q_ARG(QString, host), Q_ARG( quint16, port ),  Q_ARG(int, clientId)) );
	infoPipe();
}


inline void TWSClient::disconnectTWS() {
	Q_ASSERT( QMetaObject::invokeMethod( this, "_disconnectTWS", Qt::QueuedConnection) );
	infoPipe();
}


inline void TWSClient::reqMktData( int tickerId, const IB::Contract &contract, const QString &genericTickList, bool snapshot )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqMktData", Qt::QueuedConnection,
		Q_ARG(int, tickerId), Q_ARG(IB::Contract, contract), Q_ARG(QString, genericTickList), Q_ARG(bool, snapshot)) );
	infoPipe();
}


inline void TWSClient::cancelMktData( int tickerId ) {
	Q_ASSERT( QMetaObject::invokeMethod( this, "_cancelMktData", Qt::QueuedConnection,
		Q_ARG(int, tickerId)) );
	infoPipe();
}


inline void TWSClient::placeOrder( int id, const IB::Contract &contract, const IB::Order &order )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_placeOrder", Qt::QueuedConnection,
		Q_ARG(int, id), Q_ARG(IB::Contract, contract), Q_ARG(IB::Order, order)) );
	infoPipe();
}


inline void TWSClient::cancelOrder( int id )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_cancelOrder", Qt::QueuedConnection,
		Q_ARG(int, id)) );
	infoPipe();
}


inline void TWSClient::reqOpenOrders()
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqOpenOrders", Qt::QueuedConnection) );
	infoPipe();
}


inline void TWSClient::reqAccountUpdates( bool subscribe, const QString &acctCode )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqAccountUpdates", Qt::QueuedConnection,
		Q_ARG(bool, subscribe),Q_ARG(QString, acctCode)) );
	infoPipe();
}


inline void TWSClient::reqIds( int numIds )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqIds", Qt::QueuedConnection,
		Q_ARG(int, numIds)) );
	infoPipe();
}


inline void TWSClient::reqContractDetails( int reqId, const IB::Contract &contract )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqContractDetails", Qt::QueuedConnection,
		Q_ARG(int, reqId), Q_ARG(IB::Contract, contract)) );
	infoPipe();
}


inline void TWSClient::setServerLogLevel( int logLevel )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_setServerLogLevel", Qt::QueuedConnection,
		Q_ARG(int, logLevel)) );
	infoPipe();
}


inline void TWSClient::reqHistoricalData ( int tickerId, const IB::Contract &contract, const QString &endDateTime, const QString &durationStr,
			const QString &barSizeSetting, const QString &whatToShow, int useRTH, int formatDate )
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqHistoricalData", Qt::QueuedConnection,
		Q_ARG(int,tickerId),Q_ARG(IB::Contract,contract),Q_ARG(QString,endDateTime),Q_ARG(QString,durationStr),
		Q_ARG(QString,barSizeSetting),Q_ARG(QString,whatToShow),Q_ARG(int,useRTH),Q_ARG(int,formatDate)) );
	infoPipe();
}


inline void TWSClient::reqCurrentTime()
{
	Q_ASSERT( QMetaObject::invokeMethod( this, "_reqCurrentTime", Qt::QueuedConnection) );
	infoPipe();
}

#endif
