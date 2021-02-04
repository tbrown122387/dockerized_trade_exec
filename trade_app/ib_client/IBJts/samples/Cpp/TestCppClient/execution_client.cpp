/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"

#include "execution_client.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "ContractSamples.h"
#include "OrderSamples.h"
#include "ScannerSubscription.h"
#include "ScannerSubscriptionSamples.h"
#include "executioncondition.h"
#include "PriceCondition.h"
#include "MarginCondition.h"
#include "PercentChangeCondition.h"
#include "TimeCondition.h"
#include "VolumeCondition.h"
#include "AvailableAlgoParams.h"
#include "FAMethodSamples.h"
#include "CommonDefs.h"
#include "AccountSummaryTags.h"
#include "Utils.h"

#include <stdio.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <ctime>
#include <fstream>
#include <cstdint>
#include <cstdlib> //getenv



ExecClient::ExecClient() :
      m_osSignal(2000)//2-seconds timeout
    , m_pClient(new EClientSocket(this, &m_osSignal))
	, m_state(ST_CONNECT)
    , m_orderId(0)
    , m_pReader(0)
    , m_extraAuth(false)
    , m_printing(true)
    , m_maxLoss(atof(std::getenv("IB_MAX_LOSS")))
    , m_ticker_config("/usr/src/app/IBJts/samples/Cpp/TestCppClient/tickers.txt")
    , m_positions(m_ticker_config)
    , m_high_water_profit(0.0)
{

    std::cout 
    << "-------------------------------------\n"
    << "now starting execution client... \n";
    for(unsigned int i = 0; i < m_ticker_config.size(); ++i){
        std::string sym = m_ticker_config.loc_syms(i);
        std::cout << "added " << sym << " to list of tracked symbols\n";
    }

    std::cout << "max loss set to " << m_maxLoss << "\n";
    std::cout 
    << "-------------------------------------\n";

}


ExecClient::~ExecClient()
{
    if (m_pReader)
        delete m_pReader;
    delete m_pClient;
}


