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

#if TWSAPI_IB_VERSION_NUMBER >= 97200
# define POS_TYPE double
#else
# define POS_TYPE int
#endif

class DebugTwsWrapper : public EWrapper
{
	public:
		virtual ~DebugTwsWrapper();

	public:
#if TWSAPI_VERSION_NUMBER >= 17300
		#include <twsapi/EWrapper_prototypes.h>
#else
		void tickPrice( TickerId tickerId, TickType field, double price,
#if TWSAPI_IB_VERSION_NUMBER >= 97300
			const TickAttrib& );
#else
			int canAutoExecute);
#endif
		void tickSize( TickerId tickerId, TickType field, int size );
		void tickOptionComputation( TickerId tickerId,
			TickType tickType, double impliedVol, double delta,
			double optPrice, double pvDividend, double gamma, double vega,
			double theta, double undPrice );
		void tickGeneric( TickerId tickerId, TickType tickType,
			double value );
		void tickString(TickerId tickerId, TickType tickType,
			const IBString& value );
		void tickEFP( TickerId tickerId, TickType tickType,
			double basisPoints, const IBString& formattedBasisPoints,
			double totalDividends, int holdDays,
			const IBString& futureExpiry, double dividendImpact,
			double dividendsToExpiry );
		void orderStatus( OrderId orderId, const IBString &status,
			POS_TYPE filled, POS_TYPE remaining, double avgFillPrice, int permId,
			int parentId, double lastFillPrice, int clientId,
			const IBString& whyHeld
#if TWSAPI_IB_VERSION_NUMBER >= 97300
			, double mktCapPrice
#endif
			);
		void openOrder( OrderId orderId, const Contract&,
			const Order&, const OrderState& );
		void openOrderEnd();
		void winError( const IBString &str, int lastError );
		void connectionClosed();
		void updateAccountValue( const IBString& key,
			const IBString& val, const IBString& currency,
			const IBString& accountName );
		void updatePortfolio( const Contract& contract, POS_TYPE position,
			double marketPrice, double marketValue, double averageCost,
			double unrealizedPNL, double realizedPNL,
			const IBString& accountName );
		void updateAccountTime( const IBString& timeStamp );
		void accountDownloadEnd( const IBString& accountName );
		void nextValidId( OrderId orderId );
		void contractDetails( int reqId,
			const ContractDetails& contractDetails );
		void bondContractDetails( int reqId,
			const ContractDetails& contractDetails );
		void contractDetailsEnd( int reqId );
		void execDetails( int reqId, const Contract& contract,
			const Execution& execution );
		void execDetailsEnd( int reqId );
		void error( const int id, const int errorCode,
			const IBString errorString );
		void updateMktDepth( TickerId id, int position, int operation,
			int side, double price, int size );
		void updateMktDepthL2( TickerId id, int position,
			IBString marketMaker, int operation, int side, double price,
			int size );
		void updateNewsBulletin( int msgId, int msgType,
			const IBString& newsMessage, const IBString& originExch );
		void managedAccounts( const IBString& accountsList );
		void receiveFA( faDataType pFaDataType, const IBString& cxml );
#if TWSAPI_IB_VERSION_NUMBER >= 97300
		void historicalData(TickerId reqId, Bar bar);
		void historicalDataEnd(int reqId, IBString startDateStr,
			IBString endDateStr);
#else
		void historicalData( TickerId reqId, const IBString& date,
			double open, double high, double low, double close, int volume,
			int barCount, double WAP, int hasGaps );
#endif
		void scannerParameters( const IBString &xml );
		void scannerData( int reqId, int rank,
			const ContractDetails &contractDetails,
			const IBString &distance, const IBString &benchmark,
			const IBString &projection, const IBString &legsStr );
		void scannerDataEnd(int reqId);
		void realtimeBar( TickerId reqId, long time, double open,
			double high, double low, double close, long volume, double wap,
			int count );
		void currentTime( long time );
		void fundamentalData( TickerId reqId, const IBString& data );
		void deltaNeutralValidation( int reqId,
			const UnderComp& underComp );
		void tickSnapshotEnd( int reqId );
		void marketDataType( TickerId reqId, int marketDataType );
		void commissionReport( const CommissionReport &commissionReport );
		void position( const IBString& account,
			const Contract& contract, POS_TYPE position, double avgCost );
		void positionEnd();
		void accountSummary( int reqId, const IBString& account,
			const IBString& tag, const IBString& value,
			const IBString& currency );
		void accountSummaryEnd( int reqId );
