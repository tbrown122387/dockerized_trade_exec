#ifndef POSITIONS_H
#define POSITIONS_H

#include <string>
#include <map>
#include <boost/algorithm/string.hpp>
#include "configs.h"


namespace hft{


    // TODO force each entry to be checked against a green list


/**
 *
 */
class TickerInfo {
public:
    
    static unsigned s_curr_max_id;
    static unsigned getNewID();

    int actual_position;
    int desired_position;
    const unsigned unique_trade_req_id;
    const unsigned unique_order_req_id;
    const std::string non_local_symbol;
    const std::string security_type;
    const std::string currency;
    const std::string exchange;

    TickerInfo(const std::string& non_local_sym, 
               const std::string& sec_type, 
               const std::string& curr, 
               const std::string& exch)  
        : actual_position(0)
        , desired_position(0)
        , unique_trade_req_id(TickerInfo::getNewID())
        , unique_order_req_id(TickerInfo::getNewID())
        , non_local_symbol(non_local_sym)
        , security_type(sec_type)
        , currency(curr)
        , exchange(exch)
    {};
};


/**
 * @brief position manager class
 */
class PositionMgr {
private:

    std::map<std::string,TickerInfo> m_info;

public:
    

    PositionMgr(const FutSymsConfig& fut_cfg);

    unsigned getTradeID(std::string loc_sym);

    unsigned getOrderID(std::string loc_sym);

    std::string getNonLocalSymbol(std::string loc_sym);

    std::string getSecType(std::string loc_sym);

    std::string getCurrency(std::string loc_sym);

    std::string getExchange(std::string loc_sym);

    int getDesiredPosition(std::string loc_sym);

    int getActualPosition(std::string loc_sym);

    unsigned numSymbolsTracked() const;

    std::string getLocalSymbolFromID(int id);

    // setters
    void setDesiredPosition(std::string sym, int dp);
   
    void incrementPosition(std::string sym, int signedShares);

    void setPosition(std::string sym, int signedShares);

};


} // namespace hft

#endif 
