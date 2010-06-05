#ifndef TWS_META_H
#define TWS_META_H

#include "ibtws/Contract.h"

#include <QtCore/QString>
#include <QtCore/QList>




namespace Test {




class ContractDetailsRequest
{
	public:
		bool initialize( const IB::Contract& );
		
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
		
		void nextRequest( ReqType, int todoIndex );
		
		ReqType reqType;
		ReqState reqState;
		int reqId;
		int todoIndex;
};








class WorkTodo
{
	public:
		int fromFile( const QString & fileName );
		void dump( FILE *stream ) const;
		
		QList<HistRequest> histRequests;
		
	private:
		int read_file( const QString & fileName, QList<QByteArray> *list ) const;
};




} // namespace Test
#endif
