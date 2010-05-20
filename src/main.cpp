#include "twsDL.h"
#include "utilities/debug.h"

#include <QtCore/QCoreApplication>




int main(int argc, char *argv[])
{
	QCoreApplication app( argc, argv );
	Test::Worker worker;
	QObject::connect( &worker, SIGNAL(finished()), &app, SLOT(quit()) );
	worker.start();
	int ret = app.exec();
	Q_ASSERT( ret == 0 );
	
	Test::Worker::State state = worker.currentState();
	Q_ASSERT( (state == Test::Worker::QUIT_READY) ||
		(state == Test::Worker::QUIT_ERROR) );
	if( state == Test::Worker::QUIT_READY ) {
		return 0;
	} else {
		qDebug() << "Finished with errors.";
		return 1;
	}
}
