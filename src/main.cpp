#include "twsDL.h"
#include "debug.h"

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
	if( argc == 3 ) {
		workfile = argv[2];
	}
	Test::TwsDL twsDL( argc >= 2 ? argv[1] : "twsDL.cfg", workfile );
	QObject::connect( &twsDL, SIGNAL(finished()), &app, SLOT(quit()) );
	twsDL.start();
	int ret = app.exec();
	Q_ASSERT( ret == 0 );
	
	Test::TwsDL::State state = twsDL.currentState();
	Q_ASSERT( (state == Test::TwsDL::QUIT_READY) ||
		(state == Test::TwsDL::QUIT_ERROR) );
	if( state == Test::TwsDL::QUIT_READY ) {
		return 0;
	} else {
		qDebug() << "Finished with errors.";
		return 1;
	}
}
