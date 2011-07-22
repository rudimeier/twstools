#ifndef TWS_UTIL_H
#define TWS_UTIL_H

#include <string>
#include <stdint.h>

class QString;

namespace IB {
	class Execution;
	class Contract;
}

int64_t nowInMsecs();

QString toQString( const std::string &ibString );
std::string toIBString( const QString &qString );

std::string ibToString( int ibTickType);
std::string ibToString( const IB::Execution& );
std::string ibToString( const IB::Contract&, bool showFields = false );



#endif
