/*** tws_meta.h -- helper structs for IB/API messages
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

#ifndef TWS_META_H
#define TWS_META_H

#include "twsapi/Contract.h"
#include "twsapi/Execution.h"
#include "twsapi/Order.h"
#include "twsapi/OrderState.h"
#include "twsapi/CommonDefs.h"

#include <stdint.h>
#include <list>
#include <map>


typedef struct _xmlNode * xmlNodePtr;
typedef struct _xmlDoc * xmlDocPtr;



enum req_err
{
	REQ_ERR_NONE,
	REQ_ERR_NODATA,
	REQ_ERR_NAV,
	REQ_ERR_TWSCON,
	REQ_ERR_TIMEOUT,
	REQ_ERR_REQUEST
};








class GenericRequest
{
	public:
		enum ReqType {
			NONE,
			ACC_STATUS_REQUEST,
			EXECUTIONS_REQUEST,
			ORDERS_REQUEST,
			CONTRACT_DETAILS_REQUEST,
			HIST_REQUEST,
			MKT_DATA_REQUEST
		};
		
		GenericRequest();
		
		ReqType reqType() const;
		int reqId() const;
		int age() const;
		void nextRequest( ReqType );
		void close();
		
	private:
		ReqType _reqType;
		int _reqId;
		
		int64_t _ctime;
};








class HistRequest;
class PacingGod;
class DataFarmStates;
class WorkTodo;

class HistTodo
{
	public:
		HistTodo();
		~HistTodo();
		
		void dumpLeft() const;
		
		int countDone() const;
		int countLeft() const;
		void checkout();
		int checkoutOpt( PacingGod *pG, const DataFarmStates *dfs );
		const HistRequest& current() const;
		void tellDone();
		void cancelForRepeat( int priority );
		void add( const HistRequest& );
		
	private:
		std::list<HistRequest*> &doneRequests;
		std::list<HistRequest*> &leftRequests;
		std::list<HistRequest*> &errorRequests;
		HistRequest *checkedOutRequest;
};







class ContractDetailsRequest;

class ContractDetailsTodo
{
	public:
		ContractDetailsTodo();
		virtual ~ContractDetailsTodo();
		
		int countLeft() const;
		void checkout();
		const ContractDetailsRequest& current() const;
		void add( const ContractDetailsRequest& );
		
	private:
		int curIndex;
		std::vector<ContractDetailsRequest> &contractDetailsRequests;
};




class PlaceOrder;

class PlaceOrderTodo
{
	public:
		PlaceOrderTodo();
		virtual ~PlaceOrderTodo();

		int countLeft() const;
		void checkout();
		const PlaceOrder& current() const;
		void add( const PlaceOrder& );

	private:
		int curIndex;
		std::vector<PlaceOrder> &placeOrders;
};




class MktDataRequest;

class MktDataTodo
{
	public:
		MktDataTodo();
		virtual ~MktDataTodo();

		void add( const MktDataRequest& );

		std::vector<MktDataRequest> &mktDataRequests;
};



class WorkTodo
{
	public:
		WorkTodo();
		virtual ~WorkTodo();
		
		GenericRequest::ReqType nextReqType() const;
		ContractDetailsTodo* contractDetailsTodo() const;
		const ContractDetailsTodo& getContractDetailsTodo() const;
		HistTodo* histTodo() const;
		const HistTodo& getHistTodo() const;
		PlaceOrderTodo* placeOrderTodo() const;
		const PlaceOrderTodo& getPlaceOrderTodo() const;
		MktDataTodo *mktDataTodo() const;
		const MktDataTodo& getMktDataTodo() const;
		void addSimpleRequest( GenericRequest::ReqType reqType );
		int read_file( const char *fileName);
		
	private:
		int read_req( const xmlNodePtr xn );
		
		mutable bool acc_status_todo;
		mutable bool executions_todo;
		mutable bool orders_todo;
		ContractDetailsTodo *_contractDetailsTodo;
		HistTodo *_histTodo;
		PlaceOrderTodo *_place_order_todo;
		MktDataTodo *_market_data_todo;
};




enum tws_row_type {
	t_error,
	t_orderStatus,
	t_openOrder
};

struct TwsRow
{
	tws_row_type type;
	void *data;
};

struct RowError
{
	int id;
	int code;
	IB::IBString msg;
};
struct RowOrderStatus;
struct RowOpenOrder;

class Packet
{
	public:
		enum Mode { CLEAN, RECORD, CLOSED };
		
		Packet();
		virtual ~Packet();
		
		bool empty() const;
		bool finished() const;
		req_err getError() const;
		void closeError( req_err );
		
		virtual void clear() = 0;
		virtual void dumpXml() = 0;
		
	protected:
		Mode mode;
		req_err error;
};








class PacketContractDetails
	: public Packet
{
	public:
		PacketContractDetails();
		virtual ~PacketContractDetails();
		
		static PacketContractDetails * fromXml( xmlNodePtr );
		
		const ContractDetailsRequest& getRequest() const;
		const std::vector<IB::ContractDetails>& constList() const;
		void record( int reqId, const ContractDetailsRequest& );
		void setFinished();
		void clear();
		void append( int reqId, const IB::ContractDetails& );
		
		void dumpXml();
		
	private:
		int reqId;
		ContractDetailsRequest *request;
		std::vector<IB::ContractDetails> * const cdList;
};


inline const std::vector<IB::ContractDetails>&
PacketContractDetails::constList() const
{
	return *cdList;
}






struct RowHist
{
	std::string date;
	double open;
	double high;
	double low;
	double close;
	int volume;
	int count;
	double WAP;
	bool hasGaps;
};

/* we need a default object but want to avoid a slow default constructor */
static const RowHist dflt_RowHist
 	= {"", -1.0, -1.0, -1.0, -1.0, -1, -1, -1.0, false };

