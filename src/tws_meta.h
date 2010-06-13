#ifndef TWS_META_H
#define TWS_META_H

#include "ibtws/Contract.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QDateTime>




namespace Test {




/// stupid static helper
QString ibDate2ISO( const QString &ibDate );

extern const QHash<QString, const char*> short_wts;
extern const QHash<QString, const char*> short_bar_size;








class ContractDetailsRequest
{
	public:
		bool initialize( const IB::Contract& );
		bool fromStringList( const QList<QString>& );
		
		IB::Contract ibContract;
};








class HistRequest
{
	public:
		bool initialize( const IB::Contract&, const QString &endDateTime,
			const QString &durationStr, const QString &barSizeSetting,
			const QString &whatToShow, int useRTH, int formatDate );
		bool fromString( const QString& );
		QString toString() const;
		void clear();
		
		IB::Contract ibContract;
		QString endDateTime;
		QString durationStr;
		QString barSizeSetting;
		QString whatToShow;
		int useRTH;
		int formatDate;
};







class GenericRequest
{
	public:
		enum ReqType {
			NONE,
			CONTRACT_DETAILS_REQUEST,
			HIST_REQUEST
		};
		
		GenericRequest();
		
		void nextRequest( ReqType );
		void close();
		
		ReqType reqType;
		int reqId;
};








class PacingGod;
class DataFarmStates;

class HistTodo
{
	public:
		HistTodo();
		~HistTodo();
		
		int fromFile( const QString & fileName );
		void dump( FILE *stream ) const;
		void dumpLeft( FILE *stream ) const;
		
		int countDone() const;
		int countLeft() const;
		int currentIndex() const;
		const HistRequest& current() const;
		void tellDone();
		void add( const HistRequest& );
		void optimize(const PacingGod*, const DataFarmStates*);
		
	private:
		int read_file( const QString & fileName, QList<QByteArray> *list ) const;
		
		int curIndexTodoHistData;
		QList<HistRequest*> histRequests;
		QList<int> doneRequests;
		QList<int> leftRequests;
};








class ContractDetailsTodo
{
	public:
		int fromConfig( const QList< QList<QString> > &contractSpecs );
		
		QList<ContractDetailsRequest> contractDetailsRequests;
};








class PacketContractDetails
{
	public:
		PacketContractDetails();
		
		const QList<IB::ContractDetails>& constList() const;
		void setFinished();
		bool isFinished() const;
		void clear();
		void append( int reqId, const IB::ContractDetails& );
		
	private:
		bool complete;
		int reqId;
		QList<IB::ContractDetails> cdList;
};








class PacketHistData
{
	public:
		enum Mode { CLEAN, RECORD, CLOSED_SUCC, CLOSED_ERR };
		
		PacketHistData();
		
		bool isFinished() const;
		bool needRepeat() const;
		void clear();
		void record( int reqId );
		void append( int reqId, const QString &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps );
		void closeError( bool repeat );
		void dump( const HistRequest&, bool printFormatDates );
		
	private:
		class Row
		{
			public:
				void clear();
				
				QString date;
				double open;
				double high;
				double low;
				double close;
				int volume;
				int count;
				double WAP;
				bool hasGaps;
		};
		
		Mode mode;
		bool repeat;
		
		int reqId;
		QList<Row> rows;
		Row finishRow;
};








class PacingControl
{
	public:
		PacingControl( int packets, int interval, int min, int vPause );
		
		void setPacingTime( int packets, int interval, int min );
		void setViolationPause( int violationPause );
		
		bool isEmpty() const;
		void clear();
		void addRequest();
		void notifyViolation();
		int goodTime() const;
		
		void merge( const PacingControl& );
		
	private:
		static quint64 nowInMsecs();
		
		QList<quint64> dateTimes;
		QList<bool> violations;
		
		int maxRequests;
		int checkInterval;
		int minPacingTime;
		int violationPause;
};




class PacingGod
{
	public:
		PacingGod( const DataFarmStates& );
		~PacingGod();
		
		void setPacingTime( int packets, int interval, int min );
		void setViolationPause( int pause );
		
		void clear();
		void addRequest( const IB::Contract& );
		void notifyViolation( const IB::Contract& );
		int goodTime( const IB::Contract& );
	
	private:
		void checkAdd( const IB::Contract&,
			QString *lazyContract, QString *farm );
		bool laziesAreCleared() const;
		
		const DataFarmStates& dataFarms;
		
		int maxRequests;
		int checkInterval;
		int minPacingTime;
		int violationPause;
		
		PacingControl &controlGlobal;
		QHash<const QString, PacingControl*> controlHmds;
		QHash<const QString, PacingControl*> controlLazy;
};







class DataFarmStates
{
	public:
		enum State { BROKEN, INACTIVE, OK };
		
		QStringList getInactives() const;
		QStringList getActives() const;
		QString getMarketFarm( const IB::Contract& ) const;
		QString getHmdsFarm( const IB::Contract& ) const;
		
		void notify(int errorCode, const QString &msg);
		void learnMarket( const IB::Contract& );
		void learnHmds( const IB::Contract& );
		
	private:
		static QString getFarm( const QString prefix, const QString& msg );
		
		QHash<const QString, State> mStates;
		QHash<const QString, State> hStates;
		
		QHash<const QString, QString> mLearn;
		QHash<const QString, QString> hLearn;
};




} // namespace Test
#endif
