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




int main(int argc, char *argv[])
{
	twsgen_parse_cl(argc, (const char **) argv);
	
	TwsXml::setSkipDefaults( !skipdefp );
	
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
		
		bool myProp_includeExpired = true;
		int myProp_reqMaxContractsPerSpec = -1;
		QList<std::string> myProp_whatToShow = QList<std::string>() << "BID";
		std::string myProp_endDateTime = "20110214 16:00:00 America/Chicago";
		std::string myProp_durationStr = "1 W";
		std::string myProp_barSizeSetting = "30 mins";
		int myProp_useRTH = 1;
		int myProp_formatDate = 1;
		for( int i = 0; i<pcd->constList().size(); i++ ) {
			
			const IB::ContractDetails &cd = pcd->constList().at(i);
			IB::Contract c = cd.summary;
			c.includeExpired = myProp_includeExpired;
			
			if( myProp_reqMaxContractsPerSpec > 0 && myProp_reqMaxContractsPerSpec <= i ) {
				break;
			}
			
			foreach( std::string wts, myProp_whatToShow ) {
				HistRequest hR;
				hR.initialize( c, myProp_endDateTime, myProp_durationStr,
				               myProp_barSizeSetting, wts, myProp_useRTH, myProp_formatDate );
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
