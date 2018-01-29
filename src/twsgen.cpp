/*** twsgen.cpp -- twsxml converter and job generator
 *
 * Copyright (C) 2011-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_xml.h"
#include "tws_meta.h"
#include "tws_query.h"
#include "debug.h"
#include "version.h"
#include "config.h"

#include <twsapi/twsapi_config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libxml/tree.h>

#include "twsgen_ggo.h"

#if TWSAPI_IB_VERSION_NUMBER < 97200
# define lastTradeDateOrContractMonth expiry
#endif

static gengetopt_args_info args_info;

static const char *filep = NULL;
static int skipdefp = 0;
static int histjobp = 0;
static const char *endDateTimep = "";
static const char *durationStrp = NULL;
static const char *barSizeSettingp = "1 hour";
static const char *whatToShowp = "TRADES";
static int useRTHp = 0;
static int utcp = 0;
static const char *includeExpiredp = "auto";
static int to_csvp = 0;
static int no_convp = 0;
static const char *max_expiryp = NULL;


static char** wts_list = NULL;
static char* wts_split = NULL;

enum expiry_mode{ EXP_AUTO, EXP_ALWAYS, EXP_NEVER, EXP_KEEP };
static int exp_mode = -1;

static int formatDate()
{
	if( utcp ) {
		return 2;
	}
	return 1;
}


#define VERSION_MSG \
CMDLINE_PARSER_PACKAGE_NAME " (" PACKAGE_NAME ") %s [\
built with twsapi " TWSAPI_VERSION "]\n\
Copyright (C) 2010-2018 Ruediger Meier\n\
License BSD 3-Clause\n\
\n\
Written by Ruediger Meier <sweet_f_a@gmx.de>\n"


static void check_display_args()
{
	if( args_info.help_given ) {
		gengetopt_args_info_usage =
			"Usage: " CMDLINE_PARSER_PACKAGE_NAME " [OPTION]... [FILE]";
		cmdline_parser_print_help();
	} else if( args_info.usage_given ) {
		printf( "%s\n", gengetopt_args_info_usage );
	} else if( args_info.version_given ) {
		printf( VERSION_MSG, twstools_version_string );
 	} else {
		return;
	}

	exit(0);
}

static void gengetopt_check_opts()
{
	if( args_info.inputs_num == 1 ) {
		filep = args_info.inputs[0];
	} else if( args_info.inputs_num > 1 ) {
		fprintf( stderr, "error: bad usage\n" );
		exit(2);
	}

	skipdefp = args_info.verbose_xml_given;
	histjobp = args_info.histjob_given;
	if( args_info.endDateTime_given ) {
		endDateTimep = args_info.endDateTime_arg;
	}
	if( args_info.durationStr_given ) {
		durationStrp = args_info.durationStr_arg;
	}
	if( args_info.barSizeSetting_given ) {
		barSizeSettingp = args_info.barSizeSetting_arg;
	}
	if( args_info.whatToShow_given ) {
		whatToShowp = args_info.whatToShow_arg;
	}
	useRTHp = args_info.useRTH_given;
	utcp = args_info.utc_given;
	if( args_info.includeExpired_given ) {
		includeExpiredp = args_info.includeExpired_arg;
	}
	to_csvp = args_info.to_csv_given;
	no_convp = args_info.no_conv_given;
	if( args_info.max_expiry_given ) {
		max_expiryp = args_info.max_expiry_arg;
	}
}

static void gengetopt_free()
{
	cmdline_parser_free( &args_info );
}


const char* max_durationStr( const char* barSizeSetting )
{
/* valid bars: (1|5|15|30) secs, 1 min, (2|3|5|15|30) mins, 1 hour, 4 hours
   seems to be not supported anymore: 1 day, 1 week, 1 month, 3 months, 1 year
   valid durations: integer{SPACE}unit (S|D|W|M|Y) */
	// TODO

	if( strcmp( barSizeSetting, "1 secs")==0 ) {
		return "2000 S";
	} else if( strcasecmp( barSizeSetting, "5 secs")==0 ) {
		return "10000 S";
	} else if( strcasecmp( barSizeSetting, "15 secs")==0 ) {
		return "30000 S";
	} else if( strcasecmp( barSizeSetting, "30 secs")==0 ) {
		return "86400 S"; // seems to get more that "1 D"
	} else if( strcasecmp( barSizeSetting, "1 min")==0 ) {
		return "6 D";
	} else if( strcasecmp( barSizeSetting, "2 mins")==0 ) {
		return "6 D";
	} else if( strcasecmp( barSizeSetting, "3 mins")==0 ) {
		return "6 D";
	} else if( strcasecmp( barSizeSetting, "5 mins")==0 ) {
		return "6 D";
	} else if( strcasecmp( barSizeSetting, "15 mins")==0 ) {
		return "20 D";
	} else if( strcasecmp( barSizeSetting, "30 mins")==0 ) {
		return "34 D";
	} else if( strcasecmp( barSizeSetting, "1 hour")==0 ) {
		return "34 D";
	} else if( strcasecmp( barSizeSetting, "4 hours")==0 ) {
		return "34 D";
	} else if( strcasecmp( barSizeSetting, "1 day")==0 ) {
		/* Is "1 Y" always better than "52 W" or "12 M"? Note that longer
		   requests will be rejected from local TWS itself! */
		return "1 Y";
	} else {
		fprintf( stderr, "error, could not guess durationStr from unknown "
			"--barSizeSetting '%s', use a known one or overide with "
			"--durationStr\n", barSizeSettingp );
			exit(2);
	}
}