class PacketHistData
	: public  Packet
{
	public:
		PacketHistData();
		virtual ~PacketHistData();
		
		static PacketHistData * fromXml( xmlNodePtr );
		
		const HistRequest& getRequest() const;
		void clear();
		void record( int reqId, const HistRequest& );
		void append( int reqId, const RowHist& );
		void dump( bool printFormatDates );
		
		void dumpXml();
		
	private:
		int reqId;
		HistRequest *request;
		std::vector<RowHist> &rows;
		RowHist finishRow;
};




class PlaceOrder;

class PacketPlaceOrder
	: public  Packet
{
	public:
		PacketPlaceOrder();
		virtual ~PacketPlaceOrder();

		static PacketPlaceOrder * fromXml( xmlNodePtr );

		const PlaceOrder& getRequest() const;
		virtual void clear();
		void record( long orderId, const PlaceOrder& );
		void modify( const PlaceOrder& );
		void append( const RowError& );
		void append( const RowOrderStatus& );
		void append( const RowOpenOrder& );

		virtual void dumpXml();

	private:
		PlaceOrder *request;
		std::vector<TwsRow> * const list;
};




class AccStatusRequest;

struct RowAcc
{
	enum row_acc_type { t_AccVal, t_Prtfl, t_stamp, t_end };
	row_acc_type type;
	void *data;
};

struct RowAccVal
{
	std::string key;
	std::string val;
	std::string currency;
	std::string accountName;
};

struct RowPrtfl
{
	IB::Contract contract;
	int position;
	double marketPrice;
	double marketValue;
	double averageCost;
	double unrealizedPNL;
	double realizedPNL;
	std::string accountName;
};

class PacketAccStatus
	: public  Packet
{
	public:
		PacketAccStatus();
		virtual ~PacketAccStatus();
		
		void clear();
		void record( const AccStatusRequest& );
		void append( const RowAccVal& );
		void append( const RowPrtfl& );
		void appendUpdateAccountTime( const std::string& timeStamp );
		void appendAccountDownloadEnd( const std::string& accountName );
		
		void dumpXml();
		
	private:
		void del_list_elements();
		
		AccStatusRequest *request;
		
		std::vector<RowAcc*> * const list;
};







class ExecutionsRequest;

struct RowExecution
{
	IB::Contract contract;
	IB::Execution execution;
};

class PacketExecutions
	: public  Packet
{
	public:
		PacketExecutions();
		virtual ~PacketExecutions();
		
		void clear();
		void record( const int reqId, const ExecutionsRequest& );
		void append( int reqId, const RowExecution& );
		void appendExecutionsEnd( int reqId );
		
		void dumpXml();
		
	private:
		void del_list_elements();
		
		int reqId;
		ExecutionsRequest *request;
		
		std::vector<RowExecution*> * const list;
};







