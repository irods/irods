#include "irods/get_grid_configuration_value.h"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/procApiRequest.h"
#include "irods/rodsErrorTable.h"

auto rc_get_grid_configuration_value(RcComm* _comm,
                                     const GridConfigurationInput* _input,
                                     GridConfigurationOutput** _output) -> int
{
    if (!_input || !_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return procApiRequest(_comm, GET_GRID_CONFIGURATION_VALUE_APN,
                          _input, nullptr,
                          reinterpret_cast<void**>(_output), nullptr);
}
