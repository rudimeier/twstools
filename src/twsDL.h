#ifndef TWS_DL_H
#define TWS_DL_H

#include "properties.h"

#include <QtCore/QObject>
#include <QtCore/QList>




class TWSClient;
class QTimer;

namespace IB {
	class ContractDetails;
	class Contract;
}






class ContractDetailsRequest;
class HistRequest;
class GenericRequest;
class ContractDetailsTodo;
class HistTodo;
class PacketContractDetails;
class PacketHistData;
class PacingGod;
class DataFarmStates;


class PropTwsDL : public PropSub
{
	public:
		PropTwsDL( const Properties& prop, const QString& cName = "" );
		
		void initDefaults();
		bool readProperties();
		
		// fields
		QString twsHost;
		quint16 twsPort;
		int     clientId;
		
		int conTimeout;
		int reqTimeout;
		int maxRequests;
		int pacingInterval;
		int minPacingTime;
		int violationPause;
		
		bool downloadData;
		int reqMaxContracts;
		int reqMaxContractsPerSpec;
		
		bool printFormatDates;
		
		bool includeExpired;
		
		QString endDateTime;
		QString durationStr;
		QString barSizeSetting;
		QList<QString> whatToShow;
		int useRTH;
		int formatDate;
		
		QList< QList<QString> > contractSpecs;
};




class TwsDL : public QObject
{
	Q_OBJECT
	
	public:
		enum State {
			CONNECT,
			WAIT_TWS_CON,
			IDLE,
			WAIT_DATA,
			QUIT_READY,
			QUIT_ERROR
		};
		
		TwsDL( const QString& confFile, const QString& workFile );
		~TwsDL();
		
		void start();
		
		State currentState() const;
		
	signals:
		void finished();
		
	private:
		void initProperties();
		void initTwsClient();
		void initIdleTimer();
		
		/// Returns the count of inserted rows or -1 on error.
		int storage2stdout();
		
		void dumpWorkTodo() const;
		
		void connectTws();
		void waitTwsCon();
		void idle();
		void getContracts();
		void finContracts();
		void getData();
		void waitData();
		void waitContracts();
		void waitHist();
		void finData();
		void onQuit( int ret );
		
		void changeState( State );
		
		void initWork();
		
		void reqContractDetails( const ContractDetailsRequest& );
		void reqHistoricalData( const HistRequest& );
		
		void errorContracts(int, int, const QString &);
		void errorHistData(int, int, const QString &);
		
		State state;
		qint64 lastConnectionTime;
		bool connection_failed;
		int curIdleTime;
		
		QString confFile;
		QString workFile;
		PropTwsDL *myProp;
		
		TWSClient  *twsClient;
		
		int msgCounter;
		GenericRequest &currentRequest;
		
		int curIndexTodoContractDetails;
		
		ContractDetailsTodo *contractDetailsTodo;
		HistTodo *histTodo;
		
		PacketContractDetails &p_contractDetails;
		PacketHistData &p_histData;
		
		DataFarmStates &dataFarms;
		PacingGod &pacingControl;
		
		QTimer *idleTimer;
		
	private slots:
		void idleTimeout();
		
		void twsError(int, int, const QString &);
		
		void twsConnected( bool connected );
		void twsContractDetails( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void twsBondContractDetails( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void twsContractDetailsEnd( int reqId );
		void twsHistoricalData( int reqId, const QString &date, double open, double high, double low,
			double close, int volume, int count, double WAP, bool hasGaps );
};




#endif
