#ifndef GA_DEBUG_H
#define GA_DEBUG_H

#include "twsUtil.h"
#include <assert.h>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>




#define qDebug() debugGA(QDateTime::currentDateTime(),\
		QString(__PRETTY_FUNCTION__))
#define DEBUG_DATE_TIME_FORMAT "hh:mm:ss.zzz"
#define DEBUG_HEADER_LENGTH 60


QDebug debugGA(QDateTime dt,QString thr,QString cls);




inline QDebug debugGA(QDateTime dt,QString cls)
{
	QString dstr;
	cls=cls.left(cls.indexOf('('));
	cls=cls.right(cls.size()-cls.lastIndexOf(' ')-1);
	dstr=dt.toString(DEBUG_DATE_TIME_FORMAT) + " " + cls;
	return QDebug(QtDebugMsg)<<dstr.leftJustified(DEBUG_HEADER_LENGTH).toAscii().data();
}


#define DEBUG_PRINTF(_format_, _args_...)  \
	fprintf (stderr, "%ld " _format_ "\n" , nowInMsecs(), ## _args_)



#endif
