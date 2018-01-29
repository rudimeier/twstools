/*** tws_util.h -- common utils
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_UTIL_H
#define TWS_UTIL_H

#include <string>
#include <stdint.h>

#include <twsapi/twsapi_config.h>

#ifndef TWSAPI_NO_NAMESPACE
namespace IB {
#endif
	class Execution;
	class Contract;
#ifndef TWSAPI_NO_NAMESPACE
}
using namespace IB;
#endif


int64_t nowInMsecs();
std::string msecs_to_string( int64_t msecs );

int ib_strptime( struct tm *tm, const std::string &ib_datetime );
std::string ib_date2iso( const std::string &ibDate );
std::string time_t_local( time_t t );

int ib_duration2secs( const std::string &dur );

std::string ibToString( int ibTickType);
std::string ibToString( const Execution& );
std::string ibToString( const Contract&, bool showFields = false );

const char* short_wts( const char* wts );
const char* short_bar_size( const char* bar_size );


#endif
