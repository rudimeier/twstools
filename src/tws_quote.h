/*** tws_quote.h -- TWS real time quotes
 *
 * Copyright (C) 2012-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_QUOTE_H
#define TWS_QUOTE_H

#include <stdint.h>
#include <vector>

class Quote
{
	public:
		Quote();
		Quote( const Quote& );
		~Quote();

		Quote& operator= (const Quote&);

		double * const val;
		int64_t * const stamp;
};

typedef std::vector<Quote> Quotes;

#endif
