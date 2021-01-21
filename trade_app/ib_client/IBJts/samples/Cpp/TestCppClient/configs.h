#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <vector>
#include <stdexcept>

namespace hft {


/**
 * @brief stores information for a contract
 * automatically converts strings to uppercase
 */
class BasicContract {
public:
    BasicContract() = delete;
    BasicContract(  const std::string& sym,
                    const std::string& loc_sym,
                    const std::string& sec_type,
                    const std::string& currency, 
                    const std::string& exch);
    /* getter */
    std::string sym() const;
    std::string loc_sym() const;
    std::string sec_type() const;
    std::string currency() const;
    std::string exch() const;
    
    static std::string uppercase(std::string strToConvert);

private:
    std::string m_sym, m_loc_sym, m_sec_type, m_currency, m_exch;
};


/**
 * @brief stores basic information for a single Futures symbol, 
 * as well as trading information
 */
class FutTradingContract : public BasicContract {
public:
    FutTradingContract() = delete;
    FutTradingContract(const std::string& sym,
                       const std::string& loc_sym,
                       const std::string& sec_type,
                       const std::string& currency, 
                       const std::string& exch,
                       float min_tick, 
                       float commiss_per_contract, 
                       unsigned multiplier, 
                       unsigned chillness, 
                       unsigned num_contracts);

    /* getters */
    float min_tick() const { return m_mt; }
    float commiss_per_contract() const { return m_cpc; }
    float multiplier() const { return m_mlt; }
    float chillness() const { return m_c; }
    float num_contracts() const { return m_nc; }
private:
    float m_mt, m_cpc, m_mlt, m_c, m_nc;
};


/**
 * @brief just like IB assume going long the contract is
 * 1. short the near month
 * 2. long the far month
 *
 * FILE FORMAT:
 *  1. root symbol
 *  2. security type
 *  3. exchange
 *  4. local symbol near
 *  5. local symbol far
 *  6. minimum tick
 *  7. commission_per_contract
 *  8. num contracts per leg
 *  9. multiplier
 *  10. chillness 
 *
 * Example:
 * MES,FUT,GLOBEX,MESH0,MESM0,.25,.47,1,5,0
 *
 * Limitations:
 * - same security type
 *
 */
class FutCalendarSpreadConfig {
public:
    FutCalendarSpreadConfig() = delete;
    FutCalendarSpreadConfig(const std::string& file);

    // members
    std::string root;
    std::string sec_type;
    std::string exch;
    std::string lsym_near;
    std::string lsym_far;
    float min_tick;
    float commiss_per_contract;
    unsigned ncontracts_per_leg;
    unsigned multiplier;
    unsigned chillness;
    float mid_spread;

    static std::map<char,int> create_map();
    static std::map<char,int> contract_months;
private:
    
    // checker and helper
    void checkLSyms() const;
    void makeUpperAndTrim(std::string& s);
};


/**
 * @brief Config object that stores information for several futures symbol
 * takes in information from a config file
 *
 * FILE FORMAT:
 *  1. root symbol
 *  2. security type
 *  3. exchange
 *  4. local symbol 
 *  5. minimum tick
 *  6. commission_per_contract
 *  7. multiplier
 *  8. chillness
 *  9. num_contracts
 *  10. currency 
 *
 * Example:
 * MES,FUT,GLOBEX,MESH0,.25,.47,5,0,1,USD
 *
 */
class FutSymsConfig {
public:
    FutSymsConfig() = delete;
    FutSymsConfig(const std::string& file);

    /* getters */
    unsigned int size             ()                            const { return m_contracts.size(); }
    std::string  syms             (unsigned int idx)            const { return m_contracts[idx].sym(); }
    std::string  sec_types        (unsigned int idx)            const { return m_contracts[idx].sec_type(); }
    std::string  exchs            (unsigned int idx)            const { return m_contracts[idx].exch(); }
    std::string  loc_syms         (unsigned int idx)            const { return m_contracts[idx].loc_sym(); }
    float        min_ticks        (unsigned int idx)            const { return m_contracts[idx].min_tick(); }
    float        cms_per_contracts(unsigned int idx)            const { return m_contracts[idx].commiss_per_contract(); }
    unsigned int multipliers      (unsigned int idx)            const { return m_contracts[idx].multiplier(); }
    unsigned int chillness        (unsigned int idx)            const { return m_contracts[idx].chillness(); }
    unsigned int num_contracts    (unsigned int idx)            const { return m_contracts[idx].num_contracts(); }
    std::string  currencies       (unsigned int idx)            const { return m_contracts[idx].currency(); }
    unsigned int unique_order_id  (const std::string& ticker)   const { return m_unique_order_ids.at(ticker); }
    unsigned int unique_trade_id  (const std::string& ticker)   const { return m_unique_trade_ids.at(ticker); }
    std::string  loc_sym_from_uid (unsigned int uid)            const { 
        for(auto iter = m_unique_order_ids.begin(); iter != m_unique_order_ids.end(); ++iter){
            if( iter->second == uid)
                return iter->first;
        }
        for(auto iter = m_unique_trade_ids.begin(); iter != m_unique_trade_ids.end(); ++iter){
            if( iter->second == uid)
                return iter->first;
        }
        throw std::runtime_error("local symbol not found");
    }

    static std::map<char,int> create_map();
    static std::map<char,int> contract_months;
private:
    std::vector<FutTradingContract> m_contracts;
    std::map<std::string, unsigned int> m_unique_trade_ids;
    std::map<std::string, unsigned int> m_unique_order_ids;


    // checker and helper
    //void makeUpperAndTrim(std::string& s);
};

} // namespace hft


#endif // CONFIG_H

