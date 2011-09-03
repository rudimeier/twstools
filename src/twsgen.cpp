/*** twsgen.cpp -- twsxml converter and job generator
 *
 * Copyright (C) 2011 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 *
 * This file is part of twstools.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/

#include "twsgen_ggo.h"
#include "tws_xml.h"
#include "tws_meta.h"
#include "debug.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libxml/tree.h>

#include "twsgen_ggo.c"


static gengetopt_args_info args_info;

enum { POPT_HELP, POPT_VERSION, POPT_USAGE };

static const char *filep = "";
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
PACKAGE_NAME " " PACKAGE_VERSION "\n\
Copyright (C) 2010-2011 Ruediger Meier <sweet_f_a@gmx.de>\n\
License: BSD 3-Clause\n"


static void check_display_args()
{
	if( args_info.help_given ) {
		gengetopt_args_info_usage =
			"Usage: " PACKAGE " [OPTION]... [WORK_FILE]";
		cmdline_parser_print_help();
	} else if( args_info.usage_given ) {
		printf( "%s\n", gengetopt_args_info_usage );
	} else if( args_info.version_given ) {
		printf( VERSION_MSG );
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


bool gen_hist_job()
{
	TwsXml file;
	if( ! file.openFile(filep) ) {
		return false;
	}
	
	xmlNodePtr xn;
	int count_docs = 0;
	/* NOTE We are dumping single HistRequests but we should build and dump
	   a HistTodo object */
	while( (xn = file.nextXmlNode()) != NULL ) {
		count_docs++;
		PacketContractDetails *pcd = PacketContractDetails::fromXml( xn );
		
		int myProp_reqMaxContractsPerSpec = -1;
		for( size_t i = 0; i < pcd->constList().size(); i++ ) {
			
			const IB::ContractDetails &cd = pcd->constList()[i];
			IB::Contract c = cd.summary;
			if( exp_mode != EXP_KEEP ) {
				c.includeExpired = get_inc_exp(c.secType.c_str());
			}
			if( myProp_reqMaxContractsPerSpec > 0 && (size_t)myProp_reqMaxContractsPerSpec <= i ) {
				break;
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
	xmlNodePtr xn;
	int count_docs = 0;
	while( (xn = file.nextXmlNode()) != NULL ) {
		count_docs++;
		PacketHistData *phd = PacketHistData::fromXml( xn );
		if( !no_convp ) {
			phd->dump( true /* printFormatDates */);
		} else {
			phd->dumpXml();
		}
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