bool ExecClient::connect(const char *host, int port, int clientId)
{
	printf( "Connecting to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);
	bool bRes = m_pClient->eConnect( host, port, clientId, m_extraAuth);
	
	if (bRes) {
		printf( "Connected to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);
        	m_pReader = new EReader(m_pClient, &m_osSignal);
		m_pReader->start();
	}
	else
		printf( "Cannot connect to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

    return bRes;
}

void ExecClient::disconnect() const
{
	m_pClient->eDisconnect();

	printf ( "Disconnected\n");
}

bool ExecClient::isConnected() const
{
	return m_pClient->isConnected();
}

void ExecClient::setConnectOptions(const std::string& connectOptions)
{
	m_pClient->setConnectOptions(connectOptions);
}


void ExecClient::processMessages()
{
	// connection sequence, then oscillate back and forth
	// between checking actual/desired position and checking pnl
    // here's a chart:
	
    // ST_REQTRADEDATA -> ST_REQORDERDATA -> ST_REQPNL -> ST_REQPOSITIONS 
	// -> (ST_CHECK_POSITIONS <-> ST_CHECK_PNL)
    //                                  |
    //                                  V
    //                            ST_CLOSEOUT
    //                                  |
    //                                  V
    //                             ST_UNSUBSCRIBE

	switch (m_state) {

		case ST_REQTRADEDATA:
			reqAllTradeData(); // changes m_state to ST_REQORDERDATA
			break;
	        case ST_REQORDERDATA:
        	    	// need to wait more than 15 seconds to request two things
           		 // from the same symbol
			std::this_thread::sleep_for(std::chrono::seconds(16));
            		reqAllOrderData(); // changes m_state to ST_REQPNL
			break;
	    	case ST_REQPNL:
			reqPNL(); // changes m_state to ST_REQPOSITIONS
			break;
		case ST_REQPOSITIONS:
			reqPositions(); // positionEnd() changes m_state to ST_CHECK_POSITIONS
			break;
		case ST_CHECK_POSITIONS:
			orderOperations(); // changes m_state to ST_CHECK_PNL 
			break;
		case ST_CHECK_PNL:
			pnlOperation(); // changes m_state to either ST_CHECK_POSITIONS or ST_CLOSEOUT
			break;
        	case ST_CLOSEOUT:
            		closeoutEverything(); // changes m_state to ST_UNSUBSCRIBE
            		break;
        	case ST_UNSUBSCRIBE:
            		unsubscribeAll();   // does not change m_state
            		break; // 
	}

	m_osSignal.waitForSignal();
	errno = 0;
	m_pReader->processMsgs();
}


void ExecClient::connectAck() {
	if (!m_extraAuth && m_pClient->asyncEConnect())
        m_pClient->startApi();
}


void ExecClient::pnlOperation()
{
    if(m_printing) std::cout << "now checking your pnl info inside pnlOperation() \n";

    // set to "close-only" if you're losing money
    if(m_high_water_profit - m_current_profits > m_maxLoss) { 
        if( m_printing) std::cout << "max loss exceeded...entering clsoeout mode...\n";
        m_state = ST_CLOSEOUT;
    }else{
        // tell API to keep checking positions (desired and actual)	
        m_state = ST_CHECK_POSITIONS;
    }
}


void ExecClient::orderOperations()
{
    
    if(m_printing) { std::cout << "inside orderOperations(), potentially changing positions \n";  }
    
    // iterate over all symbols, and send orders for the shares you want 
    std::string loc_sym;
    for( unsigned int i = 0; i < m_ticker_config.size(); ++i){
            
        loc_sym = m_ticker_config.loc_syms(i);
        m_positions.getDesiredPosition(loc_sym);
    
        // if you need to get long or short, get the number of shares and do that 
        if (m_positions.getDesiredPosition(loc_sym) < m_positions.getActualPosition(loc_sym) ){

		    unsigned numShares = 
                    m_positions.getActualPosition(loc_sym)-
                    m_positions.getDesiredPosition(loc_sym); 

            market_sell(loc_sym, numShares); 

            if( m_printing) std::cout << "now selling " << numShares << " shares\n";

        }else if( m_positions.getDesiredPosition(loc_sym) > m_positions.getActualPosition(loc_sym)){  

            unsigned numShares = 
                        m_positions.getDesiredPosition(loc_sym) -
                        m_positions.getActualPosition(loc_sym);

            market_buy(loc_sym, numShares); 

            if( m_printing) std::cout << "now buying " << numShares << " shares\n";
        }            
	}

    // tell API to go check PNL  
    m_state = ST_CHECK_PNL;
}


void ExecClient::closeoutEverything()
{
    // close out all positions just in case you have them
    close_all_positions();

    // set state to unsubscribe
    m_state = ST_UNSUBSCRIBE;

}


void ExecClient::unsubscribeAll(){
    
    m_pClient->cancelPnL(PNL_REGID);
    m_pClient->cancelPositions();
    
    for(unsigned int i = 0; i < m_ticker_config.size(); ++i){
        std::string loc_sym = m_ticker_config.loc_syms(i);
        m_pClient->cancelMktData(m_positions.getTradeID(loc_sym));
        m_pClient->cancelMktData(m_positions.getOrderID(loc_sym));
    }
}


void ExecClient::reqAllTradeData()
{
    for(unsigned int i = 0; i < m_ticker_config.size(); ++i){
        
        std::string loc_sym = m_ticker_config.loc_syms(i);
        Contract contract;
        contract.localSymbol = loc_sym;
        contract.symbol = m_positions.getNonLocalSymbol(loc_sym);
        contract.secType = m_positions.getSecType(loc_sym);
        contract.currency = m_positions.getCurrency(loc_sym);
        contract.exchange = m_positions.getExchange(loc_sym);
        
        if(m_printing) std::cout << "requesting trade data for " << contract.symbol << "\n";

        m_pClient->reqTickByTickData(
                m_positions.getTradeID(loc_sym),
                contract,
                "Last",
                0, 
                true); // last argument is ignored me thinks
    }

    m_state = ST_REQORDERDATA;
}


void ExecClient::reqAllOrderData()
{
    // request two types of data for all symbols 
    for(unsigned int i = 0; i < m_ticker_config.size(); ++i){
        
        std::string loc_sym = m_ticker_config.loc_syms(i);
        Contract contract;
        contract.localSymbol = loc_sym;
        contract.symbol = m_positions.getNonLocalSymbol(loc_sym);
        contract.secType = m_positions.getSecType(loc_sym);
        contract.currency = m_positions.getCurrency(loc_sym);
        contract.exchange = m_positions.getExchange(loc_sym);
        
        if(m_printing) std::cout << "requesting bid/ask data for " << contract.symbol << "\n";
        
        m_pClient->reqTickByTickData(
                m_positions.getOrderID(loc_sym),
                contract,
                "BidAsk",
                0, // nonzero means historical data, too
                true); // ignore size only changes?
    }

    m_state = ST_REQPNL;
}


void ExecClient::reqPNL()
{
    // request pnl and account updates
    std::string account_str = std::getenv("IB_ACCOUNT_STR");
    if( account_str.empty())
        throw std::invalid_argument("must specify a TWS_ACCOUNT_STR");
    m_pClient->reqPnL(PNL_REGID, account_str, "");

    // tell API to go start requesting postion info now 
    m_state = ST_REQPOSITIONS;
}


void ExecClient::reqPositions()
{
    m_pClient->reqPositions();
    //positionEnd() will change m_state to ST_CHECK_POSITIONS
}


void ExecClient::nextValidId( OrderId orderId)
{
	if(m_printing)
        printf("Next Valid Id: %ld\n", orderId);
	m_orderId = orderId;

    // the starting state after connection is achieved
    m_state = ST_REQTRADEDATA; 
}


void ExecClient::error(int id, int errorCode, const std::string& errorString)
{
	printf( "Error. Id: %d, Code: %d, Msg: %s\n", id, errorCode, errorString.c_str());
}


void ExecClient::orderStatus(OrderId orderId, const std::string& status, double filled,
		double remaining, double avgFillPrice, int permId, int parentId,
		double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice){
	
    if(m_printing)
        printf("OrderStatus. Id: %ld, Status: %s, Filled: %g, Remaining: %g, AvgFillPrice: %g, PermId: %d, LastFillPrice: %g, ClientId: %d, WhyHeld: %s, MktCapPrice: %g\n", orderId, status.c_str(), filled, remaining, avgFillPrice, permId, lastFillPrice, clientId, whyHeld.c_str(), mktCapPrice);

}


void ExecClient::openOrder( OrderId orderId, const Contract& contract, const Order& order, const OrderState& orderState) {

    if(m_printing){
        printf( "OpenOrder. PermId: %i, ClientId: %ld, OrderId: %ld, Account: %s, Symbol: %s, SecType: %s, Exchange: %s:, Action: %s, OrderType:%s, TotalQty: %g, CashQty: %g, "
	"LmtPrice: %g, AuxPrice: %g, Status: %s\n", 
		order.permId, order.clientId, orderId, order.account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(), 
		order.action.c_str(), order.orderType.c_str(), order.totalQuantity, order.cashQty == UNSET_DOUBLE ? 0 : order.cashQty, order.lmtPrice, order.auxPrice, orderState.status.c_str());
    }
}


void ExecClient::openOrderEnd() {
	if(m_printing)
        printf( "OpenOrderEnd\n");
}

void ExecClient::connectionClosed() {
	printf( "Connection Closed\n");
}

void ExecClient::updateAccountValue(const std::string& key, const std::string& val,
                                       const std::string& currency, const std::string& accountName) {
	printf("UpdateAccountValue. Key: %s, Value: %s, Currency: %s, Account Name: %s\n", key.c_str(), val.c_str(), currency.c_str(), accountName.c_str());
}


void ExecClient::updatePortfolio(const Contract& contract, double position,
                                    double marketPrice, double marketValue, double averageCost,
                                    double unrealizedPNL, double realizedPNL, const std::string& accountName){
	printf("UpdatePortfolio. %s, %s @ %s: Position: %g, MarketPrice: %g, MarketValue: %g, AverageCost: %g, UnrealizedPNL: %g, RealizedPNL: %g, AccountName: %s\n", (contract.symbol).c_str(), (contract.secType).c_str(), (contract.primaryExchange).c_str(), position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName.c_str());

}


void ExecClient::execDetails( int reqId, const Contract& contract, const Execution& execution) {

	// TODO this requires a lot more work! E.G.
	// 1. take into account corrections aka duplicate events
///////////////////////////////////////////////////////////////////////////////////////////
// Remove this logic. It's slower, but cleaner to get position information from position()
//////////////////////////////////////////////////////////////////////////////////////////
//    // record the execution
//    std::string loc_sym = m_positions.getLocalSymbolFromID(reqId);
//    int signed_shares (0);
//    if(execution.side == "BOT") {
//        signed_shares += static_cast<int>(execution.shares);
//    } else {
//        signed_shares -= static_cast<int>(execution.shares);
//    }
//    m_positions.incrementPosition(loc_sym, signed_shares); 
//
//    if(m_printing) { 
//        printf( "ExecDetails. ReqId: %d - %s, %s, %s - %s, %ld, %g, %d\n", reqId, contract.symbol.c_str(), 
//                contract.secType.c_str(), contract.currency.c_str(), execution.execId.c_str(), execution.orderId, execution.shares, execution.lastLiquidity);
//        std::cout 
//            << "current position for" 
//            << loc_sym << " is now " 
//            << m_positions.getActualPosition(loc_sym) << "\n";
//    }	

}


void ExecClient::execDetailsEnd( int reqId) {
	// afaik, this is only important when you're *requesting* these details, instead of passively processing them
	if(m_printing)
        printf( "ExecDetailsEnd. %d\n", reqId);
}


void ExecClient::commissionReport( const CommissionReport& commissionReport) {
	if(m_printing)
        printf( "CommissionReport. %s - %g %s RPNL %g\n", commissionReport.execId.c_str(), commissionReport.commission, commissionReport.currency.c_str(), commissionReport.realizedPNL);
}


void ExecClient::position( const std::string& account, const Contract& contract, double position, double avgCost)
{
    if( m_printing) {
        std::cout << "backup checking that the position information is correct...\n";
        printf( "Position. %s - Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), position, avgCost);
    }

    // just in case execDetails is feeding us trash, 
    // this will reset our position   
    int signedShares = static_cast<int>(position);
    m_positions.setPosition(contract.localSymbol, signedShares);
}


void ExecClient::positionEnd() {
    if( m_printing) std::cout << "positions have been updated for the very first time\n";

    // changing m_state to allow starting up
    // api to start checking positions now
    // otherwise it would've started placing orders without knowing what's up 
    m_state = ST_CHECK_POSITIONS;
}


void ExecClient::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) {
	
    // TODO: run in no trading mode and make sure this is called often
    if(m_printing){
        printf("PnL. ReqId: %d, daily PnL: %g, unrealized PnL: %g, realized PnL: %g\n", reqId, dailyPnL, unrealizedPnL, realizedPnL);
        std::cout << "inside pnl()...checking to see if we should stop actively trading \n";
    }

    // update high water mark and current profits
    m_current_profits = unrealizedPnL + realizedPnL;  
    if( m_current_profits > m_high_water_profit )
        m_high_water_profit = m_current_profits;
}



void ExecClient::tickByTickAllLast(int reqId, int tickType, time_t time, double price, int size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions) {

    std::string loc_sym = m_positions.getLocalSymbolFromID(reqId); 

    if(m_printing){
        printf("Tick-By-Tick. ReqId: %d, TickType: %s, Time: %s, Price: %g, Size: %d, PastLimit: %d, Unreported: %d, Exchange: %s, SpecialConditions:%s\n", 
            reqId, (tickType == 1 ? "Last" : "AllLast"), ctime(&time), price, size, tickAttribLast.pastLimit, tickAttribLast.unreported, exchange.c_str(), specialConditions.c_str());
        std::cout 
            << "trade for ticker: "
            << loc_sym << "\n";
    }

    // TODO: do something with last trade here
    // TODO: decide when and how to send to orderOperations
    // TODO: add information to rolling window
    m_positions.setDesiredPosition(loc_sym, -1);
}


void ExecClient::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, int bidSize, int askSize, const TickAttribBidAsk& tickAttribBidAsk) {

    if(m_printing){
        std::cout << "\n\n";	
        printf("Tick-By-Tick. ReqId: %d, TickType: BidAsk, Time: %s, BidPrice: %g, AskPrice: %g, BidSize: %d, AskSize: %d, BidPastLow: %d, AskPastHigh: %d\n", 
            reqId, ctime(&time), bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk.bidPastLow, tickAttribBidAsk.askPastHigh);
        std::cout 
            << "quote for ticker: " 
            << m_positions.getLocalSymbolFromID(reqId)
            << "\n";
    }

    // TODO: do something with the bid/askdata
}


void ExecClient::completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) {
	printf( "CompletedOrder. PermId: %i, ParentPermId: %lld, Account: %s, Symbol: %s, SecType: %s, Exchange: %s:, Action: %s, OrderType: %s, TotalQty: %g, CashQty: %g, FilledQty: %g, "
		"LmtPrice: %g, AuxPrice: %g, Status: %s, CompletedTime: %s, CompletedStatus: %s\n", 
		order.permId, order.parentPermId == UNSET_LONG ? 0 : order.parentPermId, order.account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(), 
		order.action.c_str(), order.orderType.c_str(), order.totalQuantity, order.cashQty == UNSET_DOUBLE ? 0 : order.cashQty, order.filledQuantity, 
		order.lmtPrice, order.auxPrice, orderState.status.c_str(), orderState.completedTime.c_str(), orderState.completedStatus.c_str());
}




inline void ExecClient::market_sell(const std::string& local_symbol, unsigned qty) {

    Order le_order = OrderSamples::MarketOrder("SELL", qty);
    Contract contract;
    contract.symbol = m_positions.getNonLocalSymbol(local_symbol);
    contract.secType = m_positions.getSecType(local_symbol);
    contract.currency = m_positions.getCurrency(local_symbol);
    contract.exchange = m_positions.getExchange(local_symbol);
    contract.localSymbol = local_symbol; 
    m_pClient->placeOrder(m_orderId++, contract, le_order);

    // change your "actual position" 
    // this only makes sense because we're sending market orders 
    // very quickly, and we can't wait for a position update.
    // If we did, we would spam orders super hard and build up a 
    // gigantic position accidentally
    int signed_shares = -qty;
    m_positions.incrementPosition(local_symbol, signed_shares); 
}


inline void ExecClient::market_buy(const std::string& local_symbol, unsigned qty) {

    Order le_order = OrderSamples::MarketOrder("BUY", qty);
    Contract contract;
    contract.symbol = m_positions.getNonLocalSymbol(local_symbol);
    contract.secType = m_positions.getSecType(local_symbol);
    contract.currency = m_positions.getCurrency(local_symbol);
    contract.exchange = m_positions.getExchange(local_symbol);
    contract.localSymbol = local_symbol; 
    m_pClient->placeOrder(m_orderId++, contract, le_order);

    // change your "actual position" 
    // this only makes sense because we're sending market orders 
    // very quickly, and we can't wait for a position update.
    // If we did, we would spam orders super hard and build up a 
    // gigantic position accidentally 
    int signed_shares = qty;
    m_positions.incrementPosition(local_symbol, signed_shares);
}


inline void ExecClient::close_all_positions() {

    std::cout << "NOW CLOSING ALL POSITIONS\n\n";

    for(unsigned int i = 0; i < m_ticker_config.size(); ++i){
        std::string loc_sym = m_ticker_config.loc_syms(i);
        int signed_pos = m_positions.getActualPosition(loc_sym);
        if( signed_pos > 0 ){
            unsigned unsigned_pos = std::abs(signed_pos); 
            market_sell(loc_sym, unsigned_pos);        
        }else if( signed_pos < 0){
            unsigned unsigned_pos = std::abs(signed_pos); 
            market_buy(loc_sym, unsigned_pos);
        }
    }
}


// virtual functions that need to be defined but aren't used at all
void ExecClient::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) {}
void ExecClient::orderBound(long long orderId, int apiClientId, int apiOrderId) {}
void ExecClient::tickString(TickerId tickerId, TickType tickType, const std::string& value) {}
void ExecClient::currentTime( long time) {}
void ExecClient::familyCodes(const std::vector<FamilyCode> &familyCodes) {}
void ExecClient::newsArticle(int requestId, int articleType, const std::string& articleText) {}
void ExecClient::realtimeBar(TickerId reqId, long time, double open, double high, double low, 
double close, long volume, double wap, int count) {}
void ExecClient::tickGeneric(TickerId tickerId, TickType tickType, double value) {}
void ExecClient::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance, const std::string& benchmark, const std::string& projection, const std::string& legsStr) {}
void ExecClient::scannerDataEnd(int reqId) {}
void ExecClient::histogramData(int reqId, const HistogramDataVector& data) {}
void ExecClient::newsProviders(const std::vector<NewsProvider> &newsProviders) {}
void ExecClient::symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) {}
void ExecClient::tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) {}
void ExecClient::accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) {}
void ExecClient::historicalData(TickerId reqId, const Bar& bar) {}
void ExecClient::historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline) {}
void ExecClient::headTimestamp(int reqId, const std::string& headTimestamp) {}
void ExecClient::marketDataType(TickerId reqId, int marketDataType) {}
void ExecClient::updateMktDepth(TickerId id, int position, int operation, int side, double price, int size) {}
void ExecClient::contractDetails( int reqId, const ContractDetails& contractDetails) {}
void ExecClient::fundamentalData(TickerId reqId, const std::string& data) {}
void ExecClient::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done) {}
void ExecClient::managedAccounts( const std::string& accountsList) {}
void ExecClient::smartComponents(int reqId, const SmartComponentsMap& theMap) {}
void ExecClient::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) {}
void ExecClient::tickSnapshotEnd(int reqId) {}
void ExecClient::verifyCompleted( bool isSuccessful, const std::string& errorText) {}
void ExecClient::displayGroupList( int reqId, const std::string& groups) {}
void ExecClient::verifyMessageAPI( const std::string& apiData) {}
void ExecClient::accountSummaryEnd( int reqId) {}
void ExecClient::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) {}
void ExecClient::historicalNewsEnd(int requestId, bool hasMore) {}
void ExecClient::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) {}
void ExecClient::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
                                     int side, double price, int size, bool isSmartDepth) {}
