
#include "irods_operation_rule_execution_manager_no_op.hpp"
extern "C" void* operation_rule_execution_manager_factory(
        const char* _plugin_name,
        const char* _operation_name ) {
    return new irods::operation_rule_execution_manager_no_op(
                            _plugin_name,
                            _operation_name );
}


