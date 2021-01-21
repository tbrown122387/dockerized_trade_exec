#ifndef ORDER_ID_MANAGER_H
#define ORDER_ID_MANAGER_H


#include <type_traits>
#include <array>
#include <algorithm> // max_element


namespace hft{


/*
 * @brief 
 * Sending orders to IB with
 * previously used IDs results 
 * in order *modification* 
 * instead of fresh orders being
 * sent. The complication, though,
 * is that once a fill or 
 * cancellation is received, 
 * a fresh id must be generated,
 * and this fresh id must be greater
 * than all currently-used and
 * previously-used id numbers.
 *
 * Features
 *
 * 1. requires starting id to be set externally
 * 2. uses -1 for unused order_id
 * 3. user requests id, and if one 
 * isn't previously used, it will 
 * generate a new one
 * 4. upon receiving the ideal
 * max from the broker, our current
 * max is checked, and so are all of 
 * the order ids
 * 5. provides has_changed() and 
 * last() which let you know if  
 * the previous should be cancelled,
 * and then allow you to cancel it
 */
template<typename id_t, size_t num_ids>
class order_id_mgr{
private:
    
    // assert id_t is like an integer
    static_assert(std::is_integral<id_t>::value, "id_t must be an int or long\n");
    static_assert(std::is_signed<id_t>::value, "id_t must be able to be negative\n");

    // current max id
    // you need this because all orders can
    // either be noncompliant, or be reset to -1
    static id_t s_curr_max;

    // containers of ids
    static std::array<id_t,num_ids> s_ids;

    // container f previous ids
    static std::array<id_t,num_ids> s_last_ids;

public:


    /**
     * @brief resets all id numbers to -1 
     */
    static void reset() { 
        s_last_ids = s_ids;
        s_ids.fill(-1); 
    }


    /**
     * @brief checks that max >= n
     * if not, resets all id numbers because 
     * there was an unrecorded fill,
     * or because the trade client started 
     * off with a higher order id (recall
     * that they are persistent between 
     * sessions) 
     */
    static void checkMaxAgainst(id_t n) {
        if(n > s_curr_max || s_curr_max < *std::max_element(s_ids.begin(), s_ids.end()) ){
            s_curr_max = n;
            reset();
        }
    }

    
    /**
     * @brief gets the recycled order
     * id for a certain idx. If there 
     * isn't one, then a *compliant*
     * new one is auto-generated
     * previous compliance is not checked
     * here...only in checkMaxAgainst()
     */
    static id_t getID(unsigned idx)  {
       
        if(s_curr_max == -1)
           throw std::runtime_error("nextValidId hasnt been set by the broker yet\n");

        id_t tmp = s_ids.at(idx);
        if(tmp != -1){
            s_last_ids.at(idx) = tmp;
            return tmp;
        }else{ 
            // generate a new order id
            s_curr_max++;
            s_ids.at(idx) = s_curr_max;
            return s_curr_max;
        }
    }
    

    static id_t getLastID(unsigned idx){
        id_t tmp = s_last_ids.at(idx);
        if( tmp != -2)
            return tmp;
        else
            throw std::runtime_error("trying to access nonexistent last id\n");
    }


    static bool hasChanged(unsigned idx){
        return s_last_ids.at(idx) != s_ids.at(idx); 
    }

    /**
     * @brief used to start the array elements to -1
     */
    static std::array<id_t,num_ids> makeArray(id_t num) {
        std::array<id_t,num_ids> a;
        a.fill(num);
        return a;
    }
};

template<typename id_t, size_t num_ids>
id_t order_id_mgr<id_t,num_ids>::s_curr_max = -1; 

template<typename id_t, size_t num_ids>
std::array<id_t,num_ids> order_id_mgr<id_t,num_ids>::s_ids = order_id_mgr<id_t,num_ids>::makeArray(-1);

template<typename id_t, size_t num_ids>
std::array<id_t,num_ids> order_id_mgr<id_t,num_ids>::s_last_ids = order_id_mgr<id_t,num_ids>::makeArray(-2);


} // namespace hft

#endif // ORDER_ID_MANAGER_H
