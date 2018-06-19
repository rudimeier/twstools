/*** tws_wrapper.h -- simple "IB/API wrapper" just for debugging
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWSWrapper_H
#define TWSWrapper_H

#include <twsapi/twsapi_config.h>
#include <twsapi/EWrapper.h>

#ifndef TWSAPI_NO_NAMESPACE
namespace IB {
}
using namespace IB;
#endif

/* POS_TYPE was int before 97200 */
# define POS_TYPE double

class DebugTwsWrapper : public EWrapper
{
	public:
		virtual ~DebugTwsWrapper();

	public:
		#include <twsapi/EWrapper_prototypes.h>
};

#endif
