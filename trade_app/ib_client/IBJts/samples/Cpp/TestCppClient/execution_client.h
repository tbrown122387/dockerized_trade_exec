/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_SAMPLES_TESTCPPCLIENT_TESTCPPCLIENT_H
#define TWS_API_SAMPLES_TESTCPPCLIENT_TESTCPPCLIENT_H

#include "EWrapper.h"
#include "EReaderOSSignal.h"
#include "EReader.h"

#include <memory>
#include <vector>

// my stuff
#include "configs.h"
#include "positions.h"
#define PNL_REGID 123
#define POS_REGID 567

class EClientSocket;

enum State {
    ST_CONNECT,
    ST_REQTICKBYTICKDATA,
    ST_REQPNL,
    ST_REQPOSITIONS,
    ST_CHECK_POSITIONS,
    ST_CHECK_PNL
};

class ExecClient : public EWrapper
{
private:

public:

	ExecClient();
	~ExecClient();

	void setConnectOptions(const std::string&);
	void processMessages();

public:

	bool connect(const char * host, int port, int clientId = 0);
	void disconnect() const;
	bool isConnected() const;

private:
    void pnlOperation();
    void reqTickByTickData();
    void reqPNL();
    void reqPositions();
    void orderOperations();
public:
	// events
	#include "EWrapper_prototypes.h"

private:
	EReaderOSSignal m_osSignal;
	EClientSocket * const m_pClient;
	State m_state;

	OrderId m_orderId;
	EReader *m_pReader;
    bool m_extraAuth;
	std::string m_bboExchange;

    // new stuff! 
    const bool m_printing;
    bool m_closeout_only;
    const double m_maxLoss;
    hft::FutSymsConfig m_ticker_config;
    hft::PositionMgr m_positions;
    double m_current_profits;
    double m_high_water_profit;
    // rolling window stuff

    inline void market_sell(const std::string& local_symbol, unsigned qty);
    inline void market_buy(const std::string& local_symbol, unsigned qty);
    inline void close_all_positions();

};

#endif

