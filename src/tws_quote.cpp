/*** tws_quote.cpp -- TWS real time quotes
 *
 * Copyright (C) 2012 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
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

#include "tws_quote.h"
#include <twsapi/EWrapper.h>
#include <string.h>


Quote::Quote() :
	val(new double[IB::NOT_SET]),
	stamp(new int64_t[IB::NOT_SET])
{
	memset(val, 0, sizeof(val) * IB::NOT_SET);
	memset(stamp, 0, sizeof(stamp) * IB::NOT_SET);
}

Quote::Quote( const Quote& other ) :
	val(new double[IB::NOT_SET]),
	stamp(new int64_t[IB::NOT_SET])
{
	memcpy(val, other.val, sizeof(val) * IB::NOT_SET);
	memcpy(stamp, other.stamp, sizeof(stamp) * IB::NOT_SET);
}

Quote& Quote::operator=( const Quote& other )
{
	memcpy(val, other.val, sizeof(val) * IB::NOT_SET);
	memcpy(stamp, other.stamp, sizeof(stamp) * IB::NOT_SET);
	return *this;
}

Quote::~Quote()
{
	delete[] val;
	delete[] stamp;
}
