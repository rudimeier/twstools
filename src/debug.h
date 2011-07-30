#ifndef GA_DEBUG_H
#define GA_DEBUG_H

#include "twsUtil.h"
#include <assert.h>
#include <stdio.h>




#define DEBUG_PRINTF(_format_, _args_...)  \
	fprintf (stderr, "%ld " _format_ "\n" , nowInMsecs(), ## _args_)



#endif
