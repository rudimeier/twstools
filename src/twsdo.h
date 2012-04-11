/*** twsdo.h -- TWS job processing tool
 *
 * Copyright (C) 2010-2012 Ruediger Meier
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

#ifndef TWS_DL_H
#define TWS_DL_H

#include <string>
#include <stdint.h>




class TWSClient;

namespace IB {
	class ContractDetails;
	class Contract;
	class Execution;
}



struct ConfigTwsdo
{
	ConfigTwsdo();

	void init_ai_family( int ipv4, int ipv6 );

	const char *workfile;
	int skipdef;
	const char *tws_host;
	int tws_port;
	int tws_client_id;
	int ai_family;
	
	int get_account;
	const char* tws_account_name;
	int get_exec;
	int get_order;
	int do_mm;
	
	int tws_conTimeout;
	int tws_reqTimeout;
	int tws_maxRequests;
	int tws_pacingInterval;
	int tws_minPacingTime;
	int tws_violationPause;
};




class ContractDetailsRequest;
class HistRequest;
class GenericRequest;
class WorkTodo;
class ContractDetailsTodo;
class HistTodo;
class Packet;
class RowError;
class RowHist;
class RowAccVal;
class RowPrtfl;
class RowExecution;
class RowOrderStatus;
class RowOpenOrder;
class PacingGod;
class DataFarmStates;

class TwsDlWrapper;




class TwsDL
{
	public:
		enum State {
			WAIT_TWS_CON,
			IDLE,
			WAIT_DATA,
			QUIT
		};
		
		TwsDL( const ConfigTwsdo& );
		~TwsDL();
		
		int start();
		
		State currentState() const;
		std::string lastError() const;
		
	private:
		void initTwsClient();
		void eventLoop();
		
		void dumpWorkTodo() const;
		
		void connectTws();
		void waitTwsCon();
		void idle();
		bool finContracts();
		bool finHist();
		bool finPlaceOrder();
		bool finCancelOrder();
		void waitData();
		
		void changeState( State );
		
		void initWork();
		
		void reqContractDetails();
		void reqHistoricalData();
		void reqAccStatus();
		void reqExecutions();
		void reqOrders();
		void placeOrder();
		void cancelOrder();
		int reqMktData();
		
		void errorContracts( const RowError& );
		void errorHistData( const RowError& );
		void errorPlaceOrder( const RowError& );
		void errorCancelOrder( const RowError& );
		
		// callbacks from our twsWrapper
		void twsError( const RowError& );
		
		void twsConnectionClosed();
		void twsContractDetails( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void twsBondContractDetails( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void twsContractDetailsEnd( int reqId );
		void twsHistoricalData( int reqId, const RowHist& );
		void twsUpdateAccountValue( const RowAccVal& );
		void twsUpdatePortfolio( const RowPrtfl& );
		void twsUpdateAccountTime( const std::string& timeStamp );
		void twsAccountDownloadEnd( const std::string& accountName );
		void twsExecDetails( int reqId, const RowExecution& );
		void twsExecDetailsEnd( int reqId );
		void twsOrderStatus( const RowOrderStatus& );
		void twsOpenOrder( const RowOpenOrder& );
		void twsOpenOrderEnd();
		void twsCurrentTime( long time );
		void nextValidId( long orderId );
		
		
		State state;
		int error;
		std::string _lastError;
		int64_t lastConnectionTime;
		long tws_time;
		long tws_valid_orderId;
		bool connectivity_IB_TWS;
		int curIdleTime;
		
		ConfigTwsdo cfg;
		
		TwsDlWrapper *twsWrapper;
		TWSClient  *twsClient;
		
		int msgCounter;
		GenericRequest &currentRequest;
		
		WorkTodo *workTodo;
		
		Packet *packet;
		
		DataFarmStates &dataFarms;
		PacingGod &pacingControl;
		
	friend class TwsDlWrapper;
};




#endif
