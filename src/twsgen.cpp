#include "twsgen.h"
#include "config.h"

#include <popt.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


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




static int find_form_feed( const char *s, int n )
{
	int i;
	for( i=0; i<n; i++ ) {
		if( s[i] == '\f' ) {
			break;
		}
	}
	return i;
}

#define CHUNK_SIZE 1024

class XmlFile
{
	public:
		XmlFile();
		virtual ~XmlFile();
		
		bool openFile( const char *filename );
		xmlDocPtr nextXmlDoc();
	
	private:
		FILE *file;
		char *buf;
};

XmlFile::XmlFile() :
	file(NULL),
	buf( (char*) malloc(1024*1024))
{
}

XmlFile::~XmlFile()
{
	if( file != NULL ) {
		fclose(file);
	}
	free(buf);
}

bool XmlFile::openFile( const char *filename )
{
	file = fopen(filename, "rb");
	
	if( file == NULL ) {
		fprintf( stderr, "error, %s: '%s'\n", strerror(errno), filename );
		return false;
	}
	
	return true;
}

xmlDocPtr XmlFile::nextXmlDoc()
{
	xmlDocPtr doc = NULL;
	if( file == NULL ) {
		return doc;
	}
	
	char *cp = buf;
	int tmp_len;
	while( (tmp_len = fread(cp, 1, CHUNK_SIZE, file)) > 0 ) {
		int ff = find_form_feed(cp, tmp_len);
		if( ff < tmp_len ) {
			int suc = fseek( file, -(tmp_len - (ff + 1)), SEEK_CUR);
			assert( suc == 0 );
			cp += ff;
			break;
		} else {
			cp += tmp_len;
		}
	}
	
	doc = xmlReadMemory( buf, cp-buf, "URL", NULL, 0 );
	
	return doc;
}




int main(int argc, char *argv[])
{
	twsgen_parse_cl(argc, (const char **) argv);
	
	if( !histjobp ) {
		fprintf( stderr, "error, only -H is implemented\n" );
		return 1;
	}
	
	XmlFile file;
	if( ! file.openFile(filep) ) {
		return 1;
	}
	
	xmlDocPtr doc;
	int count_docs = 0;
	while( (doc = file.nextXmlDoc()) != NULL ) {
		count_docs++;
		xmlNodePtr root = doc->children;
		for( xmlNodePtr p = root->children; p!= NULL; p=p->next) {
			if( p->type == XML_ELEMENT_NODE
				&& strcmp((char*)p->name, "response") == 0 ) {
				for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
					if( q->type == XML_ELEMENT_NODE
						&& strcmp((char*)q->name, "ContractDetails") == 0 )  {
				 		printf( "%d, %s, %d\n", q->type, q->name, q->line);
					}
				}
			}
		}
	}
	fprintf( stderr, "notice, %d xml docs parsed from file '%s'\n",
		count_docs, filep );
	
	return 0;
}
