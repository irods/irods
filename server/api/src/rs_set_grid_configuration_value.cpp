#include "irods/rs_set_grid_configuration_value.hpp"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsErrorTable.h"

#include "irods/irods_server_api_call.hpp"

auto rs_set_grid_configuration_value(RsComm* _comm, const GridConfigurationInput* _input) -> int
{
    if (!_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call_without_policy(SET_GRID_CONFIGURATION_VALUE_APN, _comm, _input);
}

