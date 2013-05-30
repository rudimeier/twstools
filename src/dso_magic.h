/*** dso_magic.h -- interface for libltdl modules
 *
 * Copyright (C) 2012 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
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
