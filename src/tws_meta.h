#ifndef TWS_META_H
#define TWS_META_H

#include "ibtws/Contract.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <stdint.h>

typedef struct _xmlNode * xmlNodePtr;
typedef struct _xmlDoc * xmlDocPtr;






int64_t nowInMsecs();

/// stupid static helper
std::string ibDate2ISO( const std::string &ibDate );

const char* short_wts( const char* wts );
const char* short_bar_size( const char* bar_size );







class ContractDetailsRequest
{
	public:
		static ContractDetailsRequest * fromXml( xmlNodePtr );
		
		const IB::Contract& ibContract() const;
		bool initialize( const IB::Contract& );
		
	private:
		IB::Contract _ibContract;
};








class HistRequest
{
	public:
		static HistRequest * fromXml( xmlNodePtr );
		
		const IB::Contract& ibContract() const;
		const std::string& endDateTime() const;
		const std::string& durationStr() const;
		const std::string& barSizeSetting() const;
		const std::string& whatToShow() const;
		int useRTH() const;
		int formatDate() const;
		
		bool initialize( const IB::Contract&, const std::string &endDateTime,
			const std::string &durationStr, const std::string &barSizeSetting,
			const std::string &whatToShow, int useRTH, int formatDate );
		std::string toString() const;
		void clear();
		
	private:
		IB::Contract _ibContract;
		std::string _endDateTime;
		std::string _durationStr;
		std::string _barSizeSetting;
		std::string _whatToShow;
		int _useRTH;
		int _formatDate;
};


inline const IB::Contract& HistRequest::ibContract() const
{
	return _ibContract;
}


inline const std::string& HistRequest::endDateTime() const
{
	return _endDateTime;
}


inline const std::string& HistRequest::durationStr() const
{
	return _durationStr;
}


inline const std::string& HistRequest::barSizeSetting() const
{
	return _barSizeSetting;
}


inline const std::string& HistRequest::whatToShow() const
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
		
		int64_t _ctime;
};








class PacingGod;
class DataFarmStates;
class WorkTodo;

class HistTodo
{
	public:
		HistTodo();
		~HistTodo();
		
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
		QList<HistRequest*> &histRequests;
		QList<int> &doneRequests;
		QList<int> &leftRequests;
		QList<int> &errorRequests;
		QList<int> &checkedOutRequests;
};








class ContractDetailsTodo
{
	public:
		ContractDetailsTodo();
		virtual ~ContractDetailsTodo();
		
		QList<ContractDetailsRequest> &contractDetailsRequests;
};








class WorkTodo
{
	public:
		WorkTodo();
		virtual ~WorkTodo();
		
		GenericRequest::ReqType getType() const;
		ContractDetailsTodo* contractDetailsTodo() const;
		const ContractDetailsTodo& getContractDetailsTodo() const;
		HistTodo* histTodo() const;
		const HistTodo& getHistTodo() const;
		int read_file( const std::string & fileName);
		
	private:
		GenericRequest::ReqType reqType;
		ContractDetailsTodo *_contractDetailsTodo;
		HistTodo *_histTodo;
};








class PacketContractDetails
{
	public:
		PacketContractDetails();
		virtual ~PacketContractDetails();
		
		static PacketContractDetails * fromXml( xmlNodePtr );
		
		const ContractDetailsRequest& getRequest() const;
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
		ContractDetailsRequest *request;
		QList<IB::ContractDetails> &cdList;
};








class PacketHistData
{
	public:
		enum Mode { CLEAN, RECORD, CLOSED };
		enum Error { ERR_NONE, ERR_NODATA, ERR_NAV,
			ERR_TWSCON, ERR_TIMEOUT, ERR_REQUEST };
		
		PacketHistData();
		virtual ~PacketHistData();
		
		static PacketHistData * fromXml( xmlNodePtr );
		
		const HistRequest& getRequest() const;
		bool isFinished() const;
		Error getError() const;
		void clear();
		void record( int reqId, const HistRequest& );
		void append( int reqId, const std::string &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps );
		void closeError( Error );
		void dump( bool printFormatDates );
		
		void dumpXml();
		
	private:
		class Row
		{
			public:
				void clear();
				
				std::string date;
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
		HistRequest *request;
		QList<Row> &rows;
		Row finishRow;
};








class PacingControl
{
	public:
		PacingControl( int packets, int interval, int min, int vPause );
		virtual ~PacingControl();
		
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
		QList<int64_t> &dateTimes;
		QList<bool> &violations;
		
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
		QHash<const QString, PacingControl*> &controlHmds;
		QHash<const QString, PacingControl*> &controlLazy;
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
		void notify(int msgNumber, int errorCode, const std::string &msg);
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
