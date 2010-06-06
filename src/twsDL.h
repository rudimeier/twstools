#ifndef TWS_DL_H
#define TWS_DL_H

#include "core/properties.h"

#include <QtCore/QObject>
#include <QtCore/QList>




class TWSClient;
class TWSWrapper;
class QTimer;

namespace IB {
	class ContractDetails;
	class Contract;
}


namespace Test {




class ContractDetailsRequest;
class HistRequest;
class GenericRequest;
class ContractDetailsTodo;
class HistTodo;


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
		int pacingTime;
		int violationPause;
		
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




class TwsDL : public QObject
{
	Q_OBJECT
	
	public:
		enum State {
			START,
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
		
		void onStart();
		void waitTwsCon();
		void idle();
		void getContracts();
		void waitContracts();
		void finContracts();
		void getData();
		void waitData();
		void pauseData();
		void finData();
		void onQuit( int ret );
		
		void initWork();
		
		void reqContractDetails( const ContractDetailsRequest& );
		void reqHistoricalData( const HistRequest& );
		
		State state;
		
		QString confFile;
		QString workFile;
		PropTwsDL *myProp;
		
		TWSClient  *twsClient;
		TWSWrapper *twsWrapper;
		
		GenericRequest &currentRequest;
		
		int curReqSpecIndex;
		int curReqContractIndex;
		
		QList<IB::ContractDetails> contractDetailsStorage;
		ContractDetailsTodo *contractDetailsTodo;
		HistTodo *histTodo;
		
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
