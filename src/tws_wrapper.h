/*** twsWrapper.h -- simple "IB/API wrapper" just for debugging
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

#ifndef TWSWrapper_H
#define TWSWrapper_H

// from global installed ibapi
#include "twsapi/EWrapper.h"




class DebugTwsWrapper : public IB::EWrapper
{
	public:
		virtual ~DebugTwsWrapper();
		
	public:
		
		void tickPrice( IB::TickerId tickerId, IB::TickType field, double price,
			int canAutoExecute );
		void tickSize( IB::TickerId tickerId, IB::TickType field, int size );
		void tickOptionComputation( IB::TickerId tickerId,
			IB::TickType tickType, double impliedVol, double delta,
			double optPrice, double pvDividend, double gamma, double vega,
			double theta, double undPrice );
		void tickGeneric( IB::TickerId tickerId, IB::TickType tickType,
			double value );
		void tickString(IB::TickerId tickerId, IB::TickType tickType,
			const IB::IBString& value );
		void tickEFP( IB::TickerId tickerId, IB::TickType tickType,
			double basisPoints, const IB::IBString& formattedBasisPoints,
			double totalDividends, int holdDays,
			const IB::IBString& futureExpiry, double dividendImpact,
			double dividendsToExpiry );
		void orderStatus( IB::OrderId orderId, const IB::IBString &status,
			int filled, int remaining, double avgFillPrice, int permId,
			int parentId, double lastFillPrice, int clientId,
			const IB::IBString& whyHeld );
		void openOrder( IB::OrderId orderId, const IB::Contract&,
			const IB::Order&, const IB::OrderState& );
		void openOrderEnd();
		void winError( const IB::IBString &str, int lastError );
		void connectionClosed();
		void updateAccountValue( const IB::IBString& key,
			const IB::IBString& val, const IB::IBString& currency,
			const IB::IBString& accountName );
		void updatePortfolio( const IB::Contract& contract, int position,
			double marketPrice, double marketValue, double averageCost,
			double unrealizedPNL, double realizedPNL,
			const IB::IBString& accountName );
		void updateAccountTime( const IB::IBString& timeStamp );
		void accountDownloadEnd( const IB::IBString& accountName );
		void nextValidId( IB::OrderId orderId );
		void contractDetails( int reqId,
			const IB::ContractDetails& contractDetails );
		void bondContractDetails( int reqId,
			const IB::ContractDetails& contractDetails );
		void contractDetailsEnd( int reqId );
		void execDetails( int reqId, const IB::Contract& contract,
			const IB::Execution& execution );
		void execDetailsEnd( int reqId );
		void error( const int id, const int errorCode,
			const IB::IBString errorString );
		void updateMktDepth( IB::TickerId id, int position, int operation,
			int side, double price, int size );
		void updateMktDepthL2( IB::TickerId id, int position,
			IB::IBString marketMaker, int operation, int side, double price,
			int size );
		void updateNewsBulletin( int msgId, int msgType,
			const IB::IBString& newsMessage, const IB::IBString& originExch );
		void managedAccounts( const IB::IBString& accountsList );
		void receiveFA( IB::faDataType pFaDataType, const IB::IBString& cxml );
		void historicalData( IB::TickerId reqId, const IB::IBString& date,
			double open, double high, double low, double close, int volume,
			int barCount, double WAP, int hasGaps );
		void scannerParameters( const IB::IBString &xml );
		void scannerData( int reqId, int rank,
			const IB::ContractDetails &contractDetails,
			const IB::IBString &distance, const IB::IBString &benchmark,
			const IB::IBString &projection, const IB::IBString &legsStr );
		void scannerDataEnd(int reqId);
		void realtimeBar( IB::TickerId reqId, long time, double open,
			double high, double low, double close, long volume, double wap,
			int count );
		void currentTime( long time );
		void fundamentalData( IB::TickerId reqId, const IB::IBString& data );
		void deltaNeutralValidation( int reqId,
			const IB::UnderComp& underComp );
		void tickSnapshotEnd( int reqId );
};

#endif
