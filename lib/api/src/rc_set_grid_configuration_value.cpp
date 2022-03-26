#include "irods/set_grid_configuration_value.h"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_set_grid_configuration_value(RcComm* _comm, const GridConfigurationInput* _input) -> int
{
    if (!_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm, SET_GRID_CONFIGURATION_VALUE_APN, _input, nullptr, nullptr, nullptr);
}
