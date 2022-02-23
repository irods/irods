#include "irods/rs_replica_open.hpp"

#include "irods/plugins/api/api_plugin_number.h"
#include "irods/rodsErrorTable.h"

#include "irods/irods_server_api_call.hpp"

#include <cstdlib>
#include <cstring>

auto rs_replica_open(RsComm* _comm, DataObjInp* _input, char** _json_output) -> int
{
    if (!_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t* output{};

    const int ec = irods::server_api_call_without_policy(REPLICA_OPEN_APN, _comm, _input, &output);

    if (ec >= 3) {
        *_json_output = static_cast<char*>(output->buf);
        std::free(output);
    }

    return ec;
}
