#include "rs_sync_with_physical_object.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstring>

auto rs_sync_with_physical_object(rsComm_t* _comm, const char* _json_input) -> int
{
    if (!_json_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    return irods::server_api_call(SYNC_WITH_PHYSICAL_OBJECT_APN, _comm, _json_input);
}