void set_includeExpired()
{
	if( strcmp( includeExpiredp, "auto")==0 ) {
		exp_mode = EXP_AUTO;
	} else if( strcmp( includeExpiredp, "always")==0 ) {
		exp_mode = EXP_ALWAYS;
	} else if( strcmp( includeExpiredp, "never")==0 ) {
		exp_mode = EXP_NEVER;
	} else if( strcmp( includeExpiredp, "keep")==0 ) {
		exp_mode = EXP_KEEP;
	} else {
		fprintf( stderr, "error, unknow argument '%s'\n", includeExpiredp );
		exit(2);
	}
}

bool get_inc_exp( const char* secType )
{
	switch( exp_mode ) {
	case EXP_AUTO:
		/* seems that includeExpired works for FUT only, however we set it for
		   all secTypes where TWS doesn't return errors */
		if( strcasecmp(secType, "OPT")==0 || strcasecmp(secType, "FUT")==0 ||
			strcasecmp(secType, "FOP")==0 || strcasecmp(secType, "WAR")==0 ) {
			return true;
		}
		return false;
	case EXP_ALWAYS:
		return true;
	case EXP_NEVER:
		return false;
	default:
		assert(false);
		return false;
	}
}

void split_whatToShow()
{
	size_t len = strlen(whatToShowp) + 1;
	wts_list = (char**) malloc( (len/2 + 1) * sizeof(char*) );
	wts_split = (char*) malloc( len * sizeof(char) );
	strcpy( wts_split, whatToShowp );

	char **wts = wts_list;
	char *token = strtok( wts_split, ", \t" );
	*wts = token;
	wts++;
	while( token != NULL ) {
		token = strtok( NULL, ", \t" );
		*wts = token;
		wts++;
	}
	assert( *(wts - 1) == NULL );
	assert( (wts - wts_list) <= (ssize_t)(len/2 + 1) );
}




/* frist _business_ day data is expected, TODO review */
time_t min_begin_date( const std::string &end, const std::string dur )
{
	struct tm tm_tmp;
	time_t t_begin;

	memset(&tm_tmp, 0, sizeof(struct tm));
	if( ib_strptime( &tm_tmp, end ) == -1 ) {
		// TODO use current date time
		return 0;
	}
	tm_tmp.tm_isdst = -1; // set "auto dst", strptime() does not set it

	int minus_busy_days = ib_duration2secs( dur );
	if( minus_busy_days == -1  ) {
		return 0;
	}
	minus_busy_days /= 86400;

	/* saturday or sunday jumps to friday for free */
	if( tm_tmp.tm_wday == 0 ) {
		tm_tmp.tm_mday -= 2;
		tm_tmp.tm_wday = 5;
		minus_busy_days--;
	} else if( tm_tmp.tm_wday == 6 ) {
		tm_tmp.tm_mday -= 1;
		tm_tmp.tm_wday = 5;
		minus_busy_days--;
	}

	/* jump over full weeks */
	tm_tmp.tm_mday -= 7 * (minus_busy_days / 5);
	minus_busy_days = minus_busy_days % 5;

	while( minus_busy_days > 0 ) {
		if( tm_tmp.tm_wday == 1 ) {
			/* monday jumps to friday */
			tm_tmp.tm_mday -= 3;
			tm_tmp.tm_wday = 5;
		} else {
			tm_tmp.tm_mday -= 1;
			tm_tmp.tm_wday -= 1;
		}
		minus_busy_days--;
	}

	t_begin = mktime( &tm_tmp );
	assert( t_begin != -1 );

	return t_begin;
}


