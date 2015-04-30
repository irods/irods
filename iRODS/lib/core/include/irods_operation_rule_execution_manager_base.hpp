#ifndef __IRODS_OPERATION_RULE_EXECUTION_MANAGER_BASE_HPP__
#define __IRODS_OPERATION_RULE_EXECUTION_MANAGER_BASE_HPP__

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/smart_ptr.hpp>

// =-=-=-=-=-=-=-
// stl includes
#include <string>


namespace irods {
/// =-=-=-=-=-=-=-
/// @brief base class for rule execution which will allow
///        the use of a stub for client side network plugins
    class operation_rule_execution_manager_base {
        public:
            // =-=-=-=-=-=-=-
            /// @brief Constructor
            operation_rule_execution_manager_base(
                const std::string& _inst,   // plugin name
                const std::string& _op ) :  // operation name
                instance_( _inst ),
                op_name_( _op ) {
            }

            /// =-=-=-=-=-=-=-
            /// @brief necessary virtual dtor
            virtual ~operation_rule_execution_manager_base() {}

            /// =-=-=-=-=-=-=-
            /// @brief execute rule for pre operation
            virtual error exec_pre_op(
                rsComm_t*,          // server connection
                keyValPair_t&,      // vars from fco
                std::string& ) = 0; // rule results

            /// =-=-=-=-=-=-=-
            /// @brief execute rule for post operation
            virtual error exec_post_op(
                rsComm_t*,          // server connection
                keyValPair_t&,      // vars from fco
                std::string& ) = 0; // rule results

        protected:
            /// =-=-=-=-=-=-=-
            /// @brief execute rule for post operation
            virtual error exec_op(
                rsComm_t*,          // server connection
                keyValPair_t&,      // vars from fco
                const std::string&, // rule name
                std::string& ) = 0; // results of call to rule

            std::string instance_; // instance name of the plugin
            std::string op_name_;  // operation name

    }; // class operation_rule_execution_manager_base

    typedef boost::shared_ptr< operation_rule_execution_manager_base > oper_rule_exec_mgr_ptr;

}; // namespace irods

#endif // __IRODS_OPERATION_RULE_EXECUTION_MANAGER_BASE_HPP__




