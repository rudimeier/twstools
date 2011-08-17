/*** tws_client.h -- IB/API TWSClient class
 *
 * Copyright (C) 2010, 2011 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 *
 * This file is part of atem.
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

#ifndef TWS_CLIENT_H
#define TWS_CLIENT_H

#include <string>


namespace IB {
	class Contract;
	class ContractDetails;
	class Order;
	class OrderState;
	class Execution;
	class ExecutionFilter;
	class EPosixClientSocket;
	class EWrapper;
};


class TWSClient
{
	public:
		TWSClient( IB::EWrapper *ew );
		~TWSClient();
		
		bool isConnected() const;
		
		void selectStuff( int msec );
		
		/////////////////////////////////////////////////////
		bool connectTWS( const std::string &host, int port, int clientId );
		void disconnectTWS();
		
		int serverVersion();
		std::string TwsConnectionTime();
		
		void reqMktData( int tickerId, const IB::Contract &contract, const std::string &genericTickList, bool snapshot );
		void cancelMktData( int tickerId );
		void placeOrder( int id, const IB::Contract &contract, const IB::Order &order );
		void cancelOrder( int id );
		void reqOpenOrders();
		void reqAllOpenOrders();
		void reqAutoOpenOrders( bool bAutoBind );
		void reqExecutions(int reqId, const IB::ExecutionFilter& filter);
		void reqAccountUpdates( bool subscribe, const std::string &acctCode );
		void reqIds( int numIds );
		void reqContractDetails( int reqId, const IB::Contract &contract );
		void setServerLogLevel( int logLevel );
		void reqHistoricalData ( int tickerId, const IB::Contract &contract,
			const std::string &endDateTime, const std::string &durationStr,
			const std::string &barSizeSetting, const std::string &whatToShow,
			int useRTH, int formatDate );
		void reqCurrentTime();
		
	private:
		IB::EWrapper* myEWrapper;
		IB::EPosixClientSocket* ePosixClient;
};


#endif
