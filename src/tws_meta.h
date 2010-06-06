#ifndef TWS_META_H
#define TWS_META_H

#include "ibtws/Contract.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QHash>




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
		
		enum ReqState {
			PENDING,
			FINISHED
		};
		
		GenericRequest();
		
		void nextRequest( ReqType );
		
		ReqType reqType;
		ReqState reqState;
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
		
		QList<IB::ContractDetails> cdList;
		
	private:
		int reqId;
};








class PacketHistData
{
	public:
		PacketHistData();
		
		bool isFinished() const;
		void clear();
		void append( int reqId, const QString &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps );
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
		
		int reqId;
		QList<Row> rows;
		Row finishRow;
};




} // namespace Test
#endif
