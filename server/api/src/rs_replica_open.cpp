#include "rs_replica_open.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstring>

auto rs_replica_open(RsComm* _comm, DataObjInp* _input, char** _json_output) -> int
{
    if (!_input || !_json_output) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t* output{};

    const int ec = irods::server_api_call(REPLICA_OPEN_APN, _comm, _input, &output);

    if (ec >= 3) {
        *_json_output = static_cast<char*>(output->buf);
    }

    return ec;
}
