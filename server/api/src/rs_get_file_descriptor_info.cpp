#include "rs_get_file_descriptor_info.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstring>

extern "C"
auto rs_get_file_descriptor_info(rsComm_t* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call(GET_FILE_DESCRIPTOR_INFO_APN, _comm, _json_input, &_json_output);
}
