#include "twsgen.h"
#include "tws_xml.h"
#include "tws_meta.h"
#include "config.h"

#include <popt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <libxml/tree.h>


static poptContext opt_ctx;
static const char *filep = NULL;
static int skipdefp = 0;
static int histjobp = 0;
static const char *endDateTimep = "";
static const char *durationStrp = NULL;
static const char *barSizeSettingp = "1 hour";
static const char *whatToShowp = "TRADES";
static const char *includeExpiredp = "auto";

static char** wts_list = NULL;
static char* wts_split = NULL;


#define VERSION_MSG \
PACKAGE_NAME " " PACKAGE_VERSION "\n\
Copyright (C) 2010-2011 Ruediger Meier <sweet_f_a@gmx.de>\n\
License: BSD 3-Clause\n"


static void displayArgs( poptContext con, poptCallbackReason /*foo*/,
	poptOption *key, const char */*arg*/, void */*data*/ )
{
	if (key->shortName == 'h') {
		poptPrintHelp(con, stdout, 0);
	} else if (key->shortName == 'V') {
		fprintf(stdout, VERSION_MSG);
	} else {
		poptPrintUsage(con, stdout, 0);
	}
	
	exit(0);
}

static struct poptOption flow_opts[] = {
	{"verbose-xml", 'x', POPT_ARG_NONE, &skipdefp, 0,
		"Never skip xml default values.", NULL},
	{"histjob", 'H', POPT_ARG_NONE, &histjobp, 0,
		"generate hist job", "FILE"},
	{"endDateTime", 'e', POPT_ARG_STRING, &endDateTimep, 0,
		"Query end date time, default is \"\" which means now.", "DATETIME"},
	{"durationStr", 'd', POPT_ARG_STRING, &durationStrp, 0,
		"Query duration, default is maximum dependent on bar size.", NULL},
	{"barSizeSetting", 'b', POPT_ARG_STRING, &barSizeSettingp, 0,
		"Size of the bars, default is \"1 hour\".", NULL},
	{"whatToShow", 'w', POPT_ARG_STRING, &whatToShowp, 0,
		"List of data types, valid types are: TRADES, BID, ASK, etc., "
		"default: TRADES", "LIST"},
	{"includeExpired", '\0', POPT_ARG_STRING, &includeExpiredp, 0,
		"How to set includeExpired, valid args: auto, always, never, keep. "
		"Default is auto (dependent on secType).", NULL},
	POPT_TABLEEND
};

static struct poptOption help_opts[] = {
	{NULL, '\0', POPT_ARG_CALLBACK, (void*)displayArgs, 0, NULL, NULL},
	{"help", 'h', POPT_ARG_NONE, NULL, 0, "Show this help message.", NULL},
	{"version", 'V', POPT_ARG_NONE, NULL, 0, "Print version string and exit.",
		NULL},
	{"usage", '\0', POPT_ARG_NONE, NULL, 0, "Display brief usage message."
		, NULL},
	POPT_TABLEEND
};

static const struct poptOption twsDL_opts[] = {
	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, flow_opts, 0,
	 "Program advice:", NULL},
	{NULL, '\0', POPT_ARG_INCLUDE_TABLE, help_opts, 0,
	 "Help options:", NULL},
	POPT_TABLEEND
};

void clear_popt()
{
	poptFreeContext(opt_ctx);
}

void twsgen_parse_cl(size_t argc, const char *argv[])
{
	opt_ctx = poptGetContext(NULL, argc, argv, twsDL_opts, 0);
	atexit(clear_popt);
	
	poptSetOtherOptionHelp( opt_ctx, "[OPTION]... FILE");
	
	int rc;
	while( (rc = poptGetNextOpt(opt_ctx)) > 0 ) {
		// handle options when we have returning ones
		assert(false);
	}
	
	if( rc != -1 ) {
		fprintf( stderr, "error: %s '%s'\n",
			poptStrerror(rc), poptBadOption(opt_ctx, 0) );
		exit(2);
	}
	
	const char** rest = poptGetArgs(opt_ctx);
	if( rest != NULL && *rest != NULL ) {
		filep = *rest;
		rest++;
		
		if( *rest != NULL ) {
			fprintf( stderr, "error: bad usage\n" );
			exit(2);
		}
	} else {
			fprintf( stderr, "error: bad usage\n" );
			exit(2);
	}
}


const char* max_durationStr( const char* bar_size )
{
	// TODO
	return "3 D";
}

bool get_includeExpired( const char* sec_type )
{
	// TODO
	return 1;
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
	assert( (wts - wts_list) <= (len/2 + 1) );
}


int main(int argc, char *argv[])
{
	twsgen_parse_cl(argc, (const char **) argv);
	
	TwsXml::setSkipDefaults( !skipdefp );
	if( !durationStrp ) {
		durationStrp = max_durationStr( barSizeSettingp );
	}
	split_whatToShow();
	
	if( !histjobp ) {
		fprintf( stderr, "error, only -H is implemented\n" );
		return 1;
	}
	
	TwsXml file;
	if( ! file.openFile(filep) ) {
		return 1;
	}
	
	xmlNodePtr xn;
	int count_docs = 0;
	HistTodo histTodo;
	while( (xn = file.nextXmlNode()) != NULL ) {
		count_docs++;
		PacketContractDetails *pcd = PacketContractDetails::fromXml( xn );
		
		int myProp_reqMaxContractsPerSpec = -1;
		int myProp_useRTH = 1;
		int myProp_formatDate = 1;
		for( int i = 0; i<pcd->constList().size(); i++ ) {
			
			const IB::ContractDetails &cd = pcd->constList().at(i);
			IB::Contract c = cd.summary;
			c.includeExpired = get_includeExpired( c.secType.c_str() );
			
			if( myProp_reqMaxContractsPerSpec > 0 && myProp_reqMaxContractsPerSpec <= i ) {
				break;
			}
			
			for( char **wts = wts_list; *wts != NULL; wts++ ) {
				HistRequest hR;
				hR.initialize( c, endDateTimep, durationStrp, barSizeSettingp,
				               *wts, myProp_useRTH, myProp_formatDate );
				histTodo.add( hR );
				
				PacketHistData phd;
				phd.record( 0, hR );
				phd.dumpXml();
			}
		}
		delete pcd;
	}
	// TODO this should be xml dump
	histTodo.dumpLeft( stderr );
	fprintf( stderr, "notice, %d xml docs parsed from file '%s'\n",
		count_docs, filep );
	
	return 0;
}
