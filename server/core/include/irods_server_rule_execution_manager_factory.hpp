
#ifndef SERVER_RULE_EXECUTION_MANAGER_FACTORY
#define SERVER_RULE_EXECUTION_MANAGER_FACTORY

#include "irods_operation_rule_execution_manager.hpp"
extern "C" void* operation_rule_execution_manager_factory(
        const char* _plugin_name,
        const char* _operation_name ) {
    return new irods::operation_rule_execution_manager(
                            _plugin_name,
                            _operation_name );
}
#endif // SERVER_RULE_EXECUTION_MANAGER_FACTORY

