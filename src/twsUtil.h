#ifndef TWS_UTIL_H
#define TWS_UTIL_H

#include <string>
#include <stdint.h>


namespace IB {
	class Execution;
	class Contract;
}

int64_t nowInMsecs();
std::string msecs_to_string( int64_t msecs );

std::string ibToString( int ibTickType);
std::string ibToString( const IB::Execution& );
std::string ibToString( const IB::Contract&, bool showFields = false );



#endif
