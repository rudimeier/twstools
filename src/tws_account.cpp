/*** tws_account.cpp -- TWS portfolio and account state
 *
 * Copyright (C) 2012-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#include "tws_account.h"
#include <assert.h>


Account::Account()
{
}

Account::~Account()
{
}

void Account::updatePortfolio( const RowPrtfl& row )
{
	long conid = row.contract.conId;
	assert( conid > 0);

	portfolio[conid] = row;
}

void Account::update_oo( const RowOpenOrder& row )
{
	long permid = row.order.permId;
	assert( permid > 0);

	openOrders[permid] = row;
}

void Account::update_os( const RowOrderStatus& row )
{
	long permid = row.permId;
	assert( permid > 0);

	orderStatus[permid] = row;
}

