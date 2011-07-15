#ifndef TWS_UTIL_H
#define TWS_UTIL_H

#include <string>

class QString;

namespace IB {
	class Execution;
	class Contract;
}


QString toQString( const std::string &ibString );
std::string toIBString( const QString &qString );

QString ibToString( int ibTickType);
QString ibToString( const IB::Execution& );
QString ibToString( const IB::Contract&, bool showFields = false );



#endif
