#ifndef __IRODS_OPERATION_RULE_EXECUTION_MANAGER_NO_OP_HPP__
#define __IRODS_OPERATION_RULE_EXECUTION_MANAGER_NO_OP_HPP__

// =-=-=-=-=-=-=-
#include "irods_operation_rule_execution_manager_base.hpp"

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief base class for rule execution which will allow
///        the use of a stub for client side network plugins
    class operation_rule_execution_manager_no_op : public operation_rule_execution_manager_base {
        public:
            /// =-=-=-=-=-=-=-
            /// @brief Constructor
            operation_rule_execution_manager_no_op(
                const std::string& _inst,   // plugin name
                const std::string& _op ) :  // operation name
                operation_rule_execution_manager_base(
                    _inst,
                    _op ) {
            } // ctor

            /// =-=-=-=-=-=-=-
            /// @brief necessary virtual dtor
            virtual ~operation_rule_execution_manager_no_op() {}

            /// =-=-=-=-=-=-=-
            /// @brief execute rule for pre operation
            error exec_pre_op(
                rsComm_t*,      // server connection
                keyValPair_t&,  // fco vars
                std::string& ) {
                return SUCCESS();
            }

            /// =-=-=-=-=-=-=-
            /// @brief execute rule for post operation
            error exec_post_op(
                rsComm_t*,      // server connection
                keyValPair_t&,  // fco vars
                std::string& ) {
                return SUCCESS();
            }

            /// =-=-=-=-=-=-=-
            /// @brief execute rule for post operation
            error exec_op(
                rsComm_t*,              // server connection
                keyValPair_t&,          // fco vars
                const std::string&,     // rule name
                std::string& ) {        // results of call to rule
                return SUCCESS();
            }

    }; // class operation_rule_execution_manager_no_op

}; // namespace irods

#endif // __IRODS_OPERATION_RULE_EXECUTION_MANAGER_NO_OP_HPP__




