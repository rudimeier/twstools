#ifndef TWSWrapper_H
#define TWSWrapper_H

#include <QtCore/QObject>

namespace IB {
	class Contract;
	class ContractDetails;
	class Order;
	class OrderState;
	class Execution;
}


// this class is just an example or for debugging
class TWSWrapper : public QObject  {
	//TODO
	Q_OBJECT

	public:
		~TWSWrapper();

		static void connectAllSignals	(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);

		static void connectError					(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectError					(QObject *a, QObject *b);
		static void connectTickPrice				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectTickPrice				(QObject *a, QObject *b);
		static void connectTickSize					(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectTickSize				(QObject *a, QObject *b);
		static void connectOrderStatus				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectOrderStatus			(QObject *a, QObject *b);
		static void connectOpenOrder				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectOpenOrder				(QObject *a, QObject *b);
		static void connectUpdateAccountValue		(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectUpdateAccountValue	(QObject *a, QObject *b);
		static void connectUpdatePortfolio			(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectUpdatePortfolio		(QObject *a, QObject *b);
		static void connectTickGeneric				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectTickGeneric			(QObject *a, QObject *b);
		static void connectTickString				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectTickString			(QObject *a, QObject *b);
		static void connectUpdateAccountTime		(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectUpdateAccountTime		(QObject *a, QObject *b);
		static void connectNextValidId				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectNextValidId			(QObject *a, QObject *b);
		static void connectContractDetails			(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectContractDetails		(QObject *a, QObject *b);
		static void connectContractDetailsEnd		(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectContractDetailsEnd	(QObject *a, QObject *b);
		static void connectExecDetails				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectExecDetails			(QObject *a, QObject *b);
		static void connectManagedAccounts			(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectManagedAccounts		(QObject *a, QObject *b);
		static void connectHistoricalData			(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectHistoricalData		(QObject *a, QObject *b);
		static void connectTickOptionComputation	(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectTickOptionComputation	(QObject *a, QObject *b);
		static void connectCurrentTime				(QObject *a, QObject *b, Qt::ConnectionType type = Qt::AutoConnection);
		static void disconnectCurrentTime			(QObject *a, QObject *b);

	public slots:
		///// slots to receive signals from TWS //////////////////////////////////////
		// here just for debugging

		void error( const QString &msg );
		void error( int id, int errorCode, const QString &errorMsg );

		void tickPrice( int tickerId, int field, double price, int canAutoExecute );
		void tickSize( int tickerId, int field, int size );
		void orderStatus ( int orderId, const QString &status, int filled, int remaining, double avgFillPrice,
			int permId, int parentId, double lastFillPrice, int clientId, const QString &whyHeld );
		void openOrder( int orderId, const IB::Contract &contract, const IB::Order &order, const IB::OrderState &orderState );
		void updateAccountValue( const QString &key, const QString &value, const QString &currency, const QString &accountName );
		void updatePortfolio( const IB::Contract &contract, int position, double marketPrice, double marketValue,
			double averageCost, double unrealizedPNL, double realizedPNL, const QString &accountName );
		void updateAccountTime( const QString &timeStamp );
		void nextValidId( int orderId );
		void contractDetails( int reqId, const IB::ContractDetails &ibContractDetails );
		void contractDetailsEnd( int reqId );
		void execDetails ( int orderId, const IB::Contract &contract, const IB::Execution &exec );
		void managedAccounts ( const QString &accountsList );
		void historicalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps );
		void tickOptionComputation ( int tickerId, int tickType, double impliedVol, double delta, double modelPrice, double pvDividend );
		void tickGeneric( int tickerId, int tickType, double value );
		void tickString( int tickerId, int tickType, const QString &value );
		void currentTime( long time );
		///// end Wrapper functions ////////////////////////////////////

		void moveMeToThread(QThread *thread);

};

#endif
