


#ifndef __IRODS_OPERATION_RULE_EXECUTION_MANAGER_FACTORY_HPP__
#define __IRODS_OPERATION_RULE_EXECUTION_MANAGER_FACTORY_HPP__

#include "irods_operation_rule_execution_manager.hpp"
#include "irods_operation_rule_execution_manager_no_op.hpp"

namespace irods {
#if 0
#ifdef RODS_SERVER
    typedef operation_rule_execution_manager       MANAGER_TYPE;
#else
    typedef operation_rule_execution_manager_no_op MANAGER_TYPE;
#endif

    static oper_rule_exec_mgr_ptr operation_rule_execution_manager_factory(
        const std::string& _inst,
        const std::string& _op,
        keyValPair_t&      _kvp ) {

        return oper_rule_exec_mgr_ptr( new MANAGER_TYPE( _inst, _op, _kvp ) );

    } // operation_rule_execution_manager_factory

#endif
}; // namespace irods

#endif // __IRODS_OPERATION_RULE_EXECUTION_MANAGER_FACTORY_HPP__



