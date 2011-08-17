#ifndef GA_DEBUG_H
#define GA_DEBUG_H

#include "tws_util.h"
#include <assert.h>
#include <stdio.h>




#define DEBUG_PRINTF(_format_, _args_...)  \
	fprintf (stderr, "%s " _format_ "\n" , \
		msecs_to_string(nowInMsecs()).c_str(), ## _args_)



#endif