class OrdersRequest;

struct RowOrderStatus
{
	IB::OrderId id;
	std::string status;
	int filled;
	int remaining;
	double avgFillPrice;
	int permId;
	int parentId;
	double lastFillPrice;
	int clientId;
	std::string whyHeld;
};

struct RowOpenOrder
{
	IB::OrderId orderId;
	IB::Contract contract;
	IB::Order order;
	IB::OrderState orderState;
};

class PacketOrders
	: public  Packet
{
	public:
		PacketOrders();
		virtual ~PacketOrders();
		
		void clear();
		void record( const OrdersRequest& );
		void append( const RowOrderStatus& );
		void append( const RowOpenOrder& );
		void appendOpenOrderEnd();
		
		void dumpXml();
		
	private:
		OrdersRequest *request;
		std::vector<TwsRow> * const list;
};




class PacketMktData
	: public  Packet
{
	public:
		PacketMktData();
		virtual ~PacketMktData();

		static PacketMktData * fromXml( xmlNodePtr );

		const MktDataRequest& getRequest() const;
		void clear();
		void record( int reqId, const MktDataRequest& );
// 		void append( int reqId, const RowHist& );

		void dumpXml();

	private:
		int reqId;
		MktDataRequest *request;
// 		std::vector<RowHist> &rows;
};




class PacingControl
{
	public:
		PacingControl( int packets, int interval, int min, int vPause );
		virtual ~PacingControl();
		
		void setPacingTime( int packets, int interval, int min );
		void setViolationPause( int violationPause );
		
		bool isEmpty() const;
		void clear();
		void addRequest();
		void notifyViolation();
		int goodTime( const char** dbg ) const;
		int countLeft() const;
		
		void merge( const PacingControl& );
		
	private:
		std::vector<int64_t> &dateTimes;
		std::vector<bool> &violations;
		
		int maxRequests;
		int checkInterval;
		int minPacingTime;
		int violationPause;
};




class PacingGod
{
	public:
		PacingGod( const DataFarmStates& );
		~PacingGod();
		
		void setPacingTime( int packets, int interval, int min );
		void setViolationPause( int pause );
		
		void clear();
		void addRequest( const IB::Contract& );
		void notifyViolation( const IB::Contract& );
		int goodTime( const IB::Contract& );
		int countLeft( const IB::Contract& c );
	
	private:
		void checkAdd( const IB::Contract&,
			std::string *lazyContract, std::string *farm );
		bool laziesAreCleared() const;
		
		const DataFarmStates& dataFarms;
		
		int maxRequests;
		int checkInterval;
		int minPacingTime;
		int violationPause;
		
		PacingControl &controlGlobal;
		std::map<const std::string, PacingControl*> &controlHmds;
		std::map<const std::string, PacingControl*> &controlLazy;
};







class DataFarmStates
{
	public:
		enum State { BROKEN, INACTIVE, OK };
		
		DataFarmStates();
		virtual ~DataFarmStates();
		
		std::vector<std::string> getInactives() const;
		std::vector<std::string>  getActives() const;
		std::string getMarketFarm( const IB::Contract& ) const;
		std::string getHmdsFarm( const std::string& lazyC ) const;
		std::string getHmdsFarm( const IB::Contract& ) const;
		
		void initHardCodedFarms();
		void setAllBroken();
		void notify(int msgNumber, int errorCode, const std::string &msg);
		void learnMarket( const IB::Contract& );
		void learnHmds( const IB::Contract& );
		void learnHmdsLastOk(int msgNumber, const IB::Contract& );
		
	private:
		static std::string getFarm( const std::string &prefix,
			const std::string &msg );
		
		void check_edemo_hack( const std::string &farm );
		
		std::map<const std::string, State> &mStates;
		std::map<const std::string, State> &hStates;
		
		std::map<const std::string, std::string> &mLearn;
		std::map<const std::string, std::string> &hLearn;
		
		int lastMsgNumber;
		std::string lastChanged;
		
		/* Remember last (lazy) contract we've tried to learn as long as farm
		   states don't change. This is for optimizing repeatedly calls of
		   learnHmds(). */
		std::string last_learned_lazy_contract;
		bool edemo_checked;
};




#endif
