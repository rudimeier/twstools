#ifndef GA_DEBUG_H
#define GA_DEBUG_H

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QThread>
#include <QtCore/QVariant>



static inline QString nice_thread_name()
{
	const QThread *thr = QThread::currentThread();
	QString objectName = thr->property("objectName").toString();
	if( !objectName.isEmpty() ) {
		return objectName;
	}
	return QString(thr->metaObject()->className());
}

#define qDebug() debugGA(QDateTime::currentDateTime(),\
		nice_thread_name(),\
		QString(__PRETTY_FUNCTION__))
#define DEBUG_DATE_TIME_FORMAT "hh:mm:ss.zzz"
#define DEBUG_HEADER_LENGTH 60


QDebug debugGA(QDateTime dt,QString thr,QString cls);

void myMessageOutput(QtMsgType type, const char *msg);




inline QDebug debugGA(QDateTime dt,QString thr,QString cls)
{
	QString dstr;
	cls=cls.left(cls.indexOf('('));
	cls=cls.right(cls.size()-cls.lastIndexOf(' ')-1);
	dstr=dt.toString(DEBUG_DATE_TIME_FORMAT)+" "+thr+" "+cls;
	return QDebug(QtDebugMsg)<<dstr.leftJustified(DEBUG_HEADER_LENGTH).toAscii().data();
}


inline void myMessageOutput(QtMsgType type, const char *msg)
{
	QString msgs(msg);
	msgs=msgs.replace("\n",QString("\n")+QString(DEBUG_HEADER_LENGTH,' '));
	switch (type) {
		case QtDebugMsg:
			fprintf(stderr, "%s\n", msgs.toAscii().data());
			break;
		case QtWarningMsg:
			fprintf(stderr, "Warning: %s\n", msgs.toAscii().data());
			break;
		case QtCriticalMsg:
			fprintf(stderr, "CriticalMsg: %s\n", msgs.toAscii().data());
			break;
		case QtFatalMsg:
			fprintf(stderr, "Fatal: %s\n", msg);
			abort();
	}
}




#endif
