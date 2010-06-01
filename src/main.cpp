#include "twsDL.h"
#include "utilities/debug.h"

#include <QtCore/QCoreApplication>
#include <stdio.h>



int main(int argc, char *argv[])
{
	QCoreApplication app( argc, argv );
	
	if( argc > 3 ) {
		fprintf( stderr, "Usage: %s [configuration file] [worktodo file]\n", argv[0] );
		return 2;
	}
	QString workfile;
	if( argc == 2 ) {
		workfile = argv[2];
	}
	Test::Worker worker( argc == 2 ? argv[1] : "twsDL.cfg", workfile );
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
