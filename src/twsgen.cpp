#include "twsgen.h"
#include "config.h"

#include <popt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


static poptContext opt_ctx;
static const char *filep = NULL;
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
	if( rest != NULL ) {
		if( *rest != NULL ) {
			filep = *rest;
			rest++;
		}
		if( *rest != NULL ) {
			fprintf( stderr, "error: bad usage\n" );
			exit(2);
		}
	}
}




int main(int argc, char *argv[])
{
	twsgen_parse_cl(argc, (const char **) argv);
	
	if( histjobp ) {
		printf("generate hist hob from file '%s'\n", filep);
	}
	
	return 0;
}
