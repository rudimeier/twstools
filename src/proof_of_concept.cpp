/*** proof_of_concept.cpp -- a no-op strategy module for twsdo
 *
 * Copyright (C) 2012 Sebastian Freundt
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/
#include <stdio.h>

extern "C" {
/* libtool needs C symbols */
extern void init(void*);
extern void fini(void*);
extern void work(void*);
}


void init(void *clo)
{
	// we *know* that CLO is indeed a TwsDL instance but there's no
	// way to get it it seems
	fprintf( stderr, "init() called on class %p\n", clo );
	return;
}

void fini(void *clo)
{
	fprintf( stderr, "fini() called on class %p\n", clo );
	return;
}

void work(void *clo)
{
	fprintf( stderr, "work() called on class %p\n", clo );
	return;
}

/* proof_of_concept.cpp ends here */
