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








class HistTodo
{
	public:
		int fromFile( const QString & fileName );
		void dump( FILE *stream ) const;
		
		QList<HistRequest> histRequests;
		
	private:
		int read_file( const QString & fileName, QList<QByteArray> *list ) const;
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
		PacingControl();
		
		void setPacingTime( int min, int avg );
		void setViolationPause( int avg );
		
		void clear();
		void addRequest();
		void setViolation();
		int goodTime() const;
		
	private:
		static quint64 nowInMsecs();
		
		QList<quint64> dateTimes;
		QList<bool> violations;
		
		int checkInterval;
		int minPacingTime;
		int avgPacingTime;
		int violationPause;
};








class DataFarmStates
{
	public:
		enum State { UNKNOWN, BROKEN, INACTIVE, OK };
		
		void notify(int errorCode, const QString &msg);
		
	private:
		static QString getFarm( const QString prefix, const QString& msg );
		
		QHash<const QString, State> mStates;
		QHash<const QString, State> hStates;
};




} // namespace Test
#endif
