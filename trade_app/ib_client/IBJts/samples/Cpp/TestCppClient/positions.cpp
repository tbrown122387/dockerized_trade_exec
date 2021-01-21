#include "positions.h"



namespace hft{


unsigned TickerInfo::s_curr_max_id = 2000;


unsigned TickerInfo::getNewID() { 
    return s_curr_max_id++; 
}    


PositionMgr::PositionMgr(const FutSymsConfig& fut_cfg) {

    for(unsigned int i = 0; i < fut_cfg.size(); ++i){
        TickerInfo ti(fut_cfg.syms(i),
                      fut_cfg.sec_types(i),
                      fut_cfg.currencies(i),
                      fut_cfg.exchs(i));
        m_info.insert(std::pair<std::string, TickerInfo>(fut_cfg.loc_syms(i), ti));
    }
}


unsigned PositionMgr::getTradeID(std::string loc_sym) {
    std::transform(loc_sym.begin(), loc_sym.end(), loc_sym.begin(), ::toupper);
    boost::algorithm::trim(loc_sym);
    return m_info.at(loc_sym).unique_trade_req_id;
}


unsigned PositionMgr::getOrderID(std::string loc_sym) {
    std::transform(loc_sym.begin(), loc_sym.end(), loc_sym.begin(), ::toupper);
    boost::algorithm::trim(loc_sym);
    return m_info.at(loc_sym).unique_order_req_id;
}


std::string PositionMgr::getNonLocalSymbol(std::string loc_sym) {
    std::transform(loc_sym.begin(), loc_sym.end(), loc_sym.begin(), ::toupper);
    boost::algorithm::trim(loc_sym);
    return m_info.at(loc_sym).non_local_symbol;
}


std::string PositionMgr::getSecType(std::string loc_sym) {
    std::transform(loc_sym.begin(), loc_sym.end(), loc_sym.begin(), ::toupper);
    boost::algorithm::trim(loc_sym);
    return m_info.at(loc_sym).security_type;
}


std::string PositionMgr::getCurrency(std::string loc_sym) {
    std::transform(loc_sym.begin(), loc_sym.end(), loc_sym.begin(), ::toupper);
    boost::algorithm::trim(loc_sym);
    return m_info.at(loc_sym).currency;
}


std::string PositionMgr::getExchange(std::string loc_sym) {
    std::transform(loc_sym.begin(), loc_sym.end(), loc_sym.begin(), ::toupper);
    boost::algorithm::trim(loc_sym);
    return m_info.at(loc_sym).exchange;
}


int PositionMgr::getDesiredPosition(std::string sym) {
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    boost::algorithm::trim(sym);
    return m_info.at(sym).desired_position;
}

    
int PositionMgr::getActualPosition(std::string sym) {
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    boost::algorithm::trim(sym);
    return m_info.at(sym).actual_position;
}


std::string PositionMgr::getLocalSymbolFromID(int id) {

    unsigned uid = static_cast<unsigned>(id);
    for(auto iter = m_info.begin(); iter != m_info.end(); ++iter){
        if( iter->second.unique_order_req_id == uid ||
            iter->second.unique_trade_req_id == uid){
            return iter->first;
        }
    }
    throw std::runtime_error("local symbol not found");
}


void PositionMgr::setDesiredPosition(std::string sym, int dp) {
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    boost::algorithm::trim(sym);
    m_info.at(sym).desired_position = dp;
}


void PositionMgr::incrementPosition(std::string sym, int signedShares){
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    boost::algorithm::trim(sym);
    m_info.at(sym).actual_position += signedShares;
}


void PositionMgr::setPosition(std::string sym, int signedShares){
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    boost::algorithm::trim(sym);
    m_info.at(sym).actual_position = signedShares;
}


unsigned PositionMgr::numSymbolsTracked() const {
    return m_info.size();
}

} // namespace hft