void ExecClient::rerouteMktDataReq(int reqId, int conid, const std::string& exchange) {}
void ExecClient::scannerParameters(const std::string& xml) {}
void ExecClient::updateAccountTime(const std::string& timeStamp) {}
void ExecClient::accountDownloadEnd(const std::string& accountName) {}
void ExecClient::accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) {}
void ExecClient::contractDetailsEnd( int reqId) {}
void ExecClient::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) {}
void ExecClient::tickByTickMidPoint(int reqId, time_t time, double midPoint) {}
void ExecClient::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) {}
void ExecClient::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void ExecClient::displayGroupUpdated( int reqId, const std::string& contractInfo) {}
void ExecClient::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast>& ticks, bool done) {}
void ExecClient::accountUpdateMultiEnd( int reqId) {}
void ExecClient::historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done) {}
void ExecClient::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
                                          double optPrice, double pvDividend,
                                          double gamma, double vega, double theta, double undPrice) {}
void ExecClient::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) {}
void ExecClient::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) {}
void ExecClient::verifyAndAuthMessageAPI( const std::string& apiDatai, const std::string& xyzChallenge) {}
void ExecClient::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass,
                                                        const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes) {}
void ExecClient::securityDefinitionOptionalParameterEnd(int reqId) {}
void ExecClient::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
                            double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) {}
void ExecClient::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData) {}
void ExecClient::tickSize( TickerId tickerId, TickType field, int size) {}
void ExecClient::receiveFA(faDataType pFaDataType, const std::string& cxml) {}
void ExecClient::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attribs) {}
void ExecClient::historicalDataUpdate(TickerId reqId, const Bar& bar) {}
void ExecClient::pnlSingle(int reqId, int pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value) {}
void ExecClient::winError( const std::string& str, int lastError) {}
void ExecClient::completedOrdersEnd() {}
void ExecClient::positionMulti( int reqId, const std::string& account,const std::string& modelCode, const Contract& contract, double pos, double avgCost){} 
void ExecClient::positionMultiEnd( int reqId) {}




























