/*** twsdo_main.cpp -- TWS job processing tool cli
 *
 * Copyright (C) 2010-2012 Ruediger Meier
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

#include "config.h"
#include "debug.h"
#include "twsdo.h"
#include "tws_xml.h"

#include "twsdo_ggo.h"
#include "twsdo_ggo.c"


static gengetopt_args_info args_info;
static ConfigTwsdo cfg;




#define VERSION_MSG \
CMDLINE_PARSER_PACKAGE_NAME " (" PACKAGE_NAME ") " PACKAGE_VERSION " [\
built with twsapi " TWSAPI_VERSION "]\n\
Copyright (C) 2010-2012 Ruediger Meier\n\
License BSD 3-Clause\n\
\n\
Written by Ruediger Meier <sweet_f_a@gmx.de>\n"


static void check_display_args()
{
	if( args_info.help_given ) {
		gengetopt_args_info_usage =
			"Usage: " CMDLINE_PARSER_PACKAGE_NAME " [OPTION]... [JOB_FILE]";
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
		cfg.workfile = args_info.inputs[0];
	} else if( args_info.inputs_num > 1 ) {
		fprintf( stderr, "error: bad usage\n" );
		exit(2);
	}
	
	cfg.skipdef = args_info.verbose_xml_given;
	if( args_info.host_given ) {
		cfg.tws_host = args_info.host_arg;
	}
	if( args_info.port_given ) {
		cfg.tws_port = args_info.port_arg;
	}
	if( args_info.id_given ) {
		cfg.tws_client_id = args_info.id_arg;
	}
	cfg.init_ai_family( args_info.ipv4_given, args_info.ipv6_given );
	cfg.get_account = args_info.get_account_given;
	if( args_info.accountName_given ) {
		cfg.tws_account_name = args_info.accountName_arg;
	}
	cfg.get_exec = args_info.get_exec_given;
	cfg.get_order = args_info.get_order_given;
	
	if( args_info.conTimeout_given ) {
		cfg.tws_conTimeout = args_info.conTimeout_arg;
	}
	if( args_info.reqTimeout_given ) {
		cfg.tws_reqTimeout = args_info.reqTimeout_arg;
	}
	if( args_info.maxRequests_given ) {
		cfg.tws_maxRequests = args_info.maxRequests_arg;
	}
	if( args_info.pacingInterval_given ) {
		cfg.tws_pacingInterval = args_info.pacingInterval_arg;
	}
	if( args_info.minPacingTime_given ) {
		cfg.tws_minPacingTime = args_info.minPacingTime_arg;
	}
	if( args_info.violationPause_given ) {
		cfg.tws_violationPause = args_info.violationPause_arg;
	}

	// DSO loading
	if( args_info.strat_given ) {
#if defined HAVE_LTDL
		cfg.strat_file = args_info.strat_arg;
#else  // !HAVE_LTDL
		perror("module loading requested but there is no ltdl support");
#endif	// HAVE_LTDL
	}
}

static void gengetopt_free()
{
	cmdline_parser_free( &args_info );
}


int main(int argc, char *argv[])
{
	atexit( gengetopt_free );
	
	if( cmdline_parser(argc, argv, &args_info) != 0 ) {
		return 2; // exit
	}
	
	check_display_args();
	gengetopt_check_opts();
	
	TwsXml::setSkipDefaults( !cfg.skipdef );
	
	TwsDL twsDL( cfg );
	int ret = twsDL.start();
	
	if( ret != 0 ) {
		DEBUG_PRINTF( "error: %s", twsDL.lastError().c_str() );
	} else {
		DEBUG_PRINTF( "%s", twsDL.lastError().c_str() );
	}
	return ret;
}
