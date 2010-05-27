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




struct HistRequest;


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
		int pacingTime;
		int violationPause;
		
		QString reqExpiry;
		
		bool downloadData;
		int reqMaxContracts;
		int reqMaxContractsPerSpec;
		bool ignoreNotAvailable;
		
		bool printFormatDates;
		
		QString endDateTime;
		QString durationStr;
		QString barSizeSetting;
		QList<QString> whatToShow;
		int useRTH;
		int formatDate;
		
		QList< QList<QString> > contractSpecs;
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
			PAUSE_DATA,
			FIN_DATA,
			QUIT_READY,
			QUIT_ERROR
		};
		
		Worker( const QString& confFile );
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
		
		void dumpWorkTodo() const;
		
		void onStart();
		void waitTwsCon();
		void getContracts();
		void waitContracts();
		void finContracts();
		void getData();
		void waitData();
		void pauseData();
		void finData();
		void onQuit( int ret );
		
		State state;
		
		QString confFile;
		PropTWSTool *myProp;
		
		TWSClient  *twsClient;
		TWSWrapper *twsWrapper;
		
		int currentReqId;
		bool finishedReq;
		int countNewContracts;
		
		int curReqContractIndex;
		
		QSqlDatabase *db;
		QSqlQuery *symbolQuery;
		QSqlQuery *warnQuery;
		
		QList<IB::ContractDetails> contractDetailsStorage;
		QList<HistRequest> histRequests;
		
		QTimer *idleTimer;
		
	private slots:
		void idleTimeout();
		
		void error(int, int, const QString &);
		
		void twsConnected( bool connected );
		void contractDetails2Storage( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void contractDetailsEnd( int reqId );
		void historicalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps );
};




} // namespace Test
#endif
