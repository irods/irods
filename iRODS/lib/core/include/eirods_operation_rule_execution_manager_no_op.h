


#ifndef __EIRODS_OPERATION_RULE_EXECUTION_MANAGER_NO_OP_H__
#define __EIRODS_OPERATION_RULE_EXECUTION_MANAGER_NO_OP_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_operation_rule_execution_manager_base.h"

namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief base class for rule execution which will allow
    ///        the use of a stub for client side network plugins
    class operation_rule_execution_manager_no_op : public operation_rule_execution_manager_base {
        public:
        /// =-=-=-=-=-=-=-
        /// @brief Constructor
        operation_rule_execution_manager_no_op( 
            const std::string& _inst,   // plugin name 
            const std::string& _op,     // operation name
            keyValPair_t&      _kvp ) : // fco vars
            operation_rule_execution_manager_base(
                _inst,
                _op,
                _kvp ) {  
        } // ctor
        
        /// =-=-=-=-=-=-=-
        /// @brief necessary virtual dtor
        virtual ~operation_rule_execution_manager_no_op() {}
             
        /// =-=-=-=-=-=-=-
        /// @brief execute rule for pre operation
        error exec_pre_op( 
            std::string& _res ) {
            return SUCCESS();
        }

        /// =-=-=-=-=-=-=-
        /// @brief execute rule for post operation
        error exec_post_op( 
            std::string& _res ) {
           return SUCCESS(); 
        }

        /// =-=-=-=-=-=-=-
        /// @brief execute rule for post operation
        error exec_op( 
            const std::string& _rn, // rule name 
            std::string&       _res) {    // results of call to rule
            return SUCCESS(); 
        }

    }; // class operation_rule_execution_manager_no_op

}; // namespace eirods

#endif // __EIRODS_OPERATION_RULE_EXECUTION_MANAGER_NO_OP_H__