bool skip_expiry( const std::string &expiry, time_t t_before )
{
	struct tm tm_tmp;
	time_t t_expiry;

	if( expiry.empty() ) {
		return false;
	}

	// expiry
	memset(&tm_tmp, 0, sizeof(struct tm));
	if( ib_strptime( &tm_tmp, expiry.c_str() ) == -1 ) {
		assert(false);
	}
	tm_tmp.tm_isdst = -1; // set "auto dst", strptime() does not set it
	t_expiry = mktime( &tm_tmp );
	assert( t_expiry != -1 );

	return ( t_expiry < t_before);
}


bool gen_hist_job()
{
	TwsXml file;
	if( ! file.openFile(filep) ) {
		return false;
	}

	time_t t_begin = min_begin_date( endDateTimep, durationStrp );
	DEBUG_PRINTF("skipping expiries before: '%s'",
		time_t_local(t_begin).c_str() );

	xmlNodePtr xn;
	int count_docs = 0;
	/* NOTE We are dumping single HistRequests but we should build and dump
	   a HistTodo object */
	while( (xn = file.nextXmlNode()) != NULL ) {
		count_docs++;
		PacketContractDetails *pcd = PacketContractDetails::fromXml( xn );

		int myProp_reqMaxContractsPerSpec = -1;
		for( size_t i = 0; i < pcd->constList().size(); i++ ) {

			const ContractDetails &cd = pcd->constList()[i];
			Contract c = cd.summary;
// 			DEBUG_PRINTF("contract, %s %s", c.symbol.c_str(), c.expiry.c_str());

			if( exp_mode != EXP_KEEP ) {
				c.includeExpired = get_inc_exp(c.secType.c_str());
			}
			if( myProp_reqMaxContractsPerSpec > 0 && (size_t)myProp_reqMaxContractsPerSpec <= i ) {
				break;
			}

			if( skip_expiry(c.lastTradeDateOrContractMonth, t_begin) ) {
// 				DEBUG_PRINTF("skipping expiry: '%s'", c.expiry.c_str());
				continue;
			}

			for( char **wts = wts_list; *wts != NULL; wts++ ) {
				HistRequest hR;
				hR.initialize( c, endDateTimep, durationStrp, barSizeSettingp,
				               *wts, useRTHp, formatDate() );

				PacketHistData phd;
				phd.record( 0, hR );
				phd.dumpXml();
			}
		}
		delete pcd;
	}
	fprintf( stderr, "notice, %d xml docs parsed from file '%s'\n",
		count_docs, filep );

	return true;
}


bool gen_csv()
{
	TwsXml file;
	if( ! file.openFile(filep) ) {
		return false;
	}

	if( max_expiryp != NULL ) {
		struct tm tm_tmp;
		memset(&tm_tmp, 0, sizeof(struct tm));
		if( ib_strptime( &tm_tmp, max_expiryp ) == -1 ) {
			fprintf( stderr, "error, "
				"max-expiry must be IB's format YYYYMMDD.\n" );
			return false;
		}
		DEBUG_PRINTF("skipping expiries newer than: '%s'", max_expiryp );
	}

	xmlNodePtr xn;
	int count_docs = 0;
	while( (xn = file.nextXmlNode()) != NULL ) {
		count_docs++;
		PacketHistData *phd = PacketHistData::fromXml( xn );

		if( max_expiryp != NULL ) {
			const std::string &expiry = phd->getRequest().ibContract.lastTradeDateOrContractMonth;
			if( !expiry.empty() && strcmp(max_expiryp, expiry.c_str() ) < 0 ) {
				goto cont;
			}
		}

		if( !no_convp ) {
			phd->dump( true /* printFormatDates */);
		} else {
			phd->dumpXml();
		}
cont:
		delete phd;
	}
	fprintf( stderr, "notice, %d xml docs parsed from file '%s'\n",
		count_docs, filep );

	return true;
}


int main(int argc, char *argv[])
{
	atexit( gengetopt_free );

	if( cmdline_parser(argc, argv, &args_info) != 0 ) {
		return 2; // exit
	}

	check_display_args();
	gengetopt_check_opts();

	TwsXml::setSkipDefaults( !skipdefp );
	if( !durationStrp ) {
		durationStrp = max_durationStr( barSizeSettingp );
	}
	split_whatToShow();
	set_includeExpired();

	if( histjobp ) {
		if( !gen_hist_job() ) {
			return 1;
		}
	} else if( to_csvp ) {
		if( !gen_csv() ) {
			return 1;
		}
	} else {
		fprintf( stderr, "error, nothing to do, use -H or -C.\n" );
		return 2;
	}

	return 0;
}
