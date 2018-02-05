/*** tws_account.h -- TWS portfolio and account state
 *
 * Copyright (C) 2012-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_ACCOUNT_H
#define TWS_ACCOUNT_H

#include "tws_meta.h"
#include <map>


typedef std::map<long, RowPrtfl> Prtfl;
typedef std::map<long, RowOpenOrder> OpenOrders;
typedef std::map<long, RowOrderStatus> OrderStatus;


class Account
{
	public:
		Account();
		~Account();

	void updatePortfolio( const RowPrtfl& row );
	void update_oo( const RowOpenOrder& row );
	void update_os( const RowOrderStatus& row );

// 	private:
		Prtfl portfolio;
		OpenOrders openOrders;
		OrderStatus orderStatus;
};


#endif
