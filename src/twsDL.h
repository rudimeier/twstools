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






class ContractDetailsRequest;
class HistRequest;
class GenericRequest;
class WorkTodo;
class ContractDetailsTodo;
class HistTodo;
class Packet;
class RowAccVal;
class RowPrtfl;
class RowOrderStatus;
class RowOpenOrder;
class PacingGod;
class DataFarmStates;

class TwsDlWrapper;




class TwsDL
{
	public:
		enum State {
			CONNECT,
			WAIT_TWS_CON,
			IDLE,
			WAIT_DATA,
			QUIT_READY,
			QUIT_ERROR
		};
		
		TwsDL( const std::string& workFile );
		~TwsDL();
		
		void start();
		
		State currentState() const;
		
	private:
		void initTwsClient();
		void eventLoop();
		
		void dumpWorkTodo() const;
		
		void connectTws();
		void waitTwsCon();
		void idle();
		void getContracts();
		bool finContracts();
		void getHist();
		bool finHist();
		void waitData();
		void onQuit( int ret );
		
		void changeState( State );
		
		void initWork();
		
		void reqContractDetails( const ContractDetailsRequest& );
		void reqHistoricalData( const HistRequest& );
		void reqAccStatus();
		void reqExecutions();
		void reqOrders();
		
		void errorContracts(int, int, const std::string &);
		void errorHistData(int, int, const std::string &);
		
		// callbacks from our twsWrapper
		void twsError(int, int, const std::string &);
		
		void twsConnected( bool connected );
		void twsContractDetails( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void twsBondContractDetails( int reqId,
			const IB::ContractDetails &ibContractDetails );
		void twsContractDetailsEnd( int reqId );
		void twsHistoricalData( int reqId, const std::string &date, double open,
			double high, double low, double close, int volume, int count,
			double WAP, bool hasGaps );
		void twsUpdateAccountValue( const RowAccVal& );
		void twsUpdatePortfolio( const RowPrtfl& );
		void twsUpdateAccountTime( const std::string& timeStamp );
		void twsAccountDownloadEnd( const std::string& accountName );
		void twsExecDetails( int reqId, const IB::Contract&,
			const IB::Execution& );
		void twsExecDetailsEnd( int reqId );
		void twsOrderStatus( const RowOrderStatus& );
		void twsOpenOrder( const RowOpenOrder& );
		void twsOpenOrderEnd();
		
		
		State state;
		int64_t lastConnectionTime;
		bool connection_failed;
		int curIdleTime;
		
		std::string workFile;
		
		TwsDlWrapper *twsWrapper;
		TWSClient  *twsClient;
		
		int msgCounter;
		GenericRequest &currentRequest;
		
		int curIndexTodoContractDetails;
		
		WorkTodo *workTodo;
		
		Packet *packet;
		
		DataFarmStates &dataFarms;
		PacingGod &pacingControl;
		
	friend class TwsDlWrapper;
};




#endif
