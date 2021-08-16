#include "rs_replica_close.hpp"

#include "api_plugin_number.h"
#include "rodsErrorTable.h"

#include "irods_server_api_call.hpp"

#include <cstring>

auto rs_replica_close(RsComm* _comm, const char* _json_input) -> int
{
    if (!_json_input) {
        return SYS_INVALID_INPUT_PARAM;
    }

    bytesBuf_t input{};
    input.buf = const_cast<char*>(_json_input);
    input.len = static_cast<int>(std::strlen(_json_input)) + 1;

    return irods::server_api_call_without_policy(REPLICA_CLOSE_APN, _comm, &input);
}

