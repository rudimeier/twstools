#ifndef TWS_META_H
#define TWS_META_H

#include "ibtws/Contract.h"

#include <QtCore/QString>
#include <QtCore/QList>




namespace Test {




class HistRequest
{
	public:
		bool fromString( const QString& );
		QString toString() const;
		void clear();
		
		IB::Contract ibContract;
		QString endDateTime;
		QString whatToShow;
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
