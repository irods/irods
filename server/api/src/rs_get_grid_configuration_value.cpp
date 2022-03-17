#include "irods/rs_get_grid_configuration_value.hpp"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsErrorTable.h"

#include "irods/irods_server_api_call.hpp"

auto rs_get_grid_configuration_value(RsComm* _comm,
                                     const GridConfigurationInput* _input,
                                     GridConfigurationOutput** _output) -> int
{
    if (!_input || !_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call_without_policy(GET_GRID_CONFIGURATION_VALUE_APN,
                                                 _comm, _input, _output);
}

