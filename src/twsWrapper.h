#ifndef TWSWrapper_H
#define TWSWrapper_H

// from global installed ibapi
#include "ibtws/EWrapper.h"




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
			double optPrice, double pvDividend/*, double gamma, double vega,
			double theta, double undPrice*/ );
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
