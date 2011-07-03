#ifndef TWS_META_H
#define TWS_META_H

#include "ibtws/Contract.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>

typedef struct _xmlNode * xmlNodePtr;
typedef struct _xmlDoc * xmlDocPtr;






qint64 nowInMsecs();

/// stupid static helper
QString ibDate2ISO( const QString &ibDate );

extern const QHash<QString, const char*> short_wts;
extern const QHash<QString, const char*> short_bar_size;








class ContractDetailsRequest
{
	public:
		const IB::Contract& ibContract() const;
		bool initialize( const IB::Contract& );
		bool fromStringList( const QList<QString>&, bool includeExpired );
		
	private:
		IB::Contract _ibContract;
};








class HistRequest
{
	public:
		const IB::Contract& ibContract() const;
		const QString& endDateTime() const;
		const QString& durationStr() const;
		const QString& barSizeSetting() const;
		const QString& whatToShow() const;
		int useRTH() const;
		int formatDate() const;
		
		bool initialize( const IB::Contract&, const QString &endDateTime,
			const QString &durationStr, const QString &barSizeSetting,
			const QString &whatToShow, int useRTH, int formatDate );
		bool fromString( const QString&, bool includeExpired );
		QString toString() const;
		void clear();
		
	private:
		IB::Contract _ibContract;
		QString _endDateTime;
		QString _durationStr;
		QString _barSizeSetting;
		QString _whatToShow;
		int _useRTH;
		int _formatDate;
};


inline const IB::Contract& HistRequest::ibContract() const
{
	return _ibContract;
}


inline const QString& HistRequest::endDateTime() const
{
	return _endDateTime;
}


inline const QString& HistRequest::durationStr() const
{
	return _durationStr;
}


inline const QString& HistRequest::barSizeSetting() const
{
	return _barSizeSetting;
}


inline const QString& HistRequest::whatToShow() const
{
	return _whatToShow;
}


inline int HistRequest::useRTH() const
{
	return _useRTH;
}


inline int HistRequest::formatDate() const
{
	return _formatDate;
}








class GenericRequest
{
	public:
		enum ReqType {
			NONE,
			CONTRACT_DETAILS_REQUEST,
			HIST_REQUEST
		};
		
		GenericRequest();
		
		ReqType reqType() const;
		int reqId() const;
		int age() const;
		void nextRequest( ReqType );
		void close();
		
	private:
		ReqType _reqType;
		int _reqId;
		
		qint64 _ctime;
};








class PacingGod;
class DataFarmStates;
class WorkTodo;

class HistTodo
{
	public:
		HistTodo();
		~HistTodo();
		
		int fromFile( const QList<QByteArray> &rows, bool includeExpired );
		void dump( FILE *stream ) const;
		void dumpLeft( FILE *stream ) const;
		
		int countDone() const;
		int countLeft() const;
		void checkout();
		int checkoutOpt( PacingGod *pG, const DataFarmStates *dfs );
		int currentIndex() const;
		const HistRequest& current() const;
		void tellDone();
		void cancelForRepeat( int priority );
		void add( const HistRequest& );
		void optimize(PacingGod*, const DataFarmStates*);
		
	private:
		QList<HistRequest*> histRequests;
		QList<int> doneRequests;
		QList<int> leftRequests;
		QList<int> errorRequests;
		QList<int> checkedOutRequests;
};








class ContractDetailsTodo
{
	public:
		int fromFile( const QList<QByteArray> &rows, bool includeExpired );
		
		QList<ContractDetailsRequest> contractDetailsRequests;
};








class WorkTodo
{
	public:
		WorkTodo();
		virtual ~WorkTodo();
		
		GenericRequest::ReqType getType() const;
		const QList<QByteArray>& getRows() const;
		int read_file( const QString & fileName);
		
	private:
		GenericRequest::ReqType reqType;
		QList<QByteArray> *rows;
};








class PacketContractDetails
{
	public:
		PacketContractDetails();
		virtual ~PacketContractDetails();
		
		static PacketContractDetails * fromXml( xmlDocPtr );
		
		const QList<IB::ContractDetails>& constList() const;
		void record( int reqId, const ContractDetailsRequest& );
		void setFinished();
		bool isFinished() const;
		void clear();
		void append( int reqId, const IB::ContractDetails& );
		
		void dumpXml();
		
	private:
		bool complete;
		int reqId;
		const IB::Contract *reqContract;
		QList<IB::ContractDetails> cdList;
};








class PacketHistData
{
	public:
		enum Mode { CLEAN, RECORD, CLOSED };
		enum Error { ERR_NONE, ERR_NODATA, ERR_NAV,
			ERR_TWSCON, ERR_TIMEOUT, ERR_REQUEST };
		
		PacketHistData();
		
		static PacketHistData * fromXml( xmlDocPtr );
		
		bool isFinished() const;
		Error getError() const;
		void clear();
		void record( int reqId );
		void append( int reqId, const QString &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps );
		void closeError( Error );
		void dump( const HistRequest&, bool printFormatDates );
		
		void dumpXml( const HistRequest& );
		
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
		Error error;
		
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
		int goodTime( const char** dbg ) const;
		int countLeft() const;
		
		void merge( const PacingControl& );
		
	private:
		QList<qint64> dateTimes;
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
		int countLeft( const IB::Contract& c );
	
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
		
		DataFarmStates();
		
		QStringList getInactives() const;
		QStringList getActives() const;
		QString getMarketFarm( const IB::Contract& ) const;
		QString getHmdsFarm( const QString& lazyC ) const;
		QString getHmdsFarm( const IB::Contract& ) const;
		
		void setAllBroken();
		void notify(int msgNumber, int errorCode, const QString &msg);
		void learnMarket( const IB::Contract& );
		void learnHmds( const IB::Contract& );
		void learnHmdsLastOk(int msgNumber, const IB::Contract& );
		
	private:
		static QString getFarm( const QString prefix, const QString& msg );
		
		QHash<const QString, State> mStates;
		QHash<const QString, State> hStates;
		
		QHash<const QString, QString> mLearn;
		QHash<const QString, QString> hLearn;
		
		int lastMsgNumber;
		QString lastChanged;
};




#endif
