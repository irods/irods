


#ifndef __EIRODS_OPERATION_RULE_EXECUTION_MANAGER_FACTORY_H__
#define __EIRODS_OPERATION_RULE_EXECUTION_MANAGER_FACTORY_H__

#include "eirods_operation_rule_execution_manager.h"
#include "eirods_operation_rule_execution_manager_no_op.h"

namespace eirods {
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
}; // namespace eirods

#endif // __EIRODS_OPERATION_RULE_EXECUTION_MANAGER_FACTORY_H__



