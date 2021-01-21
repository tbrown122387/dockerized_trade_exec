#include "configs.h"

#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <cmath> // round

#include <iostream>
namespace hft{


BasicContract::BasicContract(const std::string& sym, const std::string& loc_sym,
                             const std::string& sec_type, const std::string& currency, 
                             const std::string& exch)
    : m_sym(uppercase(sym)), m_loc_sym(uppercase(loc_sym)), m_sec_type(uppercase(sec_type)), 
    m_currency(uppercase(currency)), m_exch(uppercase(exch))
{
}


std::string BasicContract::sym() const { return m_sym; }
std::string BasicContract::loc_sym() const { return m_loc_sym; }
std::string BasicContract::sec_type() const { return m_sec_type; }
std::string BasicContract::currency() const { return m_currency; }
std::string BasicContract::exch() const { return m_exch; }


std::string BasicContract::uppercase(std::string strToConvert)
{
    std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);
    boost::algorithm::trim(strToConvert);
    return strToConvert;
}


FutTradingContract::FutTradingContract(const std::string& sym, 
                                       const std::string& loc_sym, 
                                       const std::string& sec_type, 
                                       const std::string& currency,
                                       const std::string& exch, 
                                       float min_tick, 
                                       float commiss_per_contract, 
                                       unsigned multiplier, 
                                       unsigned chillness, 
                                       unsigned num_contracts)
    : BasicContract(sym, loc_sym, sec_type, currency, exch), 
    m_mt(min_tick), 
    m_cpc(commiss_per_contract), 
    m_mlt(multiplier), 
    m_c(chillness), 
    m_nc(num_contracts)
{
}


FutCalendarSpreadConfig::FutCalendarSpreadConfig(const std::string& file)
{

    std::ifstream fs(file);
    if( fs.good() ){

        std::string _root, _st, _exch, _lsn, _lsf, _mt, _cpc, _ncpl, _mult, _chill, _ms, line;
        while( std::getline(fs, line) ){

            if( !line.empty() ){ // todo: think of a better number

                // split up data
                std::istringstream stream(line);
                std::getline(stream, _root, ',');
                std::getline(stream, _st, ',');
                std::getline(stream, _exch, ',');
                std::getline(stream, _lsn, ',');
                std::getline(stream, _lsf, ',');
                std::getline(stream, _mt, ',');
                std::getline(stream, _cpc, ',');
                std::getline(stream, _ncpl, ',');
                std::getline(stream, _mult, ',');
                std::getline(stream, _chill, ',');
                std::getline(stream, _ms, ',');
    
                // clean strings
                makeUpperAndTrim(_root);
                makeUpperAndTrim(_st);
                makeUpperAndTrim(_exch);
                makeUpperAndTrim(_lsn);
                makeUpperAndTrim(_lsf);
                makeUpperAndTrim(_mt);
                makeUpperAndTrim(_cpc);
                makeUpperAndTrim(_ncpl);
                makeUpperAndTrim(_mult);
                makeUpperAndTrim(_chill);
                makeUpperAndTrim(_ms);

                // store the info
                root = _root;
                sec_type = _st;
                exch = _exch;
                lsym_near = _lsn;
                lsym_far = _lsf;
                min_tick = std::stof(_mt);
                commiss_per_contract = std::stof(_cpc);
                ncontracts_per_leg = std::stoi(_ncpl);
                multiplier = std::stoi(_mult);
                chillness = std::stoi(_chill);
                mid_spread = std::stod(_ms);

            }
        }

    }else{
        throw std::runtime_error("could not open symbol information file\n");
    }

    // check everything
    checkLSyms();

    if( std::abs(mid_spread/min_tick - std::round(mid_spread/ min_tick)) > 0.0)
        throw std::runtime_error("spread must be a multiple of theminimum tick\n");
}


std::map<char,int> FutCalendarSpreadConfig::create_map()
{
    std::map<char,int> m;
    m['F'] = 0; // january
    m['G'] = 1; // feburary
    m['H'] = 2; // march
    m['J'] = 3; // apri
    m['K'] = 4; // may
    m['M'] = 5; // june
    m['N'] = 6; // july
    m['Q'] = 7; // august
    m['U'] = 8; // sept
    m['V'] = 9; // oct
    m['X'] = 10; // nov
    m['Z'] = 11; // dec
    return m;
}


std::map<char,int> FutCalendarSpreadConfig::contract_months = FutCalendarSpreadConfig::create_map(); 


void FutCalendarSpreadConfig::checkLSyms() const
{
    char front_month = lsym_near.at(3);
    char far_month   = lsym_far.at(3);
    int front_int = contract_months[front_month];
    int far_int = contract_months[far_month];

    int front_year = lsym_near.at(4) - '0';
    int far_year   = lsym_far.at(4) - '0';

    // front month can be greater, but that must be because it's in an earlier year
    if(front_int > far_int && front_year >= far_year){
        throw std::runtime_error("front month local symbol must come first in ctor\n");
    }
}


void FutCalendarSpreadConfig::makeUpperAndTrim(std::string& s)
{
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    boost::algorithm::trim(s);
}


FutSymsConfig::FutSymsConfig(const std::string& file)
{

    std::ifstream fs(file);
    unsigned int dyn_sym_unique_id (4000);
    if( fs.good() ){

        std::string _root, _st, _exch, _ls, _mt, _cpc, _mult, _chill, _nc, _curr, line;
        while( std::getline(fs, line) ){

            if( !line.empty() ){

                // split up data
                std::istringstream stream(line);
                std::getline(stream, _root, ',');
                std::getline(stream, _st, ',');
                std::getline(stream, _exch, ',');
                std::getline(stream, _ls, ',');
                std::getline(stream, _mt, ',');
                std::getline(stream, _cpc, ',');
                std::getline(stream, _mult, ',');
                std::getline(stream, _chill, ',');
                std::getline(stream, _nc, ',');
                std::getline(stream, _curr, ',');

                // store the info
                m_contracts.push_back(FutTradingContract(_root, _ls, _st, _curr, _exch, 
                                                         std::stof(_mt),
                                                         std::stof(_cpc), 
                                                         std::stoi(_mult), 
                                                         std::stoi(_chill), 
                                                         std::stoi(_nc)));
            
                // set order and trade ids
                m_unique_order_ids.insert(
                        std::pair<std::string,unsigned int>(
                            BasicContract::uppercase(_ls), 
                                dyn_sym_unique_id++));
                m_unique_trade_ids.insert(
                        std::pair<std::string,unsigned int>(
                            BasicContract::uppercase(_ls), 
                                dyn_sym_unique_id++));
            }
        }

    }else{
        throw std::runtime_error("could not open symbol information file\n");
    }
}


std::map<char,int> FutSymsConfig::create_map()
{
    std::map<char,int> m;
    m['F'] = 0; // january
    m['G'] = 1; // feburary
    m['H'] = 2; // march
    m['J'] = 3; // apri
    m['K'] = 4; // may
    m['M'] = 5; // june
    m['N'] = 6; // july
    m['Q'] = 7; // august
    m['U'] = 8; // sept
    m['V'] = 9; // oct
    m['X'] = 10; // nov
    m['Z'] = 11; // dec
    return m;
}


std::map<char,int> FutSymsConfig::contract_months = FutSymsConfig::create_map(); 


} //namespace hft
