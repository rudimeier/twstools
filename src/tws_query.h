/*** tws_query.h -- structs for IB/API requests
 *
 * Copyright (C) 2010-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_QUERY_H
#define TWS_QUERY_H

#include <twsapi/Contract.h>
#include <twsapi/Execution.h>
#include <twsapi/Order.h>
#include <stdint.h>


class ContractDetailsRequest
{
	public:
		const IB::Contract& ibContract() const;
		bool initialize( const IB::Contract& );

	private:
		IB::Contract _ibContract;
};




class HistRequest
{
	public:
		HistRequest();

		bool initialize( const IB::Contract&, const std::string &endDateTime,
			const std::string &durationStr, const std::string &barSizeSetting,
			const std::string &whatToShow, int useRTH, int formatDate );
		std::string toString() const;

		IB::Contract ibContract;
		std::string endDateTime;
		std::string durationStr;
		std::string barSizeSetting;
		std::string whatToShow;
		int useRTH;
		int formatDate;
};




class AccStatusRequest
{
	public:
		AccStatusRequest();

		bool subscribe;
		std::string acctCode;
};




class ExecutionsRequest
{
	public:
		IB::ExecutionFilter executionFilter;
};




class OrdersRequest
{
};




class PlaceOrder
{
	public:
		PlaceOrder();

		long orderId;
		int64_t time_sent;
		IB::Contract contract;
		IB::Order order;
};




class MktDataRequest
{
	public:
		MktDataRequest();

		IB::Contract ibContract;
		std::string genericTicks;
		bool snapshot;
};


#endif