#if TWSAPI_IB_VERSION_NUMBER >= 971
		void verifyMessageAPI( const IBString& apiData);
		void verifyCompleted( bool isSuccessful, const IBString& errorText);
		void displayGroupList( int reqId, const IBString& groups);
		void displayGroupUpdated( int reqId, const IBString& contractInfo);
#endif
#if TWSAPI_IB_VERSION_NUMBER >= 97200
		void verifyAndAuthMessageAPI( const IBString& apiData,
			const IBString& xyzChallange);
		void verifyAndAuthCompleted( bool isSuccessful,
			const IBString& errorText);
		void connectAck();
		void positionMulti( int reqId, const std::string& account,
			const std::string& modelCode, const Contract& contract,
			double pos, double avgCost);
		void positionMultiEnd( int reqId);
		void accountUpdateMulti( int reqId, const std::string& account,
			const std::string& modelCode, const std::string& key,
			const std::string& value, const std::string& currency);
		void accountUpdateMultiEnd( int reqId);
		void securityDefinitionOptionalParameter(int reqId,
			const std::string& exchange, int underlyingConId,
			const std::string& tradingClass, const std::string& multiplier,
			std::set<std::string> expirations, std::set<double> strikes);
		void securityDefinitionOptionalParameterEnd(int reqId);
		void softDollarTiers(int reqId,
			const std::vector<SoftDollarTier> &tiers);
#endif
#if TWSAPI_IB_VERSION_NUMBER >= 97300
		void familyCodes(const std::vector<FamilyCode> &familyCodes);
		void symbolSamples(int reqId,
			const std::vector<ContractDescription> &contractDescriptions);
		void mktDepthExchanges(
			const std::vector<DepthMktDataDescription> &depthMktDataDescriptions);
		void tickNews(int tickerId, time_t timeStamp,
			const std::string& providerCode, const std::string& articleId,
			const std::string& headline, const std::string& extraData);
		void smartComponents(int reqId, SmartComponentsMap theMap);
		void tickReqParams(int tickerId,
			double minTick, std::string bboExchange, int snapshotPermissions);
		void newsProviders(const std::vector<NewsProvider> &newsProviders);
		void newsArticle(int requestId, int articleType,
			const std::string& articleText);
		void historicalNews(int requestId, const std::string& time,
			const std::string& providerCode, const std::string& articleId,
			const std::string& headline);
		void historicalNewsEnd(int requestId, bool hasMore);
		void headTimestamp(int reqId, const std::string& headTimestamp);
		void histogramData(int reqId, HistogramDataVector data);
		void historicalDataUpdate(TickerId reqId, Bar bar);
		void rerouteMktDataReq(int reqId, int conid,
			const std::string& exchange);
		void rerouteMktDepthReq(int reqId, int conid,
			const std::string& exchange);
		void marketRule(int marketRuleId,
			const std::vector<PriceIncrement> &priceIncrements);
		void pnl(int reqId, double dailyPnL, double unrealizedPnL,
			double realizedPnL);
		void pnlSingle(int reqId, int pos, double dailyPnL,
			double unrealizedPnL, double realizedPnL, double value);
		void historicalTicks(int reqId,
			const std::vector<HistoricalTick> &ticks, bool done);
		void historicalTicksBidAsk(int reqId,
			const std::vector<HistoricalTickBidAsk> &ticks, bool done);
		void historicalTicksLast(int reqId,
			const std::vector<HistoricalTickLast> &ticks, bool done);
		void tickByTickAllLast(int reqId, int tickType, time_t time,
			double price, int size, const TickAttrib& attribs,
			const std::string& exchange, const std::string& specialConditions);
		void tickByTickBidAsk(int reqId, time_t time, double bidPrice,
			double askPrice, int bidSize, int askSize,
			const TickAttrib& attribs);
		void tickByTickMidPoint(int reqId, time_t time, double midPoint);
#endif
#endif /* TWSAPI_VERSION_NUMBER >= 17300 */
};

#endif
