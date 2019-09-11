#include "rs_batch_apply_metadata_operations.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstring>

extern "C"
auto rs_batch_apply_metadata_operations(rsComm_t* _comm, const char* _json_input, char** _json_output) -> int
{
    if (!_json_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call(BATCH_APPLY_METADATA_OPERATIONS_APN, _comm, _json_input, &_json_output);
}
