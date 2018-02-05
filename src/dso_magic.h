/*** dso_magic.h -- interface for libltdl modules
 *
 * Copyright (C) 2012 Sebastian Freundt
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/
#if !defined INCLUDED_dso_magic_h_
#define INCLUDED_dso_magic_h_

#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
# include <ltdl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#if !defined UNUSED
# define UNUSED(x)	__attribute__((unused)) x
#endif	/* UNUSED */

typedef void(*lt_f)(void*);
typedef struct tws_dso_s *tws_dso_t;

struct tws_dso_s {
	lt_dlhandle handle;
	lt_f initf;
	lt_f finif;
	lt_f workf;
};


/* helpers */
static void
__attribute__((format(printf, 1, 2)))
error(const char *fmt, ...)
{
	va_list vap;
	va_start(vap, fmt);
	vfprintf(stderr, fmt, vap);
	va_end(vap);
	if (errno) {
		fputc(':', stderr);
		fputc(' ', stderr);
		fputs(strerror(errno), stderr);
	}
	fputc('\n', stderr);
	return;
}

static lt_dlhandle my_dlopen(const char *filename)
{
	lt_dlhandle handle = 0;
	lt_dladvise advice[1];

	if (!lt_dladvise_init(advice) &&
	    !lt_dladvise_ext(advice) &&
	    !lt_dladvise_global(advice)) {
		handle = lt_dlopenadvise( filename, advice[0] );
	}
	lt_dladvise_destroy( advice );
	return handle;
}


static tws_dso_t open_dso(const char *name, void *clo)
{
	const char minit[] = "init";
	const char mfini[] = "fini";
	const char mwork[] = "work";
	static struct tws_dso_s dso[1];

	/* initialise the dl system */
	lt_dlinit();

	if( (dso->handle = my_dlopen( name )) == NULL ) {
		error( "cannot open module `%s': %s", name, lt_dlerror() );
		return NULL;
	}

	if( (dso->initf = (lt_f)lt_dlsym( dso->handle, minit )) == NULL ) {
		lt_dlclose( dso->handle );
		error( "cannot open module `%s': init() not found", name );
		return NULL;
	}

	if( (dso->finif = (lt_f)lt_dlsym( dso->handle, mfini )) == NULL ) {
		// not an error if missing
		;
	}

	if( (dso->workf = (lt_f)lt_dlsym( dso->handle, mwork )) == NULL ) {
		// should be an error innit?
		;
	}

	/* call the init() function */
	dso->initf( clo );
	return dso;
}

static void close_dso(tws_dso_t mod, void *clo)
{
	if( mod->finif != NULL ) {
		mod->finif( clo );
	}
	lt_dlclose( mod->handle );

	mod->handle = NULL;
	mod->initf = NULL;
	mod->finif = NULL;
	mod->workf = NULL;
	return;
}

static void work_dso(tws_dso_t mod, void *clo)
{
	if( mod->workf != NULL ) {
		mod->workf( clo );
	}
	return;
}

#endif	/* INCLUDED_dso_magic_h_ */
