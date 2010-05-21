#ifndef WORKER_H
#define WORKER_H

#include "core/properties.h"

#include <QtCore/QObject>
#include <QtCore/QList>




class TWSClient;
class TWSWrapper;
class QTimer;
class QSqlDatabase;
class QSqlQuery;

namespace IB {
	class ContractDetails;
	class Contract;
}


namespace Test {


class PropTWSTool : public PropGeneral
{
	public:
		PropTWSTool( const Properties& prop, const QString& cName = "" );
		
		void initDefaults();
		bool readProperties();
		
		// fields
		QString twsHost;
		quint16 twsPort;
		int     clientId;
		
		bool useDB;
		QString ibSymbolTable;
		QString symbolQueryStrg; // %1 = ibSymbolTable
		
		int conTimeout;
		int reqTimeout;
		
		bool downloadData;
};




class Worker : public QObject
{
	Q_OBJECT
	
	public:
		enum State {
			START,
			WAIT_TWS_CON,
			GET_CONTRACTS,
			WAIT_CONTRACTS,
			FIN_CONTRACTS,
			GET_DATA,
			WAIT_DATA,
			FIN_DATA,
			QUIT_READY,
			QUIT_ERROR
		};
		
		Worker();
		~Worker();
		
		void start();
		
		State currentState() const;
		
	signals:
		void finished();
		
	private:
		void initProperties();
		void initTwsClient();
		void initIdleTimer();
		bool initDb();
		void removeDb();
		
		/// Returns the count of inserted rows or -1 on error.
		int storage2DB();
		int storage2stdout();
		
		
		void onStart();
		void waitTwsCon();
		void getContracts();
		void waitContracts();
		void finContracts();
		void getData();
		void waitData();
		void finData();
		void onQuit( int ret );
		
		State state;
		
		//tws settings
		PropTWSTool *myProp;
		
		TWSClient  *twsClient;
		TWSWrapper *twsWrapper;
		
		int currentReqId;
		bool finishedReq;
		int countNewContracts;
		
		QSqlDatabase *db;
		QSqlQuery *symbolQuery;
		QSqlQuery *warnQuery;
		
		QList<IB::ContractDetails> contractDetailsStorage;
		QList<IB::Contract> rememberContracts;
		
		QTimer *idleTimer;
		
	private slots:
		void idleTimeout();
		
		void twsConnected( bool connected );
		void contractDetails2Storage( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void contractDetailsEnd( int reqId );
		void historicalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps );
};




} // namespace Test
#endif
