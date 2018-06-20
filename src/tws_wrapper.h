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

class TwsDL;

class TwsDlWrapper : public EWrapper
{
public:
	TwsDlWrapper( TwsDL* parent );
	virtual ~TwsDlWrapper();

	#include <twsapi/EWrapper_prototypes.h>

private:
	TwsDL* parentTwsDL;
};

#endif
